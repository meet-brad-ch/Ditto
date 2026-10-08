// Clip.cpp: implementations of the Clip interfaces
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CP_Main.h"
#include "Clip.h"
#include "AppState.h"
#include "ClipContext.h"
#include "RegisteredClipboardFormats.h"
#include "DatabaseUtilities.h"
#include "sqlite\CppSQLite3.h"
#include "..\Shared\TextConvert.h"
#include "zlib.h"
#include "Misc.h"
#include "Md5.h"
#include "ImageHelper.h"

#include <Mmsystem.h>
#include <memory>

#include "Path.h"
#include "ClipboardFormatError.h"
#include "ClipText.h"
#include "ErrorReport.h"
#include "FileDataRecord.h"
#include "GlobalBytes.h"
#include "GlobalFileDrop.h"
#include "RtfNormalizer.h"
#include "ClipOrder.h"
#include "DittoDbTransaction.h"
#include <algorithm>
#include <cstdint>
#include <set>
#include <stdexcept>


/*----------------------------------------------------------------------------*\
COleDataObjectEx
\*----------------------------------------------------------------------------*/

HGLOBAL COleDataObjectEx::StreamToGlobal(IStream* stream)
{
	const LARGE_INTEGER start{};
	ULARGE_INTEGER size{};
	if (FAILED(stream->Seek(start, STREAM_SEEK_END, &size)) || size.HighPart != 0 || size.LowPart == 0)
	{
		return NULL;
	}
	HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, size.LowPart);
	if (hGlobal == NULL)
	{
		return NULL;
	}

	ULONG bytesRead{};
	HRESULT result{ E_FAIL };
	{
		DittoCore::GlobalBytes block(hGlobal);
		if (SUCCEEDED(stream->Seek(start, STREAM_SEEK_SET, NULL)))
		{
			result = stream->Read(block.WritableBytes().data(), size.LowPart, &bytesRead);
		}
	}
	if (FAILED(result) || bytesRead != size.LowPart)
	{
		return GlobalFree(hGlobal); // returns NULL
	}
	return hGlobal;
}

HGLOBAL COleDataObjectEx::GetGlobalData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc)
{
	HGLOBAL hGlobal = COleDataObject::GetGlobalData(cfFormat, lpFormatEtc);
	if (hGlobal)
	{
		if (!CGlobalMemory::IsValid(hGlobal))
		{
			CLogger::Log(CStringUtil::Format(
				_T("COleDataObjectEx::GetGlobalData(\"%s\"): ERROR: Invalid (NULL) data returned."),
				CClipboardFormats::GetFormatName(cfFormat).GetString()));
			::GlobalFree(hGlobal);
			hGlobal = NULL;
		}
		return hGlobal;
	}

	// The data isn't in global memory, so try getting an IStream interface to it.
	STGMEDIUM stg;

	if (!GetData(cfFormat, &stg))
	{
		return 0;
	}

	switch (stg.tymed)
	{
	case TYMED_HGLOBAL:
		hGlobal = stg.hGlobal;
		break;

	case TYMED_ISTREAM:
		hGlobal = StreamToGlobal(stg.pstm);
		break;
	} // end switch

	ReleaseStgMedium(&stg);

	if (hGlobal && !CGlobalMemory::IsValid(hGlobal))
	{
		CLogger::Log(CStringUtil::Format(
			_T("COleDataObjectEx::GetGlobalData(\"%s\"): ERROR: Invalid (NULL) data returned."),
			CClipboardFormats::GetFormatName(cfFormat).GetString()));
		::GlobalFree(hGlobal);
		hGlobal = NULL;
	}

	return hGlobal;
}

std::shared_ptr<CClipTypes> COleDataObjectEx::GetAvailableTypes(HWND clipboardOwner)
{
	std::shared_ptr<CClipTypes> types = std::make_shared<CClipTypes>();

	// GetNextFormat API has a bug that cannot find avaliable formats correctly. (ex. CF_DIB)
	// So, Use EnumClipboardFormats API.
	if (!OpenClipboard(clipboardOwner))
		return types;

	UINT format = 0;
	do
	{
		format = EnumClipboardFormats(format);
		// Currently CF_MAX is not valid format
		// See https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
		if (format == 0 || format == CF_MAX)
			continue;
		// clipboard format ids are 16-bit (registered formats are 0xC000-0xFFFF)
		types->Add(static_cast<CLIPFORMAT>(format));
	} while (format != 0);

	CloseClipboard();
	return types;
}

/*----------------------------------------------------------------------------*\
CClipFormat - holds the data of one clip format.
\*----------------------------------------------------------------------------*/
CClipFormat::CClipFormat(CLIPFORMAT cfType, HGLOBAL hgData, int parentId)
{
	m_cfType = cfType;
	m_hgData = hgData;
	m_autoDeleteData = true;
	m_parentId = parentId;
}

CClipFormat::~CClipFormat()
{
	Free();
}

void CClipFormat::Clear()
{
	m_cfType = 0;
	m_hgData = 0;
	m_dataId = -1;
	m_parentId = -1;
}

void CClipFormat::Free()
{
	if (m_autoDeleteData && m_hgData)
	{
		m_hgData = ::GlobalFree(m_hgData);
		m_hgData = NULL;
	}
}

DWORD CClip::TakeDword(HGLOBAL block)
{
	const std::unique_ptr<void, decltype(&::GlobalFree)> owner(block, &::GlobalFree);
	const DittoCore::GlobalBytes bytes(block);
	DWORD value{};
	if (bytes.Bytes().size() < sizeof(value))
	{
		throw DittoCore::ClipboardFormatError("DWORD clipboard format has " + std::to_string(bytes.Bytes().size()) + " bytes");
	}
	memcpy(&value, bytes.Bytes().data(), sizeof(value));
	return value;
}

CStringA CClipFormat::GetAsCStringA()
{
	if (m_hgData == nullptr)
	{
		return CStringA();
	}
	const DittoCore::GlobalBytes bytes(m_hgData);
	const std::string text = DittoCore::ClipText::ReadAnsiBounded(bytes.Bytes());
	return CStringA(text.c_str(), static_cast<int>(text.size()));
}

CString CClipFormat::GetAsCString()
{
	if (m_hgData == nullptr)
	{
		return CString();
	}
	const DittoCore::GlobalBytes bytes(m_hgData);
	const std::wstring text = DittoCore::ClipText::ReadWideBounded(bytes.Bytes());
	return CString(text.c_str(), static_cast<int>(text.size()));
}

Gdiplus::Bitmap* CClipFormat::CreateGdiplusBitmap()
{
	// ownership: the add-in caller (IClipFormat's raw-pointer ABI)
	return LoadGdiplusBitmap(CClipboardFormats::GetFormatID(_T("PNG"))).release();
}

std::unique_ptr<Gdiplus::Bitmap> CClipFormat::LoadGdiplusBitmap(CLIPFORMAT pngFormat)
{
	if (this->m_cfType != CF_DIB && this->m_cfType != pngFormat)
		return nullptr;

	if (this->m_cfType == pngFormat)
		return PNGImageHelper::GdipImageFromHGLOBAL(this->m_hgData);
	return DIBImageHelper::GdipImageFromHGLOBAL(this->m_hgData);
}

/*----------------------------------------------------------------------------*\
CClipFormats - holds an array of CClipFormat
\*----------------------------------------------------------------------------*/
// returns a pointer to the CClipFormat in this array which matches the given type
//  or NULL if that type doesn't exist in this array.
CClipFormat* CClipFormats::FindFormat(UINT cfType)
{
	CClipFormat* pCF;
	INT_PTR count = GetSize();

	for (int i = 0; i < count; i++)
	{
		pCF = &ElementAt(i);
		if (pCF->m_cfType == cfType)
			return pCF;
	}
	return NULL;
}

bool CClipFormats::RemoveFormat(CLIPFORMAT cfType)
{
	bool removed = false;
	CClipFormat* pCF;
	INT_PTR count = GetSize();

	for (int i = 0; i < count; i++)
	{
		pCF = &ElementAt(i);
		if (pCF->m_cfType == cfType)
		{
			this->RemoveAt(i);
			removed = true;
			break;
		}
	}
	return removed;
}


/*----------------------------------------------------------------------------*\
CClip - holds multiple CClipFormats and CopyClipboard() statistics
\*----------------------------------------------------------------------------*/

CClip::CClip(CClipContext& context) :
	m_id(-1),
	m_CRC(0),
	m_parentId(-1),
	m_dontAutoDelete(FALSE),
	m_shortCut(0),
	m_bIsGroup(FALSE),
	m_param1(0),
	m_clipOrder(0),
	m_stickyClipOrder(CClip::InvalidSticky),
	m_stickyClipGroupOrder(CClip::InvalidSticky),
	m_clipGroupOrder(0),
	m_globalShortCut(FALSE),
	m_moveToGroupShortCut(0),
	m_globalMoveToGroupShortCut(FALSE),
	m_context(context)
{
	m_copyReason = CopyReasonEnum::COPY_TO_UNKOWN;
	m_addToDbStickyEnum = AddToDbStickyEnum::INVALID;
}

const DittoCore::ClipSavePolicy& CClip::SavePolicy()
{
	if (!m_savePolicy.has_value())
	{
		m_savePolicy.emplace(m_context.Settings().GetClipSaveSettings());
	}
	return *m_savePolicy;
}

CClip::~CClip()
{
	EmptyFormats();
}

void CClip::Clear()
{
	m_id = -1;
	m_Time = 0;
	m_Desc = "";
	m_CRC = 0;
	m_parentId = -1;
	m_dontAutoDelete = FALSE;
	m_shortCut = 0;
	m_bIsGroup = FALSE;
	m_csQuickPaste = "";
	m_param1 = 0;
	m_globalShortCut = FALSE;
	m_moveToGroupShortCut = 0;
	m_globalMoveToGroupShortCut = 0;

	EmptyFormats();
}

const CClip& CClip::operator=(const CClip& clip)
{
	const CClipFormat* pCF;

	m_id = clip.m_id;
	m_Time = clip.m_Time;
	m_lastPasteDate = clip.m_lastPasteDate;
	m_CRC = clip.m_CRC;
	m_parentId = clip.m_parentId;
	m_dontAutoDelete = clip.m_dontAutoDelete;
	m_shortCut = clip.m_shortCut;
	m_bIsGroup = clip.m_bIsGroup;
	m_csQuickPaste = clip.m_csQuickPaste;
	m_moveToGroupShortCut = clip.m_moveToGroupShortCut;
	m_globalMoveToGroupShortCut = clip.m_globalMoveToGroupShortCut;

	INT_PTR nCount = clip.m_Formats.GetSize();

	for (int i = 0; i < nCount; i++)
	{
		pCF = &clip.m_Formats.GetData()[i];

		LPVOID pvData = GlobalLock(pCF->m_hgData);
		if (pvData)
		{
			AddFormat(pCF->m_cfType, pvData, (UINT)GlobalSize(pCF->m_hgData));
		}
		GlobalUnlock(pCF->m_hgData);
	}

	//Set this after since in could get the wrong description in AddFormat
	m_Desc = clip.m_Desc;

	return *this;
}

void CClip::EmptyFormats()
{
	// free global memory in m_Formats
	for (INT_PTR i = m_Formats.GetSize() - 1; i >= 0; i--)
	{
		m_Formats[i].Free();
		m_Formats.RemoveAt(i);
	}
}

// Adds a new Format to this Clip by copying the given data.
bool CClip::AddFormat(CLIPFORMAT cfType, void* pData, SIZE_T nLen, bool setDesc)
{
	ASSERT(pData && nLen);
	HGLOBAL hGlobal = CGlobalMemory::NewGlobalP(pData, nLen);
	ASSERT(hGlobal);

	// update the Clip statistics
	m_Time = m_Time.GetCurrentTime();

	if (setDesc)
	{
		if (cfType != CF_UNICODETEXT || !SetDescFromText(hGlobal, true))
			SetDescFromType();
	}

	CClipFormat format(cfType, hGlobal);
	CClipFormat* pFormat;

	pFormat = m_Formats.FindFormat(cfType);
	// if the format type already exists as part of this clip, replace the data
	if (pFormat)
	{
		pFormat->Free();
		pFormat->m_hgData = format.m_hgData;
	}
	else
	{
		m_Formats.Add(format);
	}

	format.m_hgData = 0; // now owned by m_Formats
	return true;
}

// Fills this CClip with the contents of the clipboard.
int CClip::LoadFromClipboard(CClipTypes* pClipTypes, CRegExFilterHelper& regexFilters, bool /*checkClipboardIgnore*/, CString activeApp)
{
	if (pClipTypes == NULL || pClipTypes->GetSize() == 0)
	{
		ASSERT(0); // this feature is not currently used... it is an error if it is.
		CLogger::Log(_T("no types were given to accept, skipping this clipboard change"));
		return FALSE;
	}

	COleDataObjectEx oleData;
	CClipTypes* pTypes = pClipTypes;

	// m_Formats should be empty when this is called.
	ASSERT(m_Formats.GetSize() == 0);

	if (!MayReadClipboard())
	{
		return FALSE;
	}

	//Attach to the clipboard
	if (!oleData.AttachClipboard())
	{
		CLogger::Log(_T("failed to attache to clipboard, skipping this clipboard change"));
		ASSERT(0); // does this ever happen?
		return FALSE;
	}

	oleData.EnsureClipboardObject();

	if (IsExcludedFromHistory(oleData))
	{
		oleData.Release();
		return FALSE;
	}

	m_Desc = "[Ditto Error] BAD DESCRIPTION";

	// Get Description String
	// NOTE: We make sure that the description always corresponds to the
	//  data saved by using the exact same globalmem instance as the source
	//  for both... i.e. we only fetch the description format type once.
	CClipFormat cfDesc;
	const bool bIsDescSet{ LoadDescription(oleData, cfDesc) };

	CClipFormat cf;
	if (!LoadClipboardFormats(oleData, *pTypes, cf, cfDesc, activeApp))
	{
		oleData.Release();
		return -1;
	}

	return FinishLoadFromClipboard(oleData, cfDesc, bIsDescSet, regexFilters, activeApp);
}

bool CClip::MayReadClipboard()
{
	// If the data is supposed to be private, then return
	const CRegisteredClipboardFormats& formats{ m_context.Formats() };
	if (::IsClipboardFormatAvailable(formats.IgnoreClipboard()))
	{
		CLogger::Log(_T("Clipboard ignore type is on the clipboard, skipping this clipboard change"));
		return false;
	}

	if (SavePolicy().Settings().enforceIgnoreFormats)
	{
		//https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
		if (::IsClipboardFormatAvailable(formats.ExcludeClipboardContentFromMonitorProcessing()))
		{
			CLogger::Log(_T("ExcludeClipboardContentFromMonitorProcessing type is on the clipboard, skipping this clipboard change"));
			return false;
		}
	}

	//If we are saving a multi paste then delay us connecting to the clipboard
	//to allow the ctrl-v to do a paste
	if (::IsClipboardFormatAvailable(formats.DelaySavingData()))
	{
		CLogger::Log(_T("Delay clipboard type is on the clipboard, delaying 1500 ms to allow ctrl-v to work"));
		Sleep(1500);
	}

	return true;
}

bool CClip::IsExcludedFromHistory(COleDataObjectEx& oleData)
{
	//https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
	const CLIPFORMAT canInclude{ m_context.Formats().CanIncludeInClipboardHistory() };
	if (SavePolicy().Settings().enforceIgnoreFormats &&
		oleData.IsDataAvailable(canInclude))
	{
		HGLOBAL includeInHistory = oleData.GetGlobalData(canInclude);
		if (includeInHistory != nullptr && TakeDword(includeInHistory) == 0)
		{
			CLogger::Log(_T("CanIncludeInClipboardHistory is 0, skipping this clipboard change"));
			return true;
		}
	}

	return false;
}

bool CClip::LoadDescription(COleDataObjectEx& oleData, CClipFormat& cfDesc)
{
	if (TryDescriptionFormat(oleData, cfDesc, CF_UNICODETEXT, true, _T("cf_unicode")))
	{
		return true;
	}

	return TryDescriptionFormat(oleData, cfDesc, CF_TEXT, false, _T("cf_text"));
}

bool CClip::TryDescriptionFormat(COleDataObjectEx& oleData, CClipFormat& cfDesc, CLIPFORMAT type, bool unicode, const TCHAR* typeName)
{
	cfDesc.m_cfType = type;
	if (!oleData.IsDataAvailable(cfDesc.m_cfType))
	{
		return false;
	}

	for (int i = 0; i < 10; i++)
	{
		cfDesc.m_hgData = oleData.GetGlobalData(cfDesc.m_cfType);
		if (cfDesc.m_hgData != NULL)
		{
			break;
		}
		CLogger::Log(CStringUtil::Format(_T("Tried to set description from %s, data is NULL, try: %d"), typeName, i + 1));
		Sleep(10);
	}

	const bool bIsDescSet{ SetDescFromText(cfDesc.m_hgData, unicode) };

	CLogger::Log(CStringUtil::Format(_T("Tried to set description from %s text, Set: %d, Desc: [%s]"), typeName, bIsDescSet, m_Desc.Left(30).GetString()));

	return bIsDescSet;
}

bool CClip::LoadClipboardFormats(COleDataObjectEx& oleData, CClipTypes& types, CClipFormat& cf, CClipFormat& cfDesc, CString& activeApp)
{
	INT_PTR numTypes = types.GetSize();

	CLogger::Log(CStringUtil::Format(_T("Begin enumerating over supported types, Count: %d"), numTypes));

	for (int i = 0; i < numTypes; i++)
	{
		cf.m_cfType = types.ElementAt(i);

		if (!LoadClipboardFormat(oleData, cf, cfDesc, activeApp))
		{
			return false;
		}
	}

	CLogger::Log(CStringUtil::Format(_T("End enumerating over supported types, Count: %d"), numTypes));

	return true;
}

bool CClip::LoadClipboardFormat(COleDataObjectEx& oleData, CClipFormat& cf, CClipFormat& cfDesc, CString& activeApp)
{
	if (IsIgnoredDib(oleData, cf.m_cfType, activeApp))
	{
		CLogger::Log(CStringUtil::Format(_T("Ignore CF_DIB from %s"), activeApp.GetString()));
		return true;
	}

	BOOL bSuccess = false;
	CLogger::Log(CStringUtil::Format(_T("Begin try and load type %s"), CClipboardFormats::GetFormatName(cf.m_cfType).GetString()));

	if (!FetchFormatData(oleData, cf, cfDesc))
	{
		return true;
	}

	if (!StoreFetchedFormat(cf, bSuccess))
	{
		return false;
	}

	CLogger::Log(CStringUtil::Format(_T("End of load - type %s, Success: %d"), CClipboardFormats::GetFormatName(cf.m_cfType).GetString(), bSuccess));

	return true;
}

bool CClip::IsIgnoredDib(COleDataObjectEx& oleData, CLIPFORMAT type, CString& activeApp)
{
	return type == CF_DIB &&
		   oleData.IsDataAvailable(CF_TEXT) &&
		   SavePolicy().IgnoresDibFrom(std::wstring(activeApp.MakeLower().GetString()));
}

bool CClip::FetchFormatData(COleDataObjectEx& oleData, CClipFormat& cf, CClipFormat& cfDesc)
{
	// is this the description we already fetched?
	if (cf.m_cfType == cfDesc.m_cfType)
	{
		cf = cfDesc;
		cfDesc.m_hgData = 0; // cf owns it now (to go into m_Formats)
		return true;
	}

	if (!oleData.IsDataAvailable(cf.m_cfType))
	{
		CLogger::Log(CStringUtil::Format(_T("End of load - Data is not available for type %s"), CClipboardFormats::GetFormatName(cf.m_cfType).GetString()));
		return false;
	}

	for (int tries = 0; tries < 2; tries++)
	{
		cf.m_hgData = oleData.GetGlobalData(cf.m_cfType);
		if (cf.m_hgData != NULL)
			break;

		CLogger::Log(CStringUtil::Format(_T("Tried to get data for type: %s, data is NULL, try: %d"), CClipboardFormats::GetFormatName(cf.m_cfType).GetString(), tries + 1));
		Sleep(5);
	}

	return true;
}

bool CClip::StoreFetchedFormat(CClipFormat& cf, BOOL& bSuccess)
{
	if (!cf.m_hgData)
	{
		return true;
	}

	const INT_PTR nSize = static_cast<INT_PTR>(GlobalSize(cf.m_hgData));
	if (nSize > 0)
	{
		if (SavePolicy().TooLarge(static_cast<std::uint64_t>(nSize)))
		{
			CString cs;
			cs.Format(_T("Maximum clip size reached max size = %lld, clip size = %Id"), SavePolicy().Settings().maxClipSizeInBytes, nSize);
			CLogger::Log(cs);

			return false;
		}

		ASSERT(CGlobalMemory::IsValid(cf.m_hgData));

		m_Formats.Add(cf);
		bSuccess = true;
	}
	else
	{
		ASSERT(FALSE); // a valid GlobalMem with 0 size is strange
		cf.Free();
		CLogger::Log(CStringUtil::Format(_T("Data length is 0 for type %s"), CClipboardFormats::GetFormatName(cf.m_cfType).GetString()));
	}
	cf.m_hgData = 0; // m_Formats owns it now

	return true;
}

int CClip::FinishLoadFromClipboard(COleDataObjectEx& oleData, CClipFormat& cfDesc, bool bIsDescSet, CRegExFilterHelper& regexFilters, CString& activeApp)
{
	m_Time = CTime::GetCurrentTime();

	if (!bIsDescSet)
	{
		SetDescFromType();

		CLogger::Log(CStringUtil::Format(_T("Setting description from type, Desc: [%s]"), m_Desc.Left(30).GetString()));
	}

	// if the description was in a type that is not supported,
	//we have to free it since it wasn't added to m_Formats
	if (cfDesc.m_hgData)
	{
		cfDesc.Free();
	}

	oleData.Release();

	if (m_Formats.GetSize() == 0)
	{
		CLogger::Log(_T("No clip types were in supported types array"));
		return FALSE;
	}

	if (this->m_Desc != _T(""))
	{
		std::wstring stringData(this->m_Desc);
		if (regexFilters.TextMatchFilters(activeApp, stringData))
		{
			return -1;
		}
	}

	return TRUE;
}

bool CClip::SetDescFromText(HGLOBAL hgData, bool unicode)
{
	if (hgData == 0)
		return false;

	const DittoCore::GlobalBytes bytes(hgData);
	if (unicode)
	{
		const std::wstring text = DittoCore::ClipText::ReadWideBounded(bytes.Bytes());
		m_Desc = CString(text.c_str(), static_cast<int>(text.size()));
	}
	else
	{
		const std::string text = DittoCore::ClipText::ReadAnsiBounded(bytes.Bytes());
		m_Desc = CString(CStringA(text.c_str(), static_cast<int>(text.size())));
	}

	const std::size_t descriptionLength{ SavePolicy().Settings().descriptionLength };
	if (static_cast<std::size_t>(m_Desc.GetLength()) > descriptionLength)
	{
		m_Desc = m_Desc.Left(static_cast<int>(descriptionLength));
	}

	return true;
}

bool CClip::SetDescFromType()
{
	INT_PTR size = m_Formats.GetSize();
	if (size <= 0)
	{
		return false;
	}

	int nCF_HDROPIndex = -1;
	for (int i = 0; i < size; i++)
	{
		if (m_Formats[i].m_cfType == CF_HDROP)
		{
			nCF_HDROPIndex = i;
		}
	}

	if (nCF_HDROPIndex >= 0)
	{
		using namespace nsPath;

		const std::vector<std::wstring> files = DittoCore::GlobalFileDrop::Read(m_Formats[nCF_HDROPIndex].m_hgData).Paths();
		const size_t nNumFiles = min(static_cast<size_t>(5), files.size());

		if (nNumFiles > 1)
			m_Desc = "Copied Files - ";
		else
			m_Desc = "Copied File - ";

		for (size_t nFile = 0; nFile < nNumFiles; nFile++)
		{
			CPath path(files[nFile].c_str());
			m_Desc += path.GetName();
			m_Desc += " - ";
			m_Desc += files[nFile].c_str();
			m_Desc += "\n";
		}
	}
	else
	{
		m_Desc = CClipboardFormats::GetFormatName(m_Formats[0].m_cfType);
	}

	return m_Desc.GetLength() > 0;
}

bool CClip::AddToDB(bool bCheckForDuplicates)
{
	// one lock from reading the newest order to writing the clip, so a clip saved by another
	// thread cannot get the same order in between
	const std::unique_lock<std::recursive_mutex> lock = m_context.Database().Lock();
	bool bResult;
	int removeStickySettingClipId{ -1 };
	try
	{
		m_Time = CTime::GetCurrentTime().GetTime();

		m_CRC = GenerateCRC();

		if (bCheckForDuplicates &&
			m_parentId < 0 &&
			MoveDuplicateToTop())
		{
			return true;
		}

		// inside the try: a failed order read stops the save (upstream saved with a default order)
		removeStickySettingClipId = ApplyAddToDbSticky();
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving the copied clip failed: %s"), e.errorMessage()));
		return false;
	}

	bResult = AddRowsInTransaction(removeStickySettingClipId);

	if (bResult)
	{
		const std::wstring& sound = SavePolicy().Settings().playSoundOnCopy;
		if (!sound.empty())
			PlaySound(sound.c_str(), NULL, SND_FILENAME | SND_ASYNC);
	}

	// should be emptied by AddToDataTable
	//ASSERT(m_Formats.GetSize() == 0);

	return bResult;
}

int CClip::ApplyAddToDbSticky()
{
	int removeStickySettingClipId = -1;

	if (m_addToDbStickyEnum == AddToDbStickyEnum::MAKE_TOP_STICKY)
	{
		m_stickyClipOrder = GetNewTopSticky(m_context, m_parentId, -1);
	}
	else if (m_addToDbStickyEnum == AddToDbStickyEnum::MAKE_LAST_STICKY)
	{
		m_stickyClipOrder = GetNewLastSticky(m_context, m_parentId, -1);
	}
	else if (m_addToDbStickyEnum == AddToDbStickyEnum::REPLACE_TOP_STICKY)
	{
		m_stickyClipOrder = GetNewTopSticky(m_context, m_parentId, -1);
		removeStickySettingClipId = GetExistingTopStickyClipId(m_context, m_parentId);
	}

	return removeStickySettingClipId;
}

bool CClip::MoveDuplicateToTop()
{
	int nID{ FindDuplicate() };
	if (nID < 0)
	{
		return false;
	}

	SetLatestOrders();

	// the duplicate moves to the top instead of a second copy being saved
	CClipRepository repository{ Repository(m_context) };
	repository.SetOrder(nID, CClipRepository::OrderColumn::Clip, m_clipOrder);
	if (m_parentId > -1)
	{
		repository.SetOrder(nID, CClipRepository::OrderColumn::ClipGroup, m_clipGroupOrder);
	}

	m_id = nID;

	CLogger::Log(CStringUtil::Format(_T("Found duplicate clip in db, Id: %d, ParentId: %d crc: %d, NewOrder: %f, GroupOrder %f"),
									 nID, m_parentId, m_CRC, m_clipOrder, m_clipGroupOrder));

	return true;
}

// if a duplicate exists, set recset to the duplicate and return true
// a failed query throws CppSQLite3Exception to AddToDB, which reports it and stops the save;
// returning -1 here would have saved a second copy of the clip
int CClip::FindDuplicate()
{
	// one read: the CRC and the id belong to the same clip
	const CLastAddedClip::Entry lastAdded{ m_context.LastAdded().Get() };
	switch (SavePolicy().DuplicateCheckFor(m_CRC, lastAdded.crc))
	{
	case DittoCore::DuplicateCheck::LastAdded:
		return lastAdded.id;
	case DittoCore::DuplicateCheck::AnyByCrc:
		return Repository(m_context).FindByCrc(m_CRC).value_or(-1);
	case DittoCore::DuplicateCheck::None:
		return -1;
	}
	throw std::logic_error("unknown duplicate check");
}


DWORD CClip::GenerateCRC()
{
	DittoCore::Crc32 crc;
	const bool adjust = SavePolicy().Settings().adjustForCrc;
	const CLIPFORMAT rtfFormat{ m_context.Formats().Rtf() };

	const INT_PTR size = m_Formats.GetSize();
	for (INT_PTR i = 0; i < size; i++)
	{
		const CClipFormat& format = m_Formats.ElementAt(i);
		if (format.m_hgData != NULL)
		{
			AddToCrc(crc, format, adjust, rtfFormat);
		}
	}
	return crc.Value();
}

void CClip::AddToCrc(DittoCore::Crc32& crc, const CClipFormat& format, bool adjust, CLIPFORMAT rtfFormat)
{
	const DittoCore::GlobalBytes block(format.m_hgData);
	std::span<const std::byte> bytes = block.Bytes();
	if (adjust && format.m_cfType == rtfFormat)
	{
		// In Word and Outlook the \datastore section and the rsid values change on every copy: leave them out
		const std::string normalized = DittoCore::RtfNormalizer::Normalize(DittoCore::ClipText::ReadAnsiBounded(bytes));
		crc.Add(std::as_bytes(std::span(normalized.data(), normalized.size())));
		return;
	}
	if (adjust)
	{
		// some programs put text in a block larger than the text: only the text counts
		bytes = TextBytesWithTerminator(format.m_cfType, bytes);
	}
	crc.Add(bytes);
}

std::span<const std::byte> CClip::TextBytesWithTerminator(CLIPFORMAT type, std::span<const std::byte> bytes)
{
	if (type == CF_TEXT)
	{
		const std::size_t length = DittoCore::ClipText::ReadAnsiBounded(bytes).size() + 1;
		return bytes.first((std::min)(bytes.size(), length));
	}
	if (type == CF_UNICODETEXT)
	{
		const std::size_t length = (DittoCore::ClipText::ReadWideBounded(bytes).size() + 1) * sizeof(wchar_t);
		return bytes.first((std::min)(bytes.size(), length));
	}
	return bytes;
}

bool CClip::AddRowsInTransaction(int removeStickySettingClipId)
{
	try
	{
		// all or nothing: upstream left a Main row without data when the data insert failed
		CDittoDbTransaction transaction(m_context.Database());
		if (AddToMainTable() == false || AddToDataTable() == false)
		{
			return false; // the transaction rolls back
		}
		if (removeStickySettingClipId > 0)
		{
			RemoveStickySetting(m_context, removeStickySettingClipId, m_parentId);
		}
		transaction.Commit();
		return true;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving the clip failed: %s"), e.errorMessage()));
		return false;
	}
}

// assigns m_ID
bool CClip::AddToMainTable()
{
	try
	{
		ClipRecord record = ToRecord();
		record.lastPasteDate = CTime::GetCurrentTime().GetTime();
		m_id = Repository(m_context).InsertClip(record);

		CLogger::Log(CStringUtil::Format(_T("Added clip to main table, Id: %d, ParentId: %d Desc: %s, Order: %f, GroupOrder: %f"), m_id, m_parentId, m_Desc.GetString(), m_clipOrder, m_clipGroupOrder));

		m_context.LastAdded().Record(m_CRC, m_id);
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Adding the clip to the database failed: %s"), e.errorMessage()));
		return false;
	}

	return true;
}

bool CClip::ModifyMainTable()
{
	bool bRet = false;
	try
	{
		Repository(m_context).UpdateClip(ToRecord());
		bRet = true;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving the changes to clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}

	return bRet;
}

bool CClip::ModifyDescription()
{
	bool bRet = false;
	try
	{
		Repository(m_context).UpdateDescription(m_id, m_Desc);
		bRet = true;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving the description of clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}

	return bRet;
}

// Empties m_Formats as it saves them to the Data Table.
bool CClip::AddToDataTable()
{
	try
	{
		// the last format is saved first, as upstream did, so the Data ids keep their order
		std::vector<FormatRecord> records;
		for (INT_PTR i = m_Formats.GetSize() - 1; i >= 0; i--)
		{
			const CClipFormat& format = m_Formats.ElementAt(i);
			const DittoCore::GlobalBytes block(format.m_hgData);
			FormatRecord record{};
			record.name = CClipboardFormats::GetFormatName(format.m_cfType);
			record.data.assign(block.Bytes().begin(), block.Bytes().end());
			records.push_back(std::move(record));
		}

		const std::vector<int> ids = Repository(m_context).InsertFormats(m_id, records);
		for (std::size_t r = 0; r < ids.size(); r++)
		{
			CClipFormat& format = m_Formats.ElementAt(m_Formats.GetSize() - 1 - static_cast<INT_PTR>(r));
			format.m_dataId = ids[r];
			CLogger::Log(CStringUtil::Format(_T("Added ClipData to DB, Id: %d, ParentId: %d Type: %s, size: %d"), ids[r], m_id, records[r].name.GetString(), static_cast<int>(records[r].data.size())));
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving the formats of clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}

	return true;
}

CClip::OrderSlot CClip::SlotFor(int parentId)
{
	OrderSlot slot{};
	const bool inGroup = parentId > -1;
	double& sticky = inGroup ? m_stickyClipGroupOrder : m_stickyClipOrder;
	slot.sticky = sticky != CClip::InvalidSticky;
	if (slot.sticky)
	{
		slot.column = inGroup ? CClipRepository::OrderColumn::StickyClipGroup : CClipRepository::OrderColumn::StickyClip;
		slot.order = &sticky;
	}
	else
	{
		slot.column = inGroup ? CClipRepository::OrderColumn::ClipGroup : CClipRepository::OrderColumn::Clip;
		slot.order = inGroup ? &m_clipGroupOrder : &m_clipOrder;
	}
	if (inGroup)
	{
		slot.parentId = parentId;
	}
	return slot;
}

bool CClip::Move(int parentId, bool up)
{
	// upstream had a copy of this for each list and kind of clip, and printed the orders into
	// the SQL with %f (6 decimals), so after a few midpoint moves the query found the clip itself
	const OrderSlot slot = SlotFor(parentId);
	return TryOrderStep([this, &slot, up]()
						{
		CClipRepository repository = Repository(m_context);
		const std::optional<double> neighbour = repository.NearestOrder(slot.column, slot.sticky, slot.parentId, *slot.order, up);
		if (neighbour)
		{
			const std::optional<double> beyond = repository.NearestOrder(slot.column, slot.sticky, slot.parentId, *neighbour, up);
			*slot.order = DittoCore::ClipOrder::MovedPast(*neighbour, beyond, up);
		} });
}

bool CClip::TryOrderStep(const std::function<void()>& step)
{
	try
	{
		step();
		return true;
	}
	catch (CppSQLite3Exception& e)
	{
		// the caller does not save the clip then; upstream saved it with an unchanged or default order
		CErrorReport::Show(CStringUtil::Format(_T("Reading the clip order for clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}
}

bool CClip::MoveUp(int parentId)
{
	return Move(parentId, true);
}

bool CClip::MoveDown(int parentId)
{
	return Move(parentId, false);
}

bool CClip::MakeStickyTop(int parentId)
{
	double& order = parentId < 0 ? m_stickyClipOrder : m_stickyClipGroupOrder;
	return TryOrderStep([this, &order, parentId]()
						{ order = GetNewTopSticky(m_context, parentId, m_id); });
}

bool CClip::MakeStickyLast(int parentId)
{
	double& order = parentId < 0 ? m_stickyClipOrder : m_stickyClipGroupOrder;
	return TryOrderStep([this, &order, parentId]()
						{ order = GetNewLastSticky(m_context, parentId, m_id); });
}

bool CClip::RemoveStickySetting(int parentId)
{
	bool reset = false;
	if (parentId < 0)
	{
		if (m_stickyClipOrder != CClip::InvalidSticky)
		{
			m_stickyClipOrder = CClip::InvalidSticky;
			reset = true;
		}
	}
	else
	{
		if (m_stickyClipGroupOrder != CClip::InvalidSticky)
		{
			m_stickyClipGroupOrder = CClip::InvalidSticky;
			reset = true;
		}
	}

	return reset;
}

bool CClip::RemoveStickySetting(CClipContext& context, int clipId, int parentId)
{
	// returns whether the clip's row was changed; upstream always returned false
	const CClipRepository::OrderColumn column = parentId < 0 ? CClipRepository::OrderColumn::StickyClip : CClipRepository::OrderColumn::StickyClipGroup;
	return Repository(context).SetOrder(clipId, column, static_cast<double>(CClip::InvalidSticky));
}

int CClip::GetExistingTopStickyClipId(CClipContext& context, int parentId)
{
	// a failed query throws: -1 is the "no top sticky clip" answer, so returning it after an error
	// (as before) let the save go on without clearing the old top sticky clip
	return Repository(context).TopStickyClipId(ParentFilter(parentId)).value_or(-1);
}

std::optional<double> CClip::EdgeOrder(CClipContext& context, CClipRepository::OrderColumn column, bool sticky, int parentId, bool highest)
{
	// a failed query throws to the caller's boundary; before, it was reported and treated as an
	// empty list, so the clip was saved with a default order
	return Repository(context).EdgeOrder(column, sticky, ParentFilter(parentId), highest);
}

std::optional<int> CClip::ParentFilter(int parentId)
{
	return parentId > -1 ? std::optional<int>(parentId) : std::nullopt;
}

CClipRepository CClip::Repository(CClipContext& context)
{
	return CClipRepository(context.Database());
}

ClipRecord CClip::ToRecord() const
{
	ClipRecord record{};
	record.id = m_id;
	record.parentId = m_parentId;
	record.description = m_Desc;
	record.time = m_Time.GetTime();
	record.shortCut = m_shortCut;
	record.dontAutoDelete = m_dontAutoDelete;
	record.crc = m_CRC;
	record.isGroup = m_bIsGroup;
	record.quickPaste = m_csQuickPaste;
	record.clipOrder = m_clipOrder;
	record.clipGroupOrder = m_clipGroupOrder;
	record.globalShortCut = m_globalShortCut;
	record.lastPasteDate = m_lastPasteDate.GetTime();
	record.stickyClipOrder = m_stickyClipOrder;
	record.stickyClipGroupOrder = m_stickyClipGroupOrder;
	record.moveToGroupShortCut = m_moveToGroupShortCut;
	record.globalMoveToGroupShortCut = m_globalMoveToGroupShortCut;
	return record;
}

void CClip::FromRecord(const ClipRecord& record)
{
	m_id = record.id;
	m_parentId = record.parentId;
	m_Desc = record.description;
	m_Time = record.time;
	m_shortCut = record.shortCut;
	m_dontAutoDelete = record.dontAutoDelete;
	m_CRC = record.crc;
	m_bIsGroup = record.isGroup;
	m_csQuickPaste = record.quickPaste;
	m_clipOrder = record.clipOrder;
	m_clipGroupOrder = record.clipGroupOrder;
	m_globalShortCut = record.globalShortCut;
	m_lastPasteDate = record.lastPasteDate;
	m_stickyClipOrder = record.stickyClipOrder;
	m_stickyClipGroupOrder = record.stickyClipGroupOrder;
	m_moveToGroupShortCut = record.moveToGroupShortCut;
	m_globalMoveToGroupShortCut = record.globalMoveToGroupShortCut;
}

double CClip::GetNewTopSticky(CClipContext& context, int parentId, int clipId)
{
	const std::optional<double> highest = EdgeOrder(context, parentId < 0 ? CClipRepository::OrderColumn::StickyClip : CClipRepository::OrderColumn::StickyClipGroup, true, parentId, true);
	const double newOrder = DittoCore::ClipOrder::TopSticky(highest);
	CLogger::Log(CStringUtil::Format(_T("GetNewTopSticky, Id: %d, parentId: %d, NewMax: %f"), clipId, parentId, newOrder));
	return newOrder;
}

double CClip::GetNewLastSticky(CClipContext& context, int parentId, int clipId)
{
	const std::optional<double> lowest = EdgeOrder(context, parentId < 0 ? CClipRepository::OrderColumn::StickyClip : CClipRepository::OrderColumn::StickyClipGroup, true, parentId, false);
	const double newOrder = DittoCore::ClipOrder::LastSticky(lowest);
	CLogger::Log(CStringUtil::Format(_T("GetNewLastSticky, Id: %d, parentId: %d, NewMin: %f"), clipId, parentId, newOrder));
	return newOrder;
}

bool CClip::MakeLatestOrder()
{
	return TryOrderStep([this]()
						{ m_clipOrder = GetNewOrder(m_context, -1, m_id); });
}

bool CClip::MakeLatestGroupOrder()
{
	if (m_parentId < 0)
	{
		return true;
	}
	return TryOrderStep([this]()
						{ m_clipGroupOrder = GetNewOrder(m_context, m_parentId, m_id); });
}

void CClip::SetLatestOrders()
{
	m_clipOrder = GetNewOrder(m_context, -1, m_id);
	if (m_parentId > -1)
	{
		m_clipGroupOrder = GetNewOrder(m_context, m_parentId, m_id);
	}
}

bool CClip::MakeLastOrder()
{
	return TryOrderStep([this]()
						{ m_clipOrder = GetNewLastOrder(-1, m_id); });
}

bool CClip::MakeLastGroupOrder()
{
	if (m_parentId < 0)
	{
		return true;
	}
	return TryOrderStep([this]()
						{ m_clipGroupOrder = GetNewLastOrder(m_parentId, m_id); });
}

double CClip::GetNewOrder(CClipContext& context, int parentId, int clipId)
{
	const std::optional<double> highest = EdgeOrder(context, parentId < 0 ? CClipRepository::OrderColumn::Clip : CClipRepository::OrderColumn::ClipGroup, false, parentId, true);
	const double newOrder = DittoCore::ClipOrder::Newest(highest);
	CLogger::Log(CStringUtil::Format(_T("GetNewOrder, Id: %d, parentId: %d, NewMax: %f"), clipId, parentId, newOrder));
	return newOrder;
}

double CClip::GetNewLastOrder(int parentId, int clipId)
{
	const std::optional<double> lowest = EdgeOrder(m_context, parentId < 0 ? CClipRepository::OrderColumn::Clip : CClipRepository::OrderColumn::ClipGroup, false, parentId, false);
	const double newOrder = DittoCore::ClipOrder::Oldest(lowest);
	CLogger::Log(CStringUtil::Format(_T("GetLastOrder, Id: %d, parentId: %d, NewMin: %f"), clipId, parentId, newOrder));
	return newOrder;
}

BOOL CClip::LoadMainTable(int id)
{
	try
	{
		const std::optional<ClipRecord> record = Repository(m_context).LoadClip(id);
		if (record)
		{
			FromRecord(*record);
			return TRUE;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading clip %d failed: %s"), id, e.errorMessage()));
		return FALSE;
	}

	return FALSE;
}

// STATICS

// Allocates a Global containing the requested Clip Format Data
HGLOBAL CClip::LoadFormat(CClipContext& context, int id, UINT cfType)
{
	try
	{
		const std::optional<std::vector<std::byte>> data = Repository(context).LoadFormat(id, CClipboardFormats::GetFormatName(static_cast<CLIPFORMAT>(cfType)));
		if (data)
		{
			return CGlobalMemory::NewGlobalP(const_cast<std::byte*>(data->data()), data->size());
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the %s format of clip %d failed: %s"), CClipboardFormats::GetFormatName(static_cast<CLIPFORMAT>(cfType)).GetString(), id, e.errorMessage()));
		return NULL;
	}

	// a missing format or one saved without data; upstream returned false as the handle
	return NULL;
}

bool CClip::LoadFormats(int id, bool bOnlyLoad_CF_TEXT, bool includeRichTextForTextOnly, int dataId)
{
	ULONGLONG startTick = GetTickCount64();
	m_Formats.RemoveAll();

	try
	{
		CClipRepository::FormatFilter filter{};
		if (bOnlyLoad_CF_TEXT)
		{
			filter.names = { _T("CF_TEXT"), _T("CF_UNICODETEXT"), _T("CF_HDROP") };
			if (includeRichTextForTextOnly)
			{
				filter.names.push_back(_T("Rich Text Format"));
			}
		}
		if (dataId >= 0)
		{
			filter.dataId = dataId;
		}

		// a format saved without data is left out (and logged) by the repository; upstream added
		// it with the previous format's handle, so two formats freed one block
		for (const FormatRecord& record : Repository(m_context).LoadFormats(id, filter))
		{
			CClipFormat cf;
			cf.m_dataId = record.dataId;
			cf.m_parentId = record.parentId;
			cf.m_cfType = CClipboardFormats::GetFormatID(record.name);
			cf.m_hgData = CGlobalMemory::NewGlobalP(const_cast<std::byte*>(record.data.data()), record.data.size());
			m_Formats.Add(cf);
			// m_Formats owns the data now
			cf.m_hgData = NULL;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the formats of clip %d failed: %s"), id, e.errorMessage()));
		return false;
	}

	ULONGLONG endTick = GetTickCount64();
	if ((endTick - startTick) > 150)
		CLogger::Log(CStringUtil::Format(_T("Paste Timing LoadFormats: %llu, ClipId: %d"), endTick - startTick, id));

	return m_Formats.GetSize() > 0;
}

void CClip::LoadTypes(CClipContext& context, int id, CClipTypes& types)
{
	types.RemoveAll();
	try
	{
		for (const CString& name : Repository(context).LoadFormatNames(id))
		{
			types.Add(CClipboardFormats::GetFormatID(name));
		}
	}
	catch (CppSQLite3Exception& e)
	{
		// the names are read in one query before any is added, so types stays empty
		CErrorReport::Show(CStringUtil::Format(_T("Loading the format list of clip %d failed: %s"), id, e.errorMessage()));
		return;
	}
}

CStringW CClip::GetUnicodeTextFormat()
{
	IClipFormat* pFormat = this->Clips()->FindFormatEx(CF_UNICODETEXT);
	if (pFormat != NULL)
	{
		return pFormat->GetAsCString();
	}

	return _T("");
}

CStringA CClip::GetCFTextTextFormat()
{
	IClipFormat* pFormat = this->Clips()->FindFormatEx(CF_TEXT);
	if (pFormat != NULL)
	{
		return pFormat->GetAsCStringA();
	}

	return _T("");
}

CStringA CClip::GetRTFTextFormat()
{
	IClipFormat* pFormat = this->Clips()->FindFormatEx(m_context.Formats().Rtf());
	if (pFormat != NULL)
	{
		return pFormat->GetAsCStringA();
	}

	return _T("");
}

BOOL CClip::ContainsClipFormat(CLIPFORMAT clipFormat)
{
	return this->Clips()->FindFormatEx(clipFormat) != NULL;
}

BOOL CClip::WriteTextToFile(CString path, BOOL unicode, BOOL asci, BOOL rtf, BOOL forceUnicode, BOOL utf8)
{
	BOOL ret = false;

	CFile f;
	if (f.Open(path, CFile::modeWrite | CFile::modeCreate))
	{
		try
		{
			ret = WriteTextFormat(f, unicode, asci, rtf, forceUnicode, utf8) ? TRUE : FALSE;
			f.Close();
		}
		catch (CFileException* e)
		{
			// a failed write is a FALSE result; upstream let the MFC exception escape to the caller
			TCHAR cause[255]{};
			e->GetErrorMessage(cause, _countof(cause));
			e->Delete();
			f.Abort();
			CLogger::Log(CStringUtil::Format(_T("Writing the clip text to %s failed: %s"), path.GetString(), cause));
			return FALSE;
		}
	}

	return ret;
}

bool CClip::WriteTextFormat(CFile& f, BOOL unicode, BOOL asci, BOOL rtf, BOOL forceUnicode, BOOL utf8)
{
	CStringW w = GetUnicodeTextFormat();
	CStringA a = GetCFTextTextFormat();
	CStringA rtfA = GetRTFTextFormat();

	if (utf8 && w != _T(""))
	{
		CStringA utf8Data = CTextConvert::UnicodeToUTF8(w);
		f.Write(utf8Data.GetBuffer(), utf8Data.GetLength());

		return true;
	}

	if (unicode && (w != _T("") || forceUnicode))
	{
		std::byte header[2];
		header[0] = (std::byte)0xFF;
		header[1] = (std::byte)0xFE;
		f.Write(&header, 2);
		f.Write(w.GetBuffer(), w.GetLength() * sizeof(wchar_t));

		return true;
	}

	return WriteAnsiTextFormat(f, a, rtfA, asci, rtf);
}

bool CClip::WriteAnsiTextFormat(CFile& f, CStringA& a, CStringA& rtfA, BOOL asci, BOOL rtf)
{
	if (asci && a != _T(""))
	{
		f.Write(a.GetBuffer(), a.GetLength());

		return true;
	}

	if (rtf && rtfA != _T(""))
	{
		f.Write(rtfA.GetBuffer(), rtfA.GetLength());

		return true;
	}

	return false;
}

BOOL CClip::SaveFormats(CString* unicode, CStringA* asci, CStringA* rtf, BOOL updateDescription, std::vector<BYTE>* cf_dibBytes, std::vector<BYTE>* pngBytes)
{
	ARRAY deletedData;
	for (INT_PTR i = m_Formats.GetSize() - 1; i >= 0; i--)
	{
		deletedData.Add(m_Formats[i].m_dataId);
	}

	EmptyFormats();

	AddImageFormats(cf_dibBytes, pngBytes);

	if (rtf != nullptr)
	{
		const int nLength = rtf->GetLength() + sizeof(char);
		AddFormat(m_context.Formats().Rtf(), rtf->GetBuffer(nLength), nLength, true);
	}

	if (asci != nullptr)
	{
		const int nLength = asci->GetLength() + sizeof(char);
		AddFormat(CF_TEXT, asci->GetBuffer(nLength), nLength, true);
	}

	if (unicode != nullptr)
	{
		const int nLength = unicode->GetLength() * sizeof(wchar_t) + sizeof(wchar_t);
		AddFormat(CF_UNICODETEXT, unicode->GetBuffer(nLength), nLength, true);
	}

	return SaveFormatsInTransaction(deletedData, updateDescription) ? TRUE : FALSE;
}

void CClip::AddImageFormats(std::vector<BYTE>* cf_dibBytes, std::vector<BYTE>* pngBytes)
{
	if (cf_dibBytes != nullptr && cf_dibBytes->size() > 0)
	{
		AddFormat(CF_DIB, cf_dibBytes->data(), cf_dibBytes->size(), false);
	}

	if (pngBytes != nullptr && pngBytes->size() > 0)
	{
		AddFormat(m_context.Formats().Png(), pngBytes->data(), pngBytes->size(), false);
	}
}

bool CClip::SaveFormatsInTransaction(const ARRAY& deletedData, BOOL updateDescription)
{
	try
	{
		m_CRC = GenerateCRC();

		// rolled back when a step fails; upstream's manual begin stayed open after an exception
		// and ignored the steps' results, so a failed save was committed in part
		CDittoDbTransaction transaction(m_context.Database());

		CClipRepository repository = Repository(m_context);
		// CArrayEx indexes with int
		const int count = static_cast<int>(deletedData.GetSize());
		for (int i = 0; i < count; i++)
		{
			repository.DeleteFormat(deletedData.GetAt(i));
		}

		if (SaveMainRow(updateDescription) == false || AddToDataTable() == false)
		{
			return false; // the transaction rolls back
		}

		transaction.Commit();
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving the formats of clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}

	return true;
}

bool CClip::SaveMainRow(BOOL updateDescription)
{
	if (m_id < 0)
	{
		SetLatestOrders();
		return AddToMainTable();
	}
	return updateDescription ? ModifyDescription() : true;
}

BOOL CClip::WriteImageToFile(CString path)
{
	CClipFormat* bitmap = this->m_Formats.FindFormat(CF_DIB);
	CClipFormat* png = this->m_Formats.FindFormat(m_context.Formats().Png());
	if (!bitmap && !png) return false;

	std::shared_ptr<CImage> i;
	// png is more closer to original
	if (png)
		i = PNGImageHelper::CImageFromHGLOBAL(png->m_hgData);
	else
		i = DIBImageHelper::CImageFromHGLOBAL(bitmap->m_hgData);
	if (!i)
		return false;

	return i->Save(path) == S_OK;
}

bool CClip::WriteImageToFileOrReport(const CString& path, const CString& operation)
{
	try
	{
		if (WriteImageToFile(path) == FALSE)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Ditto cannot %s the clip's image to %s: the clip has no image, the image cannot be read or the file cannot be written."), operation.GetString(), path.GetString()));
			return false;
		}
		return true;
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Ditto cannot %s the clip's image to %s: the image data is malformed (%s)."), operation.GetString(), path.GetString(), CString(error.what()).GetString()));
		return false;
	}
}

bool CClip::ReadFileContents(const CString& path, ULONGLONG maxSize, CopiedFile& file, CString& errorMessage)
{
	CFile source;
	CFileException ex;
	if (!source.Open(path, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone, &ex))
	{
		TCHAR szError[200]{};
		ex.GetErrorMessage(szError, _countof(szError));
		errorMessage += CStringUtil::Format(_T("Error opening file: %s, Error: %s\r\n"), path.GetString(), szError);
		return false;
	}

	const ULONGLONG fileSize = source.GetLength();
	if (fileSize >= maxSize)
	{
		TCHAR szFileSize[64]{};
		TCHAR szMaxFileSize[64]{};
		StrFormatByteSize((LONGLONG)fileSize, szFileSize, _countof(szFileSize));
		StrFormatByteSize((LONGLONG)maxSize, szMaxFileSize, _countof(szMaxFileSize));
		errorMessage += CStringUtil::Format(_T("File is to large: %s, Size: %s, Max Size: %s\r\n"), path.GetString(), szFileSize, szMaxFileSize);
		return false;
	}

	// fileSize is below the int-sized maximum, so it fits UINT and int
	file.contents.resize(static_cast<size_t>(fileSize));
	const UINT read = source.Read(file.contents.data(), static_cast<UINT>(fileSize));
	if (read != fileSize)
	{
		errorMessage += CStringUtil::Format(_T("Error reading file: %s, read %u of %I64u bytes\r\n"), path.GetString(), read, fileSize);
		return false;
	}

	file.md5 = DittoCore::Md5::Hex(file.contents);
	const CStringA utf8Path = CTextConvert::UnicodeToUTF8(path);
	file.path.assign(utf8Path.GetString(), utf8Path.GetLength());

	CLogger::Log(CStringUtil::Format(_T("Saving file contents to Ditto Database, file: %s, size: %I64u, md5: %S"), path.GetString(), fileSize, file.md5.c_str()));
	return true;
}

bool CClip::AddFileDataToData(CString& errorMessage)
{
	INT_PTR size = m_Formats.GetSize();
	if (size <= 0)
	{
		errorMessage = _T("No CF_HDROP formats to convert");
		return false;
	}

	bool addedFileData = false;

	const FileDataIndexes indexes{ FindFileDataIndexes(size) };
	const int nCF_HDROPIndex{ indexes.hdrop };
	const int dittoDataIndex{ indexes.dittoData };

	if (nCF_HDROPIndex < 0)
	{
		errorMessage = _T("No CF_HDROP formats to convert");
		return false;
	}
	else if (dittoDataIndex >= 0)
	{
		return false;
	}

	using namespace nsPath;

	std::vector<std::wstring> files;
	try
	{
		files = DittoCore::GlobalFileDrop::Read(m_Formats[nCF_HDROPIndex].m_hgData).Paths();
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		// this function reports through errorMessage; the caller shows it
		errorMessage += CStringUtil::Format(_T("The clip's file list is malformed (%s)\r\n"), CString(error.what()).GetString());
		return false;
	}

	CString newDesc = _T("File Contents - ");
	std::vector<CopiedFile> copied;
	ReadDroppedFiles(files, copied, newDesc, errorMessage);

	if (copied.empty())
		return false;

	if (!AddFileDataRecord(copied, errorMessage))
	{
		return false;
	}
	addedFileData = true;

	// AddFormat appended the record after the clip's existing formats, which are already in the
	// database; drop them so AddToDataTable saves only the record. Upstream removed index i of a
	// shrinking array, which skipped formats and removed file data instead.
	this->m_Formats.RemoveAt(0, size);

	this->m_Desc = newDesc;

	// a failed save is not reported as added (the caller would refresh the clip as saved)
	return addedFileData && SaveFileDataToDatabase();
}

CClip::FileDataIndexes CClip::FindFileDataIndexes(INT_PTR size)
{
	FileDataIndexes indexes{};
	for (int i = 0; i < size; i++)
	{
		if (m_Formats[i].m_cfType == CF_HDROP)
		{
			indexes.hdrop = i;
		}
		else if (m_Formats[i].m_cfType == m_context.Formats().DittoFileData())
		{
			indexes.dittoData = i;
		}
	}
	return indexes;
}

void CClip::ReadDroppedFiles(const std::vector<std::wstring>& files, std::vector<CopiedFile>& copied, CString& newDesc, CString& errorMessage)
{
	const ULONGLONG maxSize = SavePolicy().Settings().maxFileContentsSize;
	for (const std::wstring& path : files)
	{
		CopiedFile file{};
		if (!ReadFileContents(path.c_str(), maxSize, file, errorMessage))
			continue;

		copied.push_back(std::move(file));
		newDesc += path.c_str();
		newDesc += _T("\n");
	}
}

bool CClip::AddFileDataRecord(const std::vector<CopiedFile>& copied, CString& errorMessage)
{
	// one version-2 record holds every file; upstream added one format per file, and each
	// replaced the one before, so only the last file was kept
	std::vector<DittoCore::FileDataEntry> entries;
	for (const CopiedFile& file : copied)
	{
		entries.push_back(DittoCore::FileDataEntry{ file.path, file.md5, file.contents });
	}
	const std::vector<std::byte> record = DittoCore::FileDataRecord::Build(entries);
	if (record.size() > UINT_MAX)
	{
		errorMessage += _T("The files are too large to save together\r\n");
		return false;
	}
	AddFormat(m_context.Formats().DittoFileData(), const_cast<std::byte*>(record.data()), static_cast<UINT>(record.size()));
	return true;
}

bool CClip::SaveFileDataToDatabase()
{
	// each step shows its own error (upstream also added a second, vaguer message to the popup)
	return this->ModifyDescription() && this->AddToDataTable();
}

std::unique_ptr<Gdiplus::Bitmap> CClip::CreateGdiplusBitmap()
{
	const CLIPFORMAT pngFormat{ m_context.Formats().Png() };
	CClipFormat* png = this->m_Formats.FindFormat(pngFormat);
	if (png != NULL)
		return png->LoadGdiplusBitmap(pngFormat);

	CClipFormat* dib = this->m_Formats.FindFormat(CF_DIB);
	if (dib != NULL)
		return dib->LoadGdiplusBitmap(pngFormat);

	return nullptr;
}

bool CClip::SaveFromEditWnd(BOOL bUpdateDesc)
{
	bool bRet = false;

	try
	{
		// one transaction: upstream deleted the old data first, so a failure while writing the
		// new data lost the clip's contents
		CDittoDbTransaction transaction(m_context.Database());
		CClipRepository repository = Repository(m_context);
		repository.DeleteFormats(m_id);

		DWORD CRC = GenerateCRC();

		if (AddToDataTable() == false)
		{
			return false; // the transaction rolls back
		}

		repository.UpdateCrc(m_id, CRC);

		if (bUpdateDesc)
		{
			repository.UpdateDescription(m_id, m_Desc);
		}

		transaction.Commit();
		bRet = true;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Saving the edited clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}

	return bRet;
}

/*----------------------------------------------------------------------------*\
CClipList
\*----------------------------------------------------------------------------*/

void CClipList::Add(std::unique_ptr<CClip> clip)
{
	ASSERT(clip);
	m_clips.push_back(std::move(clip));
}

CClipList CClipList::TakeAll()
{
	CClipList taken{};
	taken.m_clips.swap(m_clips);
	m_lastSaved = nullptr;
	return taken;
}

CClip& CClipList::Last()
{
	ASSERT(!m_clips.empty());
	return *m_clips.back();
}

// returns the number of clips actually saved
// while this does empty the Format Data, it does not delete the Clips.
int CClipList::AddToDB(bool bLatestOrder)
{
	CLogger::Log(_T("AddToDB - Start"));

	int savedCount{ 0 };
	bool bResult{ false };
	m_lastSaved = nullptr;

	INT_PTR remaining{ static_cast<INT_PTR>(m_clips.size()) };
	for (const std::unique_ptr<CClip>& clip : m_clips)
	{
		CLogger::Log(CStringUtil::Format(_T("AddToDB - while(pos), Start Remaining %d"), remaining));
		remaining--;

		CClip* pClip{ clip.get() };
		ASSERT(pClip);

		// a failed order read stops the saving (shown); upstream saved the clip with a default order
		if (bLatestOrder && (pClip->MakeLatestOrder() == false || pClip->MakeLatestGroupOrder() == false))
		{
			break;
		}

		bResult = pClip->AddToDB();
		if (bResult)
		{
			savedCount++;
			m_lastSaved = pClip;
		}

		CLogger::Log(CStringUtil::Format(_T("AddToDB - while(pos), End Remaining %d, save count: %d"), remaining, savedCount));
	}

	CLogger::Log(CStringUtil::Format(_T("AddToDB - Start, count: %d"), savedCount));

	return savedCount;
}

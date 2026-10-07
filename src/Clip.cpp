// Clip.cpp: implementations of the Clip interfaces
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CP_Main.h"
#include "Clip.h"
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

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


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
		return GlobalFree(hGlobal);   // returns NULL
	}
	return hGlobal;
}

HGLOBAL COleDataObjectEx::GetGlobalData(CLIPFORMAT cfFormat, LPFORMATETC lpFormatEtc)
{
    HGLOBAL hGlobal = COleDataObject::GetGlobalData(cfFormat, lpFormatEtc);
	if(hGlobal)
	{
		if(!::IsValid(hGlobal))
		{
			Log( StrF(
				_T("COleDataObjectEx::GetGlobalData(\"%s\"): ERROR: Invalid (NULL) data returned."),
				GetFormatName(cfFormat).GetString() ) );
			::GlobalFree( hGlobal );
			hGlobal = NULL;
		}
		return hGlobal;
	}
	
	// The data isn't in global memory, so try getting an IStream interface to it.
	STGMEDIUM stg;
	
	if(!GetData(cfFormat, &stg))
	{
		return 0;
	}
	
	switch(stg.tymed)
	{
	case TYMED_HGLOBAL:
		hGlobal = stg.hGlobal;
		break;
		
	case TYMED_ISTREAM:
		hGlobal = StreamToGlobal(stg.pstm);
		break;
	} // end switch
	
	ReleaseStgMedium(&stg);
	
	if(hGlobal && !::IsValid(hGlobal))
	{
		Log( StrF(
			_T("COleDataObjectEx::GetGlobalData(\"%s\"): ERROR: Invalid (NULL) data returned."),
			GetFormatName(cfFormat).GetString()));
		::GlobalFree(hGlobal);
		hGlobal = NULL;
	}
	
	return hGlobal;
}

std::shared_ptr<CClipTypes> COleDataObjectEx::GetAvailableTypes()
{
	std::shared_ptr<CClipTypes> types = std::make_shared<CClipTypes>();

	// GetNextFormat API has a bug that cannot find avaliable formats correctly. (ex. CF_DIB)
	// So, Use EnumClipboardFormats API.
	if (!OpenClipboard(theApp.m_MainhWnd))
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
	if(m_autoDeleteData && m_hgData)
	{
		m_hgData = ::GlobalFree( m_hgData );
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

Gdiplus::Bitmap *CClipFormat::CreateGdiplusBitmap()
{
	return LoadGdiplusBitmap().release(); // ownership: the add-in caller (IClipFormat's raw-pointer ABI)
}

std::unique_ptr<Gdiplus::Bitmap> CClipFormat::LoadGdiplusBitmap()
{
	if (this->m_cfType != CF_DIB && this->m_cfType != theApp.m_PNG_Format)
		return nullptr;

	if (this->m_cfType == theApp.m_PNG_Format)
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

	for(int i=0; i < count; i++)
	{
		pCF = &ElementAt(i);
		if(pCF->m_cfType == cfType)
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

DWORD CClip::m_LastAddedCRC = 0;
int CClip::m_lastAddedID = -1;

CClip::CClip() : 
	m_id(-1), 
	m_CRC(0),
	m_parentId(-1),
	m_dontAutoDelete(FALSE),
	m_shortCut(0),
	m_bIsGroup(FALSE),
	m_param1(0),
	m_clipOrder(0),
	m_stickyClipOrder(INVALID_STICKY),
	m_stickyClipGroupOrder(INVALID_STICKY),
	m_clipGroupOrder(0),
	m_globalShortCut(FALSE),
	m_moveToGroupShortCut(0),
	m_globalMoveToGroupShortCut(FALSE)
{
	m_copyReason = CopyReasonEnum::COPY_TO_UNKOWN;
	m_addToDbStickyEnum = AddToDbStickyEnum::INVALID;
}

CClip::CClip(DittoCore::ClipSavePolicy savePolicy) :
	CClip()
{
	m_savePolicy.emplace(std::move(savePolicy));
}

const DittoCore::ClipSavePolicy& CClip::SavePolicy()
{
	if (!m_savePolicy.has_value())
	{
		m_savePolicy.emplace(CGetSetOptions::GetClipSaveSettings());
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

const CClip& CClip::operator=(const CClip &clip)
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
	
	for(int i = 0; i < nCount; i++)
	{
		pCF = &clip.m_Formats.GetData()[i];

		LPVOID pvData = GlobalLock(pCF->m_hgData);
		if(pvData)
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
	for(INT_PTR i = m_Formats.GetSize()-1; i >= 0; i--)
	{
		m_Formats[i].Free();
		m_Formats.RemoveAt(i);
	}
}

// Adds a new Format to this Clip by copying the given data.
bool CClip::AddFormat(CLIPFORMAT cfType, void* pData, SIZE_T nLen, bool setDesc)
{
	ASSERT(pData && nLen);
	HGLOBAL hGlobal = ::NewGlobalP(pData, nLen);
	ASSERT(hGlobal);

	// update the Clip statistics
	m_Time = m_Time.GetCurrentTime();

	if (setDesc)
	{
		if (cfType != CF_UNICODETEXT || !SetDescFromText(hGlobal, true))
			SetDescFromType();
	}
	
	CClipFormat format(cfType,hGlobal);
	CClipFormat *pFormat;
	
	pFormat = m_Formats.FindFormat(cfType);
	// if the format type already exists as part of this clip, replace the data
	if(pFormat)
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
	if(pClipTypes == NULL || pClipTypes->GetSize() == 0)
	{
		ASSERT(0); // this feature is not currently used... it is an error if it is.
		Log(_T("no types were given to accept, skipping this clipboard change"));
		return FALSE;
	}

	COleDataObjectEx oleData;
	CClipTypes* pTypes = pClipTypes;

	// m_Formats should be empty when this is called.
	ASSERT(m_Formats.GetSize() == 0);

	// If the data is supposed to be private, then return
	if (::IsClipboardFormatAvailable(theApp.m_cfIgnoreClipboard))
	{
		Log(_T("Clipboard ignore type is on the clipboard, skipping this clipboard change"));
		return FALSE;
	}
	
	if (SavePolicy().Settings().enforceIgnoreFormats)
	{
		//https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
		if (::IsClipboardFormatAvailable(theApp.m_excludeClipboardContentFromMonitorProcessing))
		{
			Log(_T("ExcludeClipboardContentFromMonitorProcessing type is on the clipboard, skipping this clipboard change"));
			return FALSE;
		}
	}

	//If we are saving a multi paste then delay us connecting to the clipboard
	//to allow the ctrl-v to do a paste
	if(::IsClipboardFormatAvailable(theApp.m_cfDelaySavingData))
	{
		Log(_T("Delay clipboard type is on the clipboard, delaying 1500 ms to allow ctrl-v to work"));
		Sleep(1500);
	}

	//Attach to the clipboard
	if(!oleData.AttachClipboard())
	{
		Log(_T("failed to attache to clipboard, skipping this clipboard change"));
		ASSERT(0); // does this ever happen?
		return FALSE;
	}
	
	oleData.EnsureClipboardObject();

	//https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
	if (SavePolicy().Settings().enforceIgnoreFormats &&
		oleData.IsDataAvailable(theApp.m_canIncludeInClipboardHistory))
	{
		HGLOBAL includeInHistory = oleData.GetGlobalData(theApp.m_canIncludeInClipboardHistory);
		if (includeInHistory != nullptr && TakeDword(includeInHistory) == 0)
		{
			Log(_T("CanIncludeInClipboardHistory is 0, skipping this clipboard change"));
			oleData.Release();
			return FALSE;
		}
	}
		
	m_Desc = "[Ditto Error] BAD DESCRIPTION";
	
	// Get Description String
	// NOTE: We make sure that the description always corresponds to the
	//  data saved by using the exact same globalmem instance as the source
	//  for both... i.e. we only fetch the description format type once.
	CClipFormat cfDesc;
	bool bIsDescSet = false;

	cfDesc.m_cfType = CF_UNICODETEXT;	
	if(oleData.IsDataAvailable(cfDesc.m_cfType))
	{
		for (int i = 0; i < 10; i++)
		{
			cfDesc.m_hgData = oleData.GetGlobalData(cfDesc.m_cfType);
			if (cfDesc.m_hgData == NULL)
			{
				Log(StrF(_T("Tried to set description from cf_unicode, data is NULL, try: %d"), i+1));
			}
			else
			{
				break;
			}
			Sleep(10);
		}
		bIsDescSet = SetDescFromText(cfDesc.m_hgData, true);

		Log(StrF(_T("Tried to set description from cf_unicode text, Set: %d, Desc: [%s]"), bIsDescSet, m_Desc.Left(30).GetString()));
	}

	if(bIsDescSet == false)
	{
		cfDesc.m_cfType = CF_TEXT;	
		if(oleData.IsDataAvailable(cfDesc.m_cfType))
		{
			for (int i = 0; i < 10; i++)
			{
				cfDesc.m_hgData = oleData.GetGlobalData(cfDesc.m_cfType);
				if (cfDesc.m_hgData == NULL)
				{
					Log(StrF(_T("Tried to set description from cf_text, data is NULL, try: %d"), i + 1));
				}
				else
				{
					break;
				}
				Sleep(10);
			}

			bIsDescSet = SetDescFromText(cfDesc.m_hgData, false);

			Log(StrF(_T("Tried to set description from cf_text text, Set: %d, Desc: [%s]"), bIsDescSet, m_Desc.Left(30).GetString()));
		}
	}

	INT_PTR nSize;
	CClipFormat cf;
	INT_PTR numTypes = pTypes->GetSize();

	Log(StrF(_T("Begin enumerating over supported types, Count: %d"), numTypes));

	for(int i = 0; i < numTypes; i++)
	{
		cf.m_cfType = pTypes->ElementAt(i);

		if (cf.m_cfType == CF_DIB &&
			oleData.IsDataAvailable(CF_TEXT) &&
			SavePolicy().IgnoresDibFrom(std::wstring(activeApp.MakeLower().GetString())))
		{
			Log(StrF(_T("Ignore CF_DIB from %s"), activeApp.GetString()));
			continue;
		}

		BOOL bSuccess = false;
		Log(StrF(_T("Begin try and load type %s"), GetFormatName(cf.m_cfType).GetString()));
		
		// is this the description we already fetched?
		if(cf.m_cfType == cfDesc.m_cfType)
		{
			cf = cfDesc;
			cfDesc.m_hgData = 0; // cf owns it now (to go into m_Formats)
		}
		else if(!oleData.IsDataAvailable(cf.m_cfType))
		{
			Log(StrF(_T("End of load - Data is not available for type %s"), GetFormatName(cf.m_cfType).GetString()));
			continue;
		}
		else
		{
			for (int tries = 0; tries < 2; tries++)
			{
				cf.m_hgData = oleData.GetGlobalData(cf.m_cfType);
				if (cf.m_hgData != NULL)
					break;

				Log(StrF(_T("Tried to get data for type: %s, data is NULL, try: %d"), GetFormatName(cf.m_cfType).GetString(), tries + 1));
				Sleep(5);
			}
		}
		
		if(cf.m_hgData)
		{
			nSize = GlobalSize(cf.m_hgData);
			if(nSize > 0)
			{
				if(SavePolicy().TooLarge(static_cast<std::uint64_t>(nSize)))
				{
					CString cs;
					cs.Format(_T("Maximum clip size reached max size = %lld, clip size = %Id"), SavePolicy().Settings().maxClipSizeInBytes, nSize);
					Log(cs);

					oleData.Release();
					return -1;
				}

				ASSERT(::IsValid(cf.m_hgData));
				
				m_Formats.Add(cf);
				bSuccess = true;
			}
			else
			{
				ASSERT(FALSE); // a valid GlobalMem with 0 size is strange
				cf.Free();
				Log(StrF(_T("Data length is 0 for type %s"), GetFormatName(cf.m_cfType).GetString()));
			}
			cf.m_hgData = 0; // m_Formats owns it now
		}

		Log(StrF(_T("End of load - type %s, Success: %d"), GetFormatName(cf.m_cfType).GetString(), bSuccess));
	}

	Log(StrF(_T("End enumerating over supported types, Count: %d"), numTypes));
	
	m_Time = CTime::GetCurrentTime();
			
	if(!bIsDescSet)
	{
		SetDescFromType();

		Log(StrF(_T("Setting description from type, Desc: [%s]"), m_Desc.Left(30).GetString()));
	}
	
	// if the description was in a type that is not supported,
	//we have to free it since it wasn't added to m_Formats
	if(cfDesc.m_hgData)
	{
		cfDesc.Free();
	}
	
	oleData.Release();
	
	if(m_Formats.GetSize() == 0)
	{
		Log(_T("No clip types were in supported types array"));
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
	if(hgData == 0)
		return false;
	
	const DittoCore::GlobalBytes bytes(hgData);
	if(unicode)
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
	if(static_cast<std::size_t>(m_Desc.GetLength()) > descriptionLength)
	{
		m_Desc = m_Desc.Left(static_cast<int>(descriptionLength));
	}

	return true;
}

bool CClip::SetDescFromType()
{
	INT_PTR size = m_Formats.GetSize();
	if(size <= 0)
	{
		return false;
	}

	int nCF_HDROPIndex = -1;
	for(int i = 0; i < size; i++)
	{
		if(m_Formats[i].m_cfType == CF_HDROP)
		{
			nCF_HDROPIndex = i;
		}
	}

	if(nCF_HDROPIndex >= 0)
	{
		using namespace nsPath;

		const std::vector<std::wstring> files = DittoCore::GlobalFileDrop::Read(m_Formats[nCF_HDROPIndex].m_hgData).Paths();
		const size_t nNumFiles = min(static_cast<size_t>(5), files.size());

		if(nNumFiles > 1)
			m_Desc = "Copied Files - ";
		else
			m_Desc = "Copied File - ";

		for(size_t nFile = 0; nFile < nNumFiles; nFile++)
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
		m_Desc = GetFormatName(m_Formats[0].m_cfType);
	}

	return m_Desc.GetLength() > 0;
}

bool CClip::AddToDB(bool bCheckForDuplicates)
{
	// one lock from reading the newest order to writing the clip, so a clip saved by another
	// thread cannot get the same order in between
	const std::unique_lock<std::recursive_mutex> lock = theApp.m_db.Lock();
	bool bResult;
	try
	{
		m_Time = CTime::GetCurrentTime().GetTime();

		m_CRC = GenerateCRC();

		if(bCheckForDuplicates &&
			m_parentId < 0 &&
			MoveDuplicateToTop())
		{
			return true;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Saving the copied clip failed: %s"), e.errorMessage()));
		return false;
	}

	int removeStickySettingClipId = -1;

	if (m_addToDbStickyEnum == AddToDbStickyEnum::MAKE_TOP_STICKY)
	{
		m_stickyClipOrder = this->GetNewTopSticky(m_parentId, -1);
	}
	else if (m_addToDbStickyEnum == AddToDbStickyEnum::MAKE_LAST_STICKY)
	{
		m_stickyClipOrder = this->GetNewLastSticky(m_parentId, -1);
	}
	else if (m_addToDbStickyEnum == AddToDbStickyEnum::REPLACE_TOP_STICKY)
	{
		m_stickyClipOrder = this->GetNewTopSticky(m_parentId, -1);
		removeStickySettingClipId = GetExistingTopStickyClipId(m_parentId);
	}
	
	bResult = AddRowsInTransaction(removeStickySettingClipId);

	if(bResult)
	{
		const std::wstring& sound = SavePolicy().Settings().playSoundOnCopy;
		if(!sound.empty())
			PlaySound(sound.c_str(), NULL, SND_FILENAME|SND_ASYNC);
	}
	
	// should be emptied by AddToDataTable
	//ASSERT(m_Formats.GetSize() == 0);
	
	return bResult;
}

bool CClip::MoveDuplicateToTop()
{
	int nID{FindDuplicate()};
	if(nID < 0)
	{
		return false;
	}

	MakeLatestOrder();
	MakeLatestGroupOrder();

	// the duplicate moves to the top instead of a second copy being saved
	CClipRepository repository{Repository()};
	repository.SetOrder(nID, CClipRepository::OrderColumn::Clip, m_clipOrder);
	if(m_parentId > -1)
	{
		repository.SetOrder(nID, CClipRepository::OrderColumn::ClipGroup, m_clipGroupOrder);
	}

	m_id = nID;

	Log(StrF(_T("Found duplicate clip in db, Id: %d, ParentId: %d crc: %d, NewOrder: %f, GroupOrder %f"),
							nID, m_parentId, m_CRC, m_clipOrder, m_clipGroupOrder));

	return true;
}

// if a duplicate exists, set recset to the duplicate and return true
// a failed query throws CppSQLite3Exception to AddToDB, which reports it and stops the save;
// returning -1 here would have saved a second copy of the clip
int CClip::FindDuplicate()
{
	switch (SavePolicy().DuplicateCheckFor(m_CRC, m_LastAddedCRC))
	{
	case DittoCore::DuplicateCheck::LastAdded:
		return m_lastAddedID;
	case DittoCore::DuplicateCheck::AnyByCrc:
		return Repository().FindByCrc(m_CRC).value_or(-1);
	case DittoCore::DuplicateCheck::None:
		return -1;
	}
	throw std::logic_error("unknown duplicate check");
}



DWORD CClip::GenerateCRC()
{
	DittoCore::Crc32 crc;
	const bool adjust = SavePolicy().Settings().adjustForCrc;

	const INT_PTR size = m_Formats.GetSize();
	for (INT_PTR i = 0; i < size; i++)
	{
		const CClipFormat& format = m_Formats.ElementAt(i);
		if (format.m_hgData != NULL)
		{
			AddToCrc(crc, format, adjust);
		}
	}
	return crc.Value();
}

void CClip::AddToCrc(DittoCore::Crc32& crc, const CClipFormat& format, bool adjust)
{
	const DittoCore::GlobalBytes block(format.m_hgData);
	std::span<const std::byte> bytes = block.Bytes();
	if (adjust && format.m_cfType == theApp.m_RTFFormat)
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
		CDittoDbTransaction transaction(theApp.m_db);
		if (AddToMainTable() == false || AddToDataTable() == false)
		{
			return false;   // the transaction rolls back
		}
		if (removeStickySettingClipId > 0)
		{
			RemoveStickySetting(removeStickySettingClipId, m_parentId);
		}
		transaction.Commit();
		return true;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Saving the clip failed: %s"), e.errorMessage()));
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
		m_id = Repository().InsertClip(record);

		Log(StrF(_T("Added clip to main table, Id: %d, ParentId: %d Desc: %s, Order: %f, GroupOrder: %f"), m_id, m_parentId, m_Desc.GetString(), m_clipOrder, m_clipGroupOrder));

		m_LastAddedCRC = m_CRC;
		m_lastAddedID = m_id;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Adding the clip to the database failed: %s"), e.errorMessage()));
		return false;
	}
	
	return true;
}

bool CClip::ModifyMainTable()
{
	bool bRet = false;
	try
	{
		Repository().UpdateClip(ToRecord());
		bRet = true;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Saving the changes to clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}

	return bRet;
}

bool CClip::ModifyDescription()
{
	bool bRet = false;
	try
	{
		Repository().UpdateDescription(m_id, m_Desc);
		bRet = true;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Saving the description of clip %d failed: %s"), m_id, e.errorMessage()));
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
		for(INT_PTR i = m_Formats.GetSize()-1; i >= 0 ; i--)
		{
			const CClipFormat& format = m_Formats.ElementAt(i);
			const DittoCore::GlobalBytes block(format.m_hgData);
			FormatRecord record{};
			record.name = GetFormatName(format.m_cfType);
			record.data.assign(block.Bytes().begin(), block.Bytes().end());
			records.push_back(std::move(record));
		}

		const std::vector<int> ids = Repository().InsertFormats(m_id, records);
		for(std::size_t r = 0; r < ids.size(); r++)
		{
			CClipFormat& format = m_Formats.ElementAt(m_Formats.GetSize() - 1 - static_cast<INT_PTR>(r));
			format.m_dataId = ids[r];
			Log(StrF(_T("Added ClipData to DB, Id: %d, ParentId: %d Type: %s, size: %d"), ids[r], m_id, records[r].name.GetString(), static_cast<int>(records[r].data.size())));
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Saving the formats of clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}
		
	return true;
}

CClip::OrderSlot CClip::SlotFor(int parentId)
{
	OrderSlot slot{};
	const bool inGroup = parentId > -1;
	double& sticky = inGroup ? m_stickyClipGroupOrder : m_stickyClipOrder;
	slot.sticky = sticky != INVALID_STICKY;
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

void CClip::Move(int parentId, bool up)
{
	// upstream had a copy of this for each list and kind of clip, and printed the orders into
	// the SQL with %f (6 decimals), so after a few midpoint moves the query found the clip itself
	const OrderSlot slot = SlotFor(parentId);
	try
	{
		CClipRepository repository = Repository();
		const std::optional<double> neighbour = repository.NearestOrder(slot.column, slot.sticky, slot.parentId, *slot.order, up);
		if (neighbour)
		{
			const std::optional<double> beyond = repository.NearestOrder(slot.column, slot.sticky, slot.parentId, *neighbour, up);
			*slot.order = DittoCore::ClipOrder::MovedPast(*neighbour, beyond, up);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		// the order stays unchanged, so the caller's save writes the clip's old position
		CErrorReport::Show(StrF(_T("Moving clip %d %s failed: %s"), m_id, up ? _T("up") : _T("down"), e.errorMessage()));
		return;
	}
}

void CClip::MoveUp(int parentId)
{
	Move(parentId, true);
}

void CClip::MoveDown(int parentId)
{
	Move(parentId, false);
}

void CClip::MakeStickyTop(int parentId)
{
	if (parentId < 0)
	{
		m_stickyClipOrder = GetNewTopSticky(parentId, m_id);
	}
	else
	{
		m_stickyClipGroupOrder = GetNewTopSticky(parentId, m_id);
	}
}

void CClip::MakeStickyLast(int parentId)
{
	if (parentId < 0)
	{
		m_stickyClipOrder = GetNewLastSticky(parentId, m_id);
	}
	else
	{
		m_stickyClipGroupOrder = GetNewLastSticky(parentId, m_id);
	}
}

bool CClip::RemoveStickySetting(int parentId)
{
	bool reset = false;
	if (parentId < 0)
	{
		if (m_stickyClipOrder != INVALID_STICKY)
		{
			m_stickyClipOrder = INVALID_STICKY;
			reset = true;
		}
	}
	else
	{
		if (m_stickyClipGroupOrder != INVALID_STICKY)
		{
			m_stickyClipGroupOrder = INVALID_STICKY;
			reset = true;
		}
	}

	return reset;
}

bool CClip::RemoveStickySetting(int clipId, int parentId)
{
	// returns whether the clip's row was changed; upstream always returned false
	const CClipRepository::OrderColumn column = parentId < 0 ? CClipRepository::OrderColumn::StickyClip : CClipRepository::OrderColumn::StickyClipGroup;
	return Repository().SetOrder(clipId, column, static_cast<double>(INVALID_STICKY));
}

int CClip::GetExistingTopStickyClipId(int parentId)
{
	try
	{
		return Repository().TopStickyClipId(ParentFilter(parentId)).value_or(-1);
	}
	catch (CppSQLite3Exception& e)
	{
		// -1 is also the "no top sticky clip" answer: the caller cannot tell the failure apart
		CErrorReport::Show(StrF(_T("Finding the top sticky clip failed: %s"), e.errorMessage()));
		return -1;
	}
}

std::optional<double> CClip::EdgeOrder(CClipRepository::OrderColumn column, bool sticky, int parentId, bool highest)
{
	try
	{
		return Repository().EdgeOrder(column, sticky, ParentFilter(parentId), highest);
	}
	catch (CppSQLite3Exception& e)
	{
		// kept from upstream for now (callers have no error path): a failed query is reported
		// and treated as an empty list
		CErrorReport::Show(StrF(_T("Reading the clip order failed: %s"), e.errorMessage()));
		return std::nullopt;
	}
}

std::optional<int> CClip::ParentFilter(int parentId)
{
	return parentId > -1 ? std::optional<int>(parentId) : std::nullopt;
}

CClipRepository CClip::Repository()
{
	return CClipRepository(theApp.m_db);
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

double CClip::GetNewTopSticky(int parentId, int clipId)
{
	const std::optional<double> highest = EdgeOrder(parentId < 0 ? CClipRepository::OrderColumn::StickyClip : CClipRepository::OrderColumn::StickyClipGroup, true, parentId, true);
	const double newOrder = DittoCore::ClipOrder::TopSticky(highest);
	Log(StrF(_T("GetNewTopSticky, Id: %d, parentId: %d, NewMax: %f"), clipId, parentId, newOrder));
	return newOrder;
}

double CClip::GetNewLastSticky(int parentId, int clipId)
{
	const std::optional<double> lowest = EdgeOrder(parentId < 0 ? CClipRepository::OrderColumn::StickyClip : CClipRepository::OrderColumn::StickyClipGroup, true, parentId, false);
	const double newOrder = DittoCore::ClipOrder::LastSticky(lowest);
	Log(StrF(_T("GetNewLastSticky, Id: %d, parentId: %d, NewMin: %f"), clipId, parentId, newOrder));
	return newOrder;
}

void CClip::MakeLatestOrder()
{
	m_clipOrder = GetNewOrder(-1, m_id);
}

void CClip::MakeLatestGroupOrder()
{
	if(m_parentId > -1)
	{
		m_clipGroupOrder = GetNewOrder(m_parentId, m_id);
	}
}

void CClip::MakeLastOrder()
{
	m_clipOrder = GetNewLastOrder(-1, m_id);
}

void CClip::MakeLastGroupOrder()
{
	if (m_parentId > -1)
	{
		m_clipGroupOrder = GetNewLastOrder(m_parentId, m_id);
	}
}

double CClip::GetNewOrder(int parentId, int clipId)
{
	const std::optional<double> highest = EdgeOrder(parentId < 0 ? CClipRepository::OrderColumn::Clip : CClipRepository::OrderColumn::ClipGroup, false, parentId, true);
	const double newOrder = DittoCore::ClipOrder::Newest(highest);
	Log(StrF(_T("GetNewOrder, Id: %d, parentId: %d, NewMax: %f"), clipId, parentId, newOrder));
	return newOrder;
}

double CClip::GetNewLastOrder(int parentId, int clipId)
{
	const std::optional<double> lowest = EdgeOrder(parentId < 0 ? CClipRepository::OrderColumn::Clip : CClipRepository::OrderColumn::ClipGroup, false, parentId, false);
	const double newOrder = DittoCore::ClipOrder::Oldest(lowest);
	Log(StrF(_T("GetLastOrder, Id: %d, parentId: %d, NewMin: %f"), clipId, parentId, newOrder));
	return newOrder;
}

BOOL CClip::LoadMainTable(int id)
{
	try
	{
		const std::optional<ClipRecord> record = Repository().LoadClip(id);
		if (record)
		{
			FromRecord(*record);
			return TRUE;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Loading clip %d failed: %s"), id, e.errorMessage()));
		return FALSE;
	}

	return FALSE;
}

// STATICS

// Allocates a Global containing the requested Clip Format Data
HGLOBAL CClip::LoadFormat(int id, UINT cfType)
{
	try
	{
		const std::optional<std::vector<std::byte>> data = Repository().LoadFormat(id, GetFormatName(static_cast<CLIPFORMAT>(cfType)));
		if (data)
		{
			return NewGlobalP(const_cast<std::byte*>(data->data()), data->size());
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Loading the %s format of clip %d failed: %s"), GetFormatName(static_cast<CLIPFORMAT>(cfType)).GetString(), id, e.errorMessage()));
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
		for (const FormatRecord& record : Repository().LoadFormats(id, filter))
		{
			CClipFormat cf;
			cf.m_dataId = record.dataId;
			cf.m_parentId = record.parentId;
			cf.m_cfType = GetFormatID(record.name);
			cf.m_hgData = NewGlobalP(const_cast<std::byte*>(record.data.data()), record.data.size());
			m_Formats.Add(cf);
			// m_Formats owns the data now
			cf.m_hgData = NULL;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Loading the formats of clip %d failed: %s"), id, e.errorMessage()));
		return false;
	}

	ULONGLONG endTick = GetTickCount64();
	if((endTick-startTick) > 150)
		Log(StrF(_T("Paste Timing LoadFormats: %llu, ClipId: %d"), endTick-startTick, id));

	return m_Formats.GetSize() > 0;
}

void CClip::LoadTypes(int id, CClipTypes& types)
{
	types.RemoveAll();
	try
	{
		for (const CString& name : Repository().LoadFormatNames(id))
		{
			types.Add(GetFormatID(name));
		}
	}
	catch (CppSQLite3Exception& e)
	{
		// the names are read in one query before any is added, so types stays empty
		CErrorReport::Show(StrF(_T("Loading the format list of clip %d failed: %s"), id, e.errorMessage()));
		return;
	}
}

CStringW CClip::GetUnicodeTextFormat()
{
	IClipFormat *pFormat = this->Clips()->FindFormatEx(CF_UNICODETEXT);
	if(pFormat != NULL)
	{
		return pFormat->GetAsCString();
	}

	return _T("");
}

CStringA CClip::GetCFTextTextFormat()
{
	IClipFormat *pFormat = this->Clips()->FindFormatEx(CF_TEXT);
	if(pFormat != NULL)
	{
		return pFormat->GetAsCStringA();
	}

	return _T("");
}

CStringA CClip::GetRTFTextFormat()
{
	IClipFormat* pFormat = this->Clips()->FindFormatEx(theApp.m_RTFFormat);
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
	if(f.Open(path, CFile::modeWrite|CFile::modeCreate))
	{
		CStringW w = GetUnicodeTextFormat();
		CStringA a = GetCFTextTextFormat();
		CStringA rtfA = GetRTFTextFormat();		

		if (utf8 && w != _T(""))
		{
			CStringA utf8Data = CTextConvert::UnicodeToUTF8(w);
			f.Write(utf8Data.GetBuffer(), utf8Data.GetLength());

			ret = true;
		}
		else if(unicode && (w != _T("") || forceUnicode))
		{
			std::byte header[2];
			header[0] = (std::byte)0xFF;
			header[1] = (std::byte)0xFE;
			f.Write(&header, 2);
			f.Write(w.GetBuffer(), w.GetLength() * sizeof(wchar_t));

			ret = true;
		}
		else if(asci && a != _T(""))
		{
			f.Write(a.GetBuffer(), a.GetLength());

			ret = true;
		}
		else if (rtf && rtfA != _T(""))
		{
			f.Write(rtfA.GetBuffer(), rtfA.GetLength());

			ret = true;
		}

		f.Close();
	}

	return ret;
}

BOOL CClip::SaveFormats(CString *unicode, CStringA *asci, CStringA *rtf, BOOL updateDescription, std::vector<BYTE> *cf_dibBytes, std::vector<BYTE>* pngBytes)
{
	ARRAY deletedData;
	for (INT_PTR i = m_Formats.GetSize() - 1; i >= 0; i--)
	{
		deletedData.Add(m_Formats[i].m_dataId);
	}

	EmptyFormats();

	if (cf_dibBytes != nullptr && cf_dibBytes->size() > 0)
	{
		AddFormat(CF_DIB, cf_dibBytes->data(), cf_dibBytes->size(), false);
	}

	if (pngBytes != nullptr && pngBytes->size() > 0)
	{
		AddFormat(theApp.m_PNG_Format, pngBytes->data(), pngBytes->size(), false);
	}

	if (rtf != nullptr)
	{
		const int nLength = rtf->GetLength() + sizeof(char);
		AddFormat(theApp.m_RTFFormat, rtf->GetBuffer(nLength), nLength, true);
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

bool CClip::SaveFormatsInTransaction(const ARRAY& deletedData, BOOL updateDescription)
{
	try
	{
		m_CRC = GenerateCRC();

		// rolled back when a step fails; upstream's manual begin stayed open after an exception
		// and ignored the steps' results, so a failed save was committed in part
		CDittoDbTransaction transaction(theApp.m_db);

		CClipRepository repository = Repository();
		// CArrayEx indexes with int
		const int count = static_cast<int>(deletedData.GetSize());
		for (int i = 0; i < count; i++)
		{
			repository.DeleteFormat(deletedData.GetAt(i));
		}

		if (SaveMainRow(updateDescription) == false || AddToDataTable() == false)
		{
			return false;   // the transaction rolls back
		}

		transaction.Commit();
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Saving the formats of clip %d failed: %s"), m_id, e.errorMessage()));
		return false;
	}

	return true;
}

bool CClip::SaveMainRow(BOOL updateDescription)
{
	if (m_id < 0)
	{
		MakeLatestOrder();
		MakeLatestGroupOrder();
		return AddToMainTable();
	}
	return updateDescription ? ModifyDescription() : true;
}

BOOL CClip::WriteImageToFile(CString path)
{
	CClipFormat *bitmap = this->m_Formats.FindFormat(CF_DIB);
	CClipFormat *png = this->m_Formats.FindFormat(theApp.m_PNG_Format);
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
		return WriteImageToFile(path) != FALSE;
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CErrorReport::Show(StrF(_T("Ditto cannot %s the clip's image: the image data is malformed (%s)."), operation.GetString(), CString(error.what()).GetString()));
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
		errorMessage += StrF(_T("Error opening file: %s, Error: %s\r\n"), path.GetString(), szError);
		return false;
	}

	const ULONGLONG fileSize = source.GetLength();
	if (fileSize >= maxSize)
	{
		TCHAR szFileSize[64]{};
		TCHAR szMaxFileSize[64]{};
		StrFormatByteSize((LONGLONG)fileSize, szFileSize, _countof(szFileSize));
		StrFormatByteSize((LONGLONG)maxSize, szMaxFileSize, _countof(szMaxFileSize));
		errorMessage += StrF(_T("File is to large: %s, Size: %s, Max Size: %s\r\n"), path.GetString(), szFileSize, szMaxFileSize);
		return false;
	}

	// fileSize is below the int-sized maximum, so it fits UINT and int
	file.contents.resize(static_cast<size_t>(fileSize));
	const UINT read = source.Read(file.contents.data(), static_cast<UINT>(fileSize));
	if (read != fileSize)
	{
		errorMessage += StrF(_T("Error reading file: %s, read %u of %I64u bytes\r\n"), path.GetString(), read, fileSize);
		return false;
	}

	file.md5 = DittoCore::Md5::Hex(file.contents);
	const CStringA utf8Path = CTextConvert::UnicodeToUTF8(path);
	file.path.assign(utf8Path.GetString(), utf8Path.GetLength());

	Log(StrF(_T("Saving file contents to Ditto Database, file: %s, size: %I64u, md5: %S"), path.GetString(), fileSize, file.md5.c_str()));
	return true;
}

bool CClip::AddFileDataToData(CString &errorMessage)
{
	INT_PTR size = m_Formats.GetSize();
	if (size <= 0)
	{
		errorMessage = _T("No CF_HDROP formats to convert");
		return false;
	}

	bool addedFileData = false;

	int nCF_HDROPIndex = -1;
	int dittoDataIndex = -1;
	for (int i = 0; i < size; i++)
	{
		if (m_Formats[i].m_cfType == CF_HDROP)
		{
			nCF_HDROPIndex = i;
		}
		else if(m_Formats[i].m_cfType == theApp.m_DittoFileData)
		{
			dittoDataIndex = i;
		}
	}	

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
		errorMessage += StrF(_T("The clip's file list is malformed (%s)\r\n"), CString(error.what()).GetString());
		return false;
	}

	CString newDesc = _T("File Contents - ");
	const ULONGLONG maxSize = SavePolicy().Settings().maxFileContentsSize;
	std::vector<CopiedFile> copied;
	for (const std::wstring& path : files)
	{
		CopiedFile file{};
		if (!ReadFileContents(path.c_str(), maxSize, file, errorMessage))
			continue;

		copied.push_back(std::move(file));
		newDesc += path.c_str();
		newDesc += _T("\n");
	}

	if (copied.empty())
		return false;

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
	AddFormat(theApp.m_DittoFileData, const_cast<std::byte*>(record.data()), static_cast<UINT>(record.size()));
	addedFileData = true;

	// AddFormat appended the record after the clip's existing formats, which are already in the
	// database; drop them so AddToDataTable saves only the record. Upstream removed index i of a
	// shrinking array, which skipped formats and removed file data instead.
	this->m_Formats.RemoveAt(0, size);

	this->m_Desc = newDesc;

	if (this->ModifyDescription())
	{
		if (this->AddToDataTable() == FALSE)
		{
			errorMessage += _T("Error saving data to database.");
		}
	}
	else
	{
		errorMessage += _T("Error saving main table to database.");
	}

	return addedFileData;
}

std::unique_ptr<Gdiplus::Bitmap> CClip::CreateGdiplusBitmap()
{
	CClipFormat *png = this->m_Formats.FindFormat(GetFormatID(_T("PNG")));
	if (png != NULL)
		return png->LoadGdiplusBitmap();

	CClipFormat *dib = this->m_Formats.FindFormat(CF_DIB);
	if (dib != NULL)
		return dib->LoadGdiplusBitmap();

	return nullptr;
}

bool CClip::SaveFromEditWnd(BOOL bUpdateDesc)
{
	bool bRet = false;

	try
	{
		// one transaction: upstream deleted the old data first, so a failure while writing the
		// new data lost the clip's contents
		CDittoDbTransaction transaction(theApp.m_db);
		CClipRepository repository = Repository();
		repository.DeleteFormats(m_id);

		DWORD CRC = GenerateCRC();

		if (AddToDataTable() == false)
		{
			return false;   // the transaction rolls back
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
		CErrorReport::Show(StrF(_T("Saving the edited clip %d failed: %s"), m_id, e.errorMessage()));
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
	Log(_T("AddToDB - Start"));

	int savedCount{0};
	bool bResult{false};

	INT_PTR remaining{static_cast<INT_PTR>(m_clips.size())};
	for(const std::unique_ptr<CClip>& clip : m_clips)
	{
		Log(StrF(_T("AddToDB - while(pos), Start Remaining %d"), remaining));
		remaining--;

		CClip* pClip{clip.get()};
		ASSERT(pClip);

		if(bLatestOrder)
		{
			pClip->MakeLatestOrder();
			pClip->MakeLatestGroupOrder();
		}

		bResult = pClip->AddToDB();
		if(bResult)
		{
			savedCount++;
		}

		Log(StrF(_T("AddToDB - while(pos), End Remaining %d, save count: %d"), remaining, savedCount));
	}

	Log(StrF(_T("AddToDB - Start, count: %d"), savedCount));
	
	return savedCount;
}

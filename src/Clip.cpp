// Clip.cpp: implementations of the Clip interfaces
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CP_Main.h"
#include "Clip.h"
#include "DatabaseUtilities.h"
#include "Crc32Dynamic.h"
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
#include <set>

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
				GetFormatName(cfFormat) ) );
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
			GetFormatName(cfFormat)));
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

	int format = 0;
	do
	{
		format = EnumClipboardFormats(format);
		// Currently CF_MAX is not valid format
		// See https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
		if (format == 0 || format == CF_MAX)
			continue;
		types->Add(format);
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
	if (this->m_cfType != CF_DIB && this->m_cfType != theApp.m_PNG_Format)
		return NULL;

	Gdiplus::Bitmap *gdipBitmap;
	if (this->m_cfType == theApp.m_PNG_Format)
		gdipBitmap = PNGImageHelper::GdipImageFromHGLOBAL(this->m_hgData);
	else
		gdipBitmap = DIBImageHelper::GdipImageFromHGLOBAL(this->m_hgData);

	return gdipBitmap;
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
bool CClip::AddFormat(CLIPFORMAT cfType, void* pData, UINT nLen, bool setDesc)
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
int CClip::LoadFromClipboard(CClipTypes* pClipTypes, bool checkClipboardIgnore, CString activeApp)
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
	
	if (CGetSetOptions::m_enforceClipboardIgnoreFormats)
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
	if (CGetSetOptions::m_enforceClipboardIgnoreFormats &&
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

		Log(StrF(_T("Tried to set description from cf_unicode text, Set: %d, Desc: [%s]"), bIsDescSet, m_Desc.Left(30)));
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

			Log(StrF(_T("Tried to set description from cf_text text, Set: %d, Desc: [%s]"), bIsDescSet, m_Desc.Left(30)));
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
			CGetSetOptions::GetIgnoreAnnoyingCFDIBSet(TRUE).count(activeApp.MakeLower()))
		{
			Log(StrF(_T("Ignore CF_DIB from %s"), activeApp));
			continue;
		}

		BOOL bSuccess = false;
		Log(StrF(_T("Begin try and load type %s"), GetFormatName(cf.m_cfType)));
		
		// is this the description we already fetched?
		if(cf.m_cfType == cfDesc.m_cfType)
		{
			cf = cfDesc;
			cfDesc.m_hgData = 0; // cf owns it now (to go into m_Formats)
		}
		else if(!oleData.IsDataAvailable(cf.m_cfType))
		{
			Log(StrF(_T("End of load - Data is not available for type %s"), GetFormatName(cf.m_cfType)));
			continue;
		}
		else
		{
			for (int tries = 0; tries < 2; tries++)
			{
				cf.m_hgData = oleData.GetGlobalData(cf.m_cfType);
				if (cf.m_hgData != NULL)
					break;

				Log(StrF(_T("Tried to get data for type: %s, data is NULL, try: %d"), GetFormatName(cf.m_cfType), tries + 1));
				Sleep(5);
			}
		}
		
		if(cf.m_hgData)
		{
			nSize = GlobalSize(cf.m_hgData);
			if(nSize > 0)
			{
				if(CGetSetOptions::m_lMaxClipSizeInBytes > 0 && (int)nSize > CGetSetOptions::m_lMaxClipSizeInBytes)
				{
					CString cs;
					cs.Format(_T("Maximum clip size reached max size = %d, clip size = %d"), CGetSetOptions::m_lMaxClipSizeInBytes, nSize);
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
				Log(StrF(_T("Data length is 0 for type %s"), GetFormatName(cf.m_cfType)));
			}
			cf.m_hgData = 0; // m_Formats owns it now
		}

		Log(StrF(_T("End of load - type %s, Success: %d"), GetFormatName(cf.m_cfType), bSuccess));
	}

	Log(StrF(_T("End enumerating over supported types, Count: %d"), numTypes));
	
	m_Time = CTime::GetCurrentTime();
			
	if(!bIsDescSet)
	{
		SetDescFromType();

		Log(StrF(_T("Setting description from type, Desc: [%s]"), m_Desc.Left(30)));
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
		if (CGetSetOptions::m_regexHelper.TextMatchFilters(activeApp, stringData))
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

	if(m_Desc.GetLength() > CGetSetOptions::m_bDescTextSize)
	{
		m_Desc = m_Desc.Left(CGetSetOptions::m_bDescTextSize);
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
			m_parentId < 0)
		{	
			int nID = FindDuplicate();
			if(nID >= 0)
			{
				MakeLatestOrder();
				MakeLatestGroupOrder();

				// bound orders keep full precision (upstream printed them with %f)
				CppSQLite3Statement update = theApp.m_db.compileStatement(_T("UPDATE Main SET clipOrder = ? where lID = ?;"));
				update.bind(1, m_clipOrder);
				update.bind(2, nID);
				int ret = update.execDML();

				int groupRet = -1;

				if(m_parentId > -1)
				{
					CppSQLite3Statement groupUpdate = theApp.m_db.compileStatement(_T("UPDATE Main SET clipGroupOrder = ? where lID = ?;"));
					groupUpdate.bind(1, m_clipGroupOrder);
					groupUpdate.bind(2, nID);
					groupRet = groupUpdate.execDML();
				}


				m_id = nID;

				Log(StrF(_T("Found duplicate clip in db, Id: %d, ParentId: %d crc: %d, NewOrder: %f, GroupOrder %f, Ret: %d, GroupRet: %d"),
										nID, m_parentId, m_CRC, m_clipOrder, m_clipGroupOrder, ret, groupRet));

				return true;
			}
		}
	}
	CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

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
		if(CGetSetOptions::m_csPlaySoundOnCopy.IsEmpty() == FALSE)
			PlaySound(CGetSetOptions::m_csPlaySoundOnCopy, NULL, SND_FILENAME|SND_ASYNC);
	}
	
	// should be emptied by AddToDataTable
	//ASSERT(m_Formats.GetSize() == 0);
	
	return bResult;
}

// if a duplicate exists, set recset to the duplicate and return true
int CClip::FindDuplicate()
{
	try
	{
		//If they are allowing duplicates still check 
		//the last copied item
		if(CGetSetOptions::m_bAllowDuplicates)
		{
			if (CGetSetOptions::m_allowBackToBackDuplicates == FALSE)
			{
				if (m_CRC == m_LastAddedCRC)
					return m_lastAddedID;
			}
		}
		else
		{
			CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT lID FROM Main WHERE CRC = %d"), m_CRC);
				
			if(q.eof() == false)
			{
				return q.getIntField(_T("lID"));
			}
		}
	}
	CATCH_SQLITE_EXCEPTION
		
	return -1;
}



DWORD CClip::GenerateCRC()
{
	CCrc32Dynamic crc32;
	DWORD dwCRC = 0xFFFFFFFF;
	const bool adjust = CGetSetOptions::GetAdjustClipsForCRC() != FALSE;

	const INT_PTR size = m_Formats.GetSize();
	for (INT_PTR i = 0; i < size; i++)
	{
		const CClipFormat& format = m_Formats.ElementAt(i);
		if (format.m_hgData != NULL)
		{
			AddToCrc(crc32, format, adjust, dwCRC);
		}
	}
	return ~dwCRC;
}

void CClip::AddToCrc(CCrc32Dynamic& crc32, const CClipFormat& format, bool adjust, DWORD& crc)
{
	const DittoCore::GlobalBytes block(format.m_hgData);
	std::span<const std::byte> bytes = block.Bytes();
	if (adjust && format.m_cfType == theApp.m_RTFFormat)
	{
		// In Word and Outlook the \datastore section and the rsid values change on every copy: leave them out
		std::string normalized = DittoCore::RtfNormalizer::Normalize(DittoCore::ClipText::ReadAnsiBounded(bytes));
		crc32.GenerateCrc32(reinterpret_cast<LPBYTE>(normalized.data()), static_cast<DWORD>(normalized.size()), crc);
		return;
	}
	if (adjust)
	{
		// some programs put text in a block larger than the text: only the text counts
		bytes = TextBytesWithTerminator(format.m_cfType, bytes);
	}
	crc32.GenerateCrc32(reinterpret_cast<LPBYTE>(const_cast<std::byte*>(bytes.data())), static_cast<DWORD>(bytes.size()), crc);
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
	CATCH_SQLITE_EXCEPTION_AND_RETURN(false)
}

// assigns m_ID
bool CClip::AddToMainTable()
{
	try
	{
		// bound values: the texts are stored as they are and the orders keep full precision;
		// upstream doubled the quotes of m_Desc and m_csQuickPaste in memory and printed the
		// orders with %f (6 decimals)
		CppSQLite3Statement insert = theApp.m_db.compileStatement(
			_T("INSERT into Main (lDate, mText, lShortCut, lDontAutoDelete, CRC, bIsGroup, lParentID, QuickPasteText, clipOrder, clipGroupOrder, globalShortCut, lastPasteDate, stickyClipOrder, stickyClipGroupOrder, MoveToGroupShortCut, GlobalMoveToGroupShortCut) ")
			_T("values(?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);"));
		insert.bindInt64(1, m_Time.GetTime());
		insert.bind(2, m_Desc);
		insert.bind(3, m_shortCut);
		insert.bind(4, m_dontAutoDelete);
		insert.bind(5, static_cast<int>(m_CRC));
		insert.bind(6, m_bIsGroup);
		insert.bind(7, m_parentId);
		insert.bind(8, m_csQuickPaste);
		insert.bind(9, m_clipOrder);
		insert.bind(10, m_clipGroupOrder);
		insert.bind(11, m_globalShortCut);
		insert.bindInt64(12, CTime::GetCurrentTime().GetTime());
		insert.bind(13, m_stickyClipOrder);
		insert.bind(14, m_stickyClipGroupOrder);
		insert.bind(15, m_moveToGroupShortCut);
		insert.bind(16, m_globalMoveToGroupShortCut);

		m_id = (long)theApp.m_db.InsertReturningId(insert);

		Log(StrF(_T("Added clip to main table, Id: %d, ParentId: %d Desc: %s, Order: %f, GroupOrder: %f"), m_id, m_parentId, m_Desc, m_clipOrder, m_clipGroupOrder));

		m_LastAddedCRC = m_CRC;
		m_lastAddedID = m_id;
	}
	CATCH_SQLITE_EXCEPTION_AND_RETURN(false)
	
	return true;
}

bool CClip::ModifyMainTable()
{
	bool bRet = false;
	try
	{
		CppSQLite3Statement update = theApp.m_db.compileStatement(_T("UPDATE Main SET lShortCut = ?, ")
			_T("mText = ?, ")
			_T("lParentID = ?, ")
			_T("lDontAutoDelete = ?, ")
			_T("QuickPasteText = ?, ")
			_T("clipOrder = ?, ")
			_T("clipGroupOrder = ?, ")
			_T("globalShortCut = ?, ")
			_T("stickyClipOrder = ?, ")
			_T("stickyClipGroupOrder = ?, ")
			_T("MoveToGroupShortCut = ?, ")
			_T("GlobalMoveToGroupShortCut = ? ")
			_T("WHERE lID = ?;"));
		update.bind(1, m_shortCut);
		update.bind(2, m_Desc);
		update.bind(3, m_parentId);
		update.bind(4, m_dontAutoDelete);
		update.bind(5, m_csQuickPaste);
		update.bind(6, m_clipOrder);
		update.bind(7, m_clipGroupOrder);
		update.bind(8, m_globalShortCut);
		update.bind(9, m_stickyClipOrder);
		update.bind(10, m_stickyClipGroupOrder);
		update.bind(11, m_moveToGroupShortCut);
		update.bind(12, m_globalMoveToGroupShortCut);
		update.bind(13, m_id);
		update.execDML();

		bRet = true;
	}
	CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

	return bRet;
}

bool CClip::ModifyDescription()
{
	bool bRet = false;
	try
	{
		CppSQLite3Statement update = theApp.m_db.compileStatement(_T("UPDATE Main SET mText = ? WHERE lID = ?;"));
		update.bind(1, m_Desc);
		update.bind(2, m_id);
		update.execDML();

		bRet = true;
	}
	CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

		return bRet;
}

// Empties m_Formats as it saves them to the Data Table.
bool CClip::AddToDataTable()
{
	CClipFormat* pCF;

	try
	{
		CppSQLite3Statement stmt = theApp.m_db.compileStatement(_T("insert into Data values (NULL, ?, ?, ?);"));
		
		for(INT_PTR i = m_Formats.GetSize()-1; i >= 0 ; i--)
		{
			pCF = &m_Formats.ElementAt(i);

			CString formatName = GetFormatName(pCF->m_cfType);
			int clipSize = 0;
			
			stmt.bind(1, m_id);
			stmt.bind(2, formatName);

			const DittoCore::GlobalBytes block(pCF->m_hgData);
			clipSize = static_cast<int>(block.Bytes().size());
			stmt.bind(3, reinterpret_cast<const unsigned char*>(block.Bytes().data()), clipSize);

			pCF->m_dataId = (long)theApp.m_db.InsertReturningId(stmt);
			stmt.reset();

			Log(StrF(_T("Added ClipData to DB, Id: %d, ParentId: %d Type: %s, size: %d"), pCF->m_dataId, m_id, formatName, clipSize));
		}
	}
	CATCH_SQLITE_EXCEPTION_AND_RETURN(false)
		
	return true;
}

CClip::OrderSlot CClip::SlotFor(int parentId)
{
	OrderSlot slot{};
	slot.inGroup = parentId > -1;
	slot.stickyColumn = slot.inGroup ? _T("stickyClipGroupOrder") : _T("stickyClipOrder");
	double& sticky = slot.inGroup ? m_stickyClipGroupOrder : m_stickyClipOrder;
	slot.sticky = sticky != INVALID_STICKY;
	slot.column = slot.sticky ? slot.stickyColumn : (slot.inGroup ? _T("clipGroupOrder") : _T("clipOrder"));
	slot.order = slot.sticky ? &sticky : (slot.inGroup ? &m_clipGroupOrder : &m_clipOrder);
	return slot;
}

CString CClip::NeighbourSql(const OrderSlot& slot, bool up)
{
	// the nearest order above (up) or below (down) among the clips of the same list and kind
	CString sql;
	sql.Format(_T("SELECT %s FROM Main WHERE %s%s %s -(2147483647) AND %s %s ? ORDER BY %s %s LIMIT 1"),
		slot.column.GetString(),
		slot.inGroup ? _T("lParentID = ? AND ") : _T(""),
		slot.stickyColumn.GetString(),
		slot.sticky ? _T("<>") : _T("="),
		slot.column.GetString(),
		up ? _T(">") : _T("<"),
		slot.column.GetString(),
		up ? _T("ASC") : _T("DESC"));
	return sql;
}

std::optional<double> CClip::NeighbourOrder(const CString& sql, int parentId, double from)
{
	CppSQLite3Statement statement = theApp.m_db.compileStatement(sql);
	int param = 1;
	if (parentId > -1)
	{
		statement.bind(param++, parentId);
	}
	statement.bind(param, from);
	CppSQLite3Query q = statement.execQuery();
	if (q.eof())
	{
		return std::nullopt;
	}
	return q.getFloatField(0);
}

void CClip::Move(int parentId, bool up)
{
	// upstream had a copy of this for each list and kind of clip, and printed the orders into
	// the SQL with %f (6 decimals), so after a few midpoint moves the query found the clip itself
	const OrderSlot slot = SlotFor(parentId);
	const CString sql = NeighbourSql(slot, up);
	try
	{
		const std::optional<double> neighbour = NeighbourOrder(sql, parentId, *slot.order);
		if (neighbour)
		{
			*slot.order = DittoCore::ClipOrder::MovedPast(*neighbour, NeighbourOrder(sql, parentId, *neighbour), up);
		}
	}
	CATCH_SQLITE_EXCEPTION
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
	const TCHAR* sql = parentId < 0
		? _T("UPDATE Main SET stickyClipOrder = ? WHERE lID = ?")
		: _T("UPDATE Main SET stickyClipGroupOrder = ? WHERE lID = ?");
	CppSQLite3Statement update = theApp.m_db.compileStatement(sql);
	update.bind(1, static_cast<double>(INVALID_STICKY));
	update.bind(2, clipId);
	return update.execDML() > 0;
}

int CClip::GetExistingTopStickyClipId(int parentId)
{
	int existingTopClipId = -1;

	try
	{
		if (parentId < 0)
		{
			CppSQLite3Query q = theApp.m_db.execQuery(_T("SELECT lID FROM Main WHERE stickyClipOrder <> -(2147483647) ORDER BY stickyClipOrder DESC LIMIT 1"));
			if (q.eof() == false)
			{
				existingTopClipId = q.getIntField(_T("lID"));
			}
		}
		else
		{
			CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT lID FROM Main WHERE lParentID = %d AND stickyClipGroupOrder <> -(2147483647) ORDER BY stickyClipGroupOrder DESC LIMIT 1"), parentId);
			if (q.eof() == false)
			{
				existingTopClipId = q.getIntField(_T("lID"));
			}
		}

	}
	CATCH_SQLITE_EXCEPTION

	return existingTopClipId;
}

std::optional<double> CClip::EdgeOrder(const CString& column, bool sticky, int parentId, bool highest)
{
	// the highest or lowest order among the main list's clips or a group's clips; only sticky
	// clips when sticky
	CString sql;
	sql.Format(_T("SELECT %s FROM Main WHERE %s %s ORDER BY %s %s LIMIT 1"),
		column.GetString(),
		parentId > -1 ? _T("lParentID = ? AND") : _T(""),
		sticky ? (column + _T(" <> -(2147483647)")).GetString() : (column + _T(" notnull")).GetString(),
		column.GetString(),
		highest ? _T("DESC") : _T("ASC"));
	try
	{
		CppSQLite3Statement statement = theApp.m_db.compileStatement(sql);
		if (parentId > -1)
		{
			statement.bind(1, parentId);
		}
		CppSQLite3Query q = statement.execQuery();
		if (q.eof() == false)
		{
			return q.getFloatField(0);
		}
	}
	// kept from upstream for now (callers have no error path): a failed query is logged and
	// treated as an empty list
	CATCH_SQLITE_EXCEPTION
	return std::nullopt;
}

double CClip::GetNewTopSticky(int parentId, int clipId)
{
	const std::optional<double> highest = EdgeOrder(parentId < 0 ? _T("stickyClipOrder") : _T("stickyClipGroupOrder"), true, parentId, true);
	const double newOrder = DittoCore::ClipOrder::TopSticky(highest);
	Log(StrF(_T("GetNewTopSticky, Id: %d, parentId: %d, NewMax: %f"), clipId, parentId, newOrder));
	return newOrder;
}

double CClip::GetNewLastSticky(int parentId, int clipId)
{
	const std::optional<double> lowest = EdgeOrder(parentId < 0 ? _T("stickyClipOrder") : _T("stickyClipGroupOrder"), true, parentId, false);
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
	const std::optional<double> highest = EdgeOrder(parentId < 0 ? _T("clipOrder") : _T("clipGroupOrder"), false, parentId, true);
	const double newOrder = DittoCore::ClipOrder::Newest(highest);
	Log(StrF(_T("GetNewOrder, Id: %d, parentId: %d, NewMax: %f"), clipId, parentId, newOrder));
	return newOrder;
}

double CClip::GetNewLastOrder(int parentId, int clipId)
{
	const std::optional<double> lowest = EdgeOrder(parentId < 0 ? _T("clipOrder") : _T("clipGroupOrder"), false, parentId, false);
	const double newOrder = DittoCore::ClipOrder::Oldest(lowest);
	Log(StrF(_T("GetLastOrder, Id: %d, parentId: %d, NewMin: %f"), clipId, parentId, newOrder));
	return newOrder;
}

BOOL CClip::LoadMainTable(int id)
{
	bool bRet = false;
	try
	{
		CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT * FROM Main WHERE lID = %d"), id);

		if(q.eof() == false)
		{
			m_Time = q.getInt64Field(_T("lDate"));
			m_Desc = q.getStringField(_T("mText"));
			m_CRC = q.getIntField(_T("CRC"));
			m_parentId = q.getIntField(_T("lParentID"));
			m_dontAutoDelete = q.getIntField(_T("lDontAutoDelete"));
			m_shortCut = q.getIntField(_T("lShortCut"));
			m_bIsGroup = q.getIntField(_T("bIsGroup"));
			m_csQuickPaste = q.getStringField(_T("QuickPasteText"));
			m_clipOrder = q.getFloatField(_T("clipOrder"));
			m_clipGroupOrder = q.getFloatField(_T("clipGroupOrder"));
			m_globalShortCut = q.getIntField(_T("globalShortCut"));
			m_lastPasteDate = q.getInt64Field(_T("lastPasteDate"));
			m_stickyClipOrder = q.getFloatField(_T("stickyClipOrder"));
			m_stickyClipGroupOrder = q.getFloatField(_T("stickyClipGroupOrder"));
			m_moveToGroupShortCut = q.getIntField(_T("MoveToGroupShortCut"));
			m_globalMoveToGroupShortCut = q.getIntField(_T("GlobalMoveToGroupShortCut"));

			m_id = id;

			bRet = true;
		}
	}
	CATCH_SQLITE_EXCEPTION_AND_RETURN(FALSE)

	return bRet;
}

// STATICS

// Allocates a Global containing the requested Clip Format Data
HGLOBAL CClip::LoadFormat(int id, UINT cfType)
{
	HGLOBAL hGlobal = 0;
	try
	{
		CString csSQL;
		
		csSQL.Format(
			_T("SELECT Data.ooData FROM Data ")
			_T("INNER JOIN Main ON Main.lID = Data.lParentID ")
			_T("WHERE Main.lID = %d ")
			_T("AND Data.strClipBoardFormat = \'%s\'"),
			id,
			GetFormatName(cfType));

		CppSQLite3Query q = theApp.m_db.execQuery(csSQL);

		if(q.eof() == false)
		{
			int nDataLen = 0;
			const unsigned char *cData = q.getBlobField(0, nDataLen);
			if(cData == NULL)
			{
				return NULL;   // a format saved without data; upstream returned false as a handle
			}

			hGlobal = NewGlobalP((LPVOID)cData, nDataLen);
		}
	}
	CATCH_SQLITE_EXCEPTION
		
	return hGlobal;
}

bool CClip::LoadFormats(int id, bool bOnlyLoad_CF_TEXT, bool includeRichTextForTextOnly, int dataId)
{
	DWORD startTick = GetTickCount();
	CClipFormat cf;
	m_Formats.RemoveAll();

	try
	{	
		//Open the data table for all that have the parent id

		//Order by Data.lID so that when generating CRC it's always in the same order as the first time
		//we generated it
		CString csSQL;

		CString textFilter = _T("");
		if(bOnlyLoad_CF_TEXT)
		{
			textFilter = _T("(strClipBoardFormat = 'CF_TEXT' OR strClipBoardFormat = 'CF_UNICODETEXT' OR strClipBoardFormat = 'CF_HDROP'");

			if(includeRichTextForTextOnly)
			{
				textFilter = textFilter + _T(" OR strClipBoardFormat = 'Rich Text Format') AND ");
			}
			else
			{
				textFilter = textFilter + _T(") AND ");
			}
		}

		CString dataIdFilter = _T("");
		if (dataId >= 0)
		{
			dataIdFilter.Format(_T("AND lID = %d "), dataId);


		}

		csSQL.Format(
			_T("SELECT lID, lParentID, strClipBoardFormat, ooData FROM Data ")
			_T("WHERE %s lParentID = %d %s ORDER BY Data.lID desc"), textFilter, id, dataIdFilter);

		CppSQLite3Query q = theApp.m_db.execQuery(csSQL);

		while(q.eof() == false)
		{
			cf.m_dataId = q.getIntField(_T("lID"));
			cf.m_parentId = q.getIntField(_T("lParentID"));
			cf.m_cfType = GetFormatID(q.getStringField(_T("strClipBoardFormat")));
			
			if(bOnlyLoad_CF_TEXT)
			{
				if(cf.m_cfType != CF_TEXT && 
					cf.m_cfType != CF_UNICODETEXT &&
					cf.m_cfType != CF_HDROP &&
					(cf.m_cfType != theApp.m_RTFFormat && !includeRichTextForTextOnly))
				{
					q.nextRow();
					continue;
				}
			}

			int nDataLen = 0;
			const unsigned char *cData = q.getBlobField(_T("ooData"), nDataLen);
			if(cData == NULL)
			{
				// a format saved without data is left out; upstream added it with the previous
				// format's handle, so two formats owned one block and freed it twice
				Log(StrF(_T("LoadFormats: clip %d has a format without data (row %d), left out"), id, cf.m_dataId));
				q.nextRow();
				continue;
			}

			cf.m_hgData = NewGlobalP((LPVOID)cData, nDataLen);
			m_Formats.Add(cf);

			q.nextRow();
		}

		// formats owns all the data
		cf.m_hgData = 0;
	}
	CATCH_SQLITE_EXCEPTION_AND_RETURN(false)
		
	DWORD endTick = GetTickCount();
	if((endTick-startTick) > 150)
		Log(StrF(_T("Paste Timing LoadFormats: %d, ClipId: %d"), endTick-startTick, id));

	return m_Formats.GetSize() > 0;
}

void CClip::LoadTypes(int id, CClipTypes& types)
{
	types.RemoveAll();
	try
	{
		CString csSQL;
		// get formats for Clip "lID" (Main.lID) using the corresponding Main.lDataID
		
		//Order by Data.lID so that when generating CRC it's always in the same order as the first time
		//we generated it
		csSQL.Format(
			_T("SELECT strClipBoardFormat FROM Data ")
			_T("INNER JOIN Main ON Main.lID = Data.lParentID ")
			_T("WHERE Main.lID = %d ORDER BY Data.lID desc"), id);

		CppSQLite3Query q = theApp.m_db.execQuery(csSQL);			

		while(q.eof() == false)
		{		
			types.Add(GetFormatID(q.getStringField(0)));
			q.nextRow();
		}
	}
	CATCH_SQLITE_EXCEPTION
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

BOOL CClip::WriteTextToHtmlFile(CString path)
{
	BOOL ret = false;

	CFile f;
	if (f.Open(path, CFile::modeWrite | CFile::modeCreate))
	{
		IClipFormat *pFormat = this->Clips()->FindFormatEx(theApp.m_HTML_Format);
		if (pFormat != NULL)
		{
			CStringA html = pFormat->GetAsCStringA();

			int pos = html.Find("<html");
			if (pos >= 0)
			{
				html = html.Mid(pos);
			}
			else
			{
				html = html;
			}

			f.Write(html.GetBuffer(), html.GetLength());			
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

	try
	{
		m_CRC = GenerateCRC();

		theApp.m_db.execDML(_T("begin transaction;"));

		auto count = deletedData.GetSize();
		for (int i = 0; i < count; i++)
		{
			int count = theApp.m_db.execDMLEx(_T("DELETE FROM Data WHERE lID = %d;"), deletedData[i]);
		}

		if (m_id >= 0)
		{
			if (updateDescription)
			{
				ModifyDescription();
			}
		}
		else
		{
			MakeLatestOrder();
			MakeLatestGroupOrder();
			AddToMainTable();
		}

		AddToDataTable();

		theApp.m_db.execDML(_T("commit transaction;"));
	}
	CATCH_SQLITE_EXCEPTION_AND_RETURN(false)

	return TRUE;
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

	CMd5 md5;
	file.md5 = md5.CalcMD5FromString(reinterpret_cast<const char*>(file.contents.data()), static_cast<int>(fileSize));
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
	const ULONGLONG maxSize = (ULONGLONG)CGetSetOptions::GetMaxFileContentsSize();
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

Gdiplus::Bitmap *CClip::CreateGdiplusBitmap()
{
	CClipFormat *png = this->m_Formats.FindFormat(GetFormatID(_T("PNG")));
	if (png != NULL)
		return png->CreateGdiplusBitmap();

	CClipFormat *dib = this->m_Formats.FindFormat(CF_DIB);
	if (dib != NULL)
		return dib->CreateGdiplusBitmap();

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
		theApp.m_db.execDMLEx(_T("DELETE FROM Data WHERE lParentID = %d;"), m_id);

		DWORD CRC = GenerateCRC();

		if (AddToDataTable() == false)
		{
			return false;   // the transaction rolls back
		}

		theApp.m_db.execDMLEx(_T("UPDATE Main SET CRC = %d WHERE lID = %d"), CRC, m_id);

		if (bUpdateDesc)
		{
			CppSQLite3Statement update = theApp.m_db.compileStatement(_T("UPDATE Main SET mText = ? WHERE lID = ?"));
			update.bind(1, m_Desc);
			update.bind(2, m_id);
			update.execDML();
		}

		transaction.Commit();
		bRet = true;
	}
	CATCH_SQLITE_EXCEPTION

		return bRet;
}

/*----------------------------------------------------------------------------*\
CClipList
\*----------------------------------------------------------------------------*/

CClipList::~CClipList()
{
	CClip* pClip;
	while(GetCount())
	{
		pClip = RemoveHead();
		delete pClip;
	}
}

// returns the number of clips actually saved
// while this does empty the Format Data, it does not delete the Clips.
int CClipList::AddToDB(bool bLatestOrder)
{
	Log(_T("AddToDB - Start"));

	int savedCount = 0;
	CClip* pClip;
	POSITION pos;
	bool bResult;
	
	INT_PTR remaining = GetCount();
	pos = GetHeadPosition();
	while(pos)
	{
		Log(StrF(_T("AddToDB - while(pos), Start Remaining %d"), remaining));
		remaining--;
		
		pClip = GetNext(pos);
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

const CClipList& CClipList::operator=(const CClipList &cliplist)
{
	POSITION pos;
	CClip* pClip;
	
	pos = cliplist.GetHeadPosition();
	while(pos)
	{
		pClip = cliplist.GetNext(pos);
		ASSERT(pClip);

		CClip *pNewClip = new CClip;
		if(pNewClip)
		{
			*pNewClip = *pClip;
			
			AddTail(pNewClip);
		}
	}
	
	return *this;
}

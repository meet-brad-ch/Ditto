#include "stdafx.h"
#include "FileDialogPath.h"
#include "CP_Main.h"
#include "Misc.h"
#include "OptionsSheet.h"
#include "..\Shared\TextConvert.h"
#include "AlphaBlend.h"
#include "Tlhelp32.h"
#include <sys/types.h>  
#include <sys/stat.h> 
#include "Path.h"
#include "GlobalBytes.h"
#include "ClipboardFormatError.h"
#include "ErrorReport.h"
#include <algorithm>
#include <array>
#include <new>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

void AppendToFile(const TCHAR* fn, const TCHAR* msg)
{
#ifdef _UNICODE
	FILE *file = _wfopen(fn, _T("a"));
#else
	FILE *file = fopen(fn, _T("a"));
#endif

	ASSERT( file );

	if(file != NULL)
	{
		#ifdef _UNICODE
			fwprintf(file, _T("%s"), msg);
		#else
			fprintf(file, _T("%s"),msg);
		#endif

		fclose(file);	
	}
}

void log(const TCHAR* msg, CString csFile, long lLine)
{
	ASSERT(AfxIsValidString(msg));

	SYSTEMTIME st;
	GetLocalTime(&st);
	
	CString	csText;
	csText.Format(_T("[%d/%d/%d %02d:%02d:%02d.%03d - "), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	CString csFileLine;
	csFile = GetFileName(csFile);
	csFileLine.Format(_T("%s %d] "), csFile.GetString(), lLine);
	csText += csFileLine;
	
	csText += msg;
	csText += "\n";

#ifndef _DEBUG
	if(CGetSetOptions::m_outputDebugStringLogging)
#endif
	{
		OutputDebugString(csText);
	}

#ifndef _DEBUG
	if(!CGetSetOptions::m_bEnableDebugLogging)
		return;
#endif
	
	CString csExeFile = CGetSetOptions::GetPath(PATH_LOG_FILE);
	csExeFile += "Ditto.log";

	AppendToFile(csExeFile, csText);
}

CString GetErrorString( int err )
{
	CString str;
	LPVOID lpMsgBuf;
	
	::FormatMessage( 
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		err,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR) &lpMsgBuf,
		0,
		NULL 
		);
	str = (LPCTSTR) lpMsgBuf;
	// Display the string.
	//  ::MessageBox( NULL, lpMsgBuf, "GetLastError", MB_OK|MB_ICONINFORMATION );
	::LocalFree( lpMsgBuf );
	return str;
}

int g_funnyGetTickCountAdjustment = -1;

double IdleSeconds()
{
	LASTINPUTINFO info; 
	info.cbSize = sizeof(info);
	GetLastInputInfo(&info);   
	// Compared with LASTINPUTINFO::dwTime, a 32-bit tick value, so keep 32-bit wrap-around arithmetic.
	DWORD currentTick  = static_cast<DWORD>(GetTickCount64());

	if(g_funnyGetTickCountAdjustment == -1)
	{
		if(currentTick < info.dwTime)
		{
			g_funnyGetTickCountAdjustment = 1;
		}
		else
		{
			g_funnyGetTickCountAdjustment = 0; 
		}		
	}
	
	if(g_funnyGetTickCountAdjustment == 1 || g_funnyGetTickCountAdjustment == 2)
	{
		//Output message the first time
		if(g_funnyGetTickCountAdjustment == 1)
		{
			Log(StrF(_T("Adjusting time of get tickcount by: %d, on startup we found GetTickCount to be less than last input"), CGetSetOptions::GetFunnyTickCountAdjustment()));
			g_funnyGetTickCountAdjustment = 2;
		}
		currentTick += CGetSetOptions::GetFunnyTickCountAdjustment();
	}

	double idleSeconds = (currentTick - info.dwTime)/1000.0;

	return idleSeconds;
}

CString StrF(const TCHAR * pszFormat, ...)
{
	ASSERT( AfxIsValidString( pszFormat ) );
	CString str;
	va_list argList;
	va_start( argList, pszFormat );
	str.FormatV( pszFormat, argList );
	va_end( argList );
	return str;
}

BYTE GetEscapeChar( BYTE ch )
{
	/** @brief One C escape sequence: the character after the backslash and the character it stands for. */
	struct EscapeChar
	{
		/** @brief The character after the backslash. */
		BYTE escaped{};
		/** @brief The character the sequence stands for. */
		BYTE value{};
	};
	static constexpr std::array<EscapeChar, 12> escapes{ {
		{ '\'', '\'' }, // Single quotation mark (') = 39 or 0x27
		{ '\"', '\"' }, // Double quotation mark (") = 34 or 0x22
		{ '?', '\?' }, // Question mark (?) = 63 or 0x3f
		{ '\\', '\\' }, // Backslash (\) = 92 or 0x5c
		{ 'a', '\a' }, // Alert (BEL) = 7
		{ 'b', '\b' }, // Backspace (BS) = 8
		{ 'f', '\f' }, // Formfeed (FF) = 12 or 0x0c
		{ 'n', '\n' }, // Newline (NL or LF) = 10 or 0x0a
		{ 'r', '\r' }, // Carriage Return (CR) = 13 or 0x0d
		{ 't', '\t' }, // Horizontal tab (HT) = 9
		{ 'v', '\v' }, // Vertical tab (VT) = 11 or 0x0b
		{ '0', '\0' }, // Null character (NUL) = 0
	} };
	for (const EscapeChar& escape : escapes)
	{
		if (escape.escaped == ch)
		{
			return escape.value;
		}
	}
	return 0; // invalid
}

CString RemoveEscapes( const TCHAR* str )
{
	ASSERT( str );
	CString ret;
	TCHAR* pSrc = (TCHAR*) str;
	TCHAR* pDest = ret.GetBuffer((int)STRLEN(pSrc));
	TCHAR* pStart = pDest;
	while( *pSrc != '\0' )
	{
		if( *pSrc == '\\' )
		{
			pSrc++;
                       *pDest = GetEscapeChar((BYTE)*pSrc );
		}
		else
			*pDest = *pSrc;
		pSrc++;
		pDest++;
	}
	ret.ReleaseBuffer((int)(pDest - pStart));
	return ret;
}

bool IsAppWnd( HWND hWnd )
{
	DWORD dwMyPID = ::GetCurrentProcessId();
	DWORD dwTestPID;
	::GetWindowThreadProcessId( hWnd, &dwTestPID );
	return dwMyPID == dwTestPID;
}

/*----------------------------------------------------------------------------*\
Global Memory Helper Functions
\*----------------------------------------------------------------------------*/

// make sure the given HGLOBAL is valid.
BOOL IsValid(HGLOBAL hGlobal)
{
	void* pvData = ::GlobalLock(hGlobal);
	::GlobalUnlock(hGlobal);
	return (pvData != NULL);
}

// Copies ulBufLen bytes into hDest; throws when hDest is not a lockable block of at least that size
void CopyToGlobalHP(HGLOBAL hDest, const void* pBuf, SIZE_T ulBufLen)
{
	DittoCore::GlobalBytes dest(hDest);
	if (pBuf == nullptr || ulBufLen > dest.WritableBytes().size())
	{
		throw DittoCore::ClipboardFormatError("copy of " + std::to_string(ulBufLen) + " bytes does not fit a block of " + std::to_string(dest.WritableBytes().size()));
	}
	memcpy(dest.WritableBytes().data(), pBuf, ulBufLen);
}

void CopyToGlobalHH(HGLOBAL hDest, HGLOBAL hSource, SIZE_T ulBufLen)
{
	const DittoCore::GlobalBytes source(hSource);
	if (ulBufLen > source.Bytes().size())
	{
		throw DittoCore::ClipboardFormatError("copy of " + std::to_string(ulBufLen) + " bytes reads past a block of " + std::to_string(source.Bytes().size()));
	}
	CopyToGlobalHP(hDest, source.Bytes().data(), ulBufLen);
}


HGLOBAL NewGlobalP(const void* pBuf, SIZE_T nLen)
{
	HGLOBAL hDest = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, nLen);
	if (hDest == nullptr)
	{
		throw std::bad_alloc();
	}
	CopyToGlobalHP(hDest, pBuf, nLen);
	return hDest;
}

HGLOBAL NewGlobal(SIZE_T nLen)
{
	ASSERT(nLen);
	HGLOBAL hDest = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, nLen);
	return hDest;
}

HGLOBAL NewGlobalH(HGLOBAL hSource, SIZE_T nLen)
{
	const DittoCore::GlobalBytes source(hSource);
	if (nLen > source.Bytes().size())
	{
		throw DittoCore::ClipboardFormatError("copy of " + std::to_string(nLen) + " bytes reads past a block of " + std::to_string(source.Bytes().size()));
	}
	return NewGlobalP(source.Bytes().data(), nLen);
}

int CompareGlobalHP(HGLOBAL hLeft, LPVOID pBuf, SIZE_T ulBufLen)
{
	const DittoCore::GlobalBytes left(hLeft);
	if (pBuf == nullptr || ulBufLen > left.Bytes().size())
	{
		throw DittoCore::ClipboardFormatError("compare of " + std::to_string(ulBufLen) + " bytes reads past a block of " + std::to_string(left.Bytes().size()));
	}
	return memcmp(left.Bytes().data(), pBuf, ulBufLen);
}

int CompareGlobalHH( HGLOBAL hLeft, HGLOBAL hRight, SIZE_T ulBufLen)
{
	const DittoCore::GlobalBytes right(hRight);
	if (ulBufLen > right.Bytes().size())
	{
		throw DittoCore::ClipboardFormatError("compare of " + std::to_string(ulBufLen) + " bytes reads past a block of " + std::to_string(right.Bytes().size()));
	}
	return CompareGlobalHP(hLeft, const_cast<std::byte*>(right.Bytes().data()), ulBufLen);
}

// https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
std::vector<CLIPFORMAT> GetSystemClipFormats()
{
	std::vector<CLIPFORMAT> v = {
		CF_TEXT,
		CF_BITMAP,
		CF_METAFILEPICT,
		CF_SYLK,
		CF_DIF,
		CF_TIFF,
		CF_OEMTEXT,
		CF_DIB,
		CF_PALETTE,
		CF_PENDATA,
		CF_RIFF,
		CF_WAVE,
		CF_UNICODETEXT,
		CF_ENHMETAFILE,
		CF_HDROP,
		CF_LOCALE,
		CF_OWNERDISPLAY,
		CF_DSPTEXT,
		CF_DSPBITMAP,
		CF_DSPMETAFILEPICT,
		CF_DSPENHMETAFILE
	};

	return v;
}

//Do not change these these are stored in the database
CLIPFORMAT GetFormatID(LPCTSTR cbName)
{
	/** @brief A standard clipboard format and the name it is stored under. */
	struct NamedFormat
	{
		/** @brief The stored name. */
		const TCHAR* name{};
		/** @brief The format. */
		CLIPFORMAT id{};
	};
	// CF_BITMAP has no entry: its name is registered as a format of its own (as before)
	static constexpr std::array<NamedFormat, 21> formats{ {
		{ _T("CF_TEXT"), CF_TEXT },
		{ _T("CF_METAFILEPICT"), CF_METAFILEPICT },
		{ _T("CF_SYLK"), CF_SYLK },
		{ _T("CF_DIF"), CF_DIF },
		{ _T("CF_TIFF"), CF_TIFF },
		{ _T("CF_OEMTEXT"), CF_OEMTEXT },
		{ _T("CF_DIB"), CF_DIB },
		{ _T("CF_PALETTE"), CF_PALETTE },
		{ _T("CF_PENDATA"), CF_PENDATA },
		{ _T("CF_RIFF"), CF_RIFF },
		{ _T("CF_WAVE"), CF_WAVE },
		{ _T("CF_UNICODETEXT"), CF_UNICODETEXT },
		{ _T("CF_ENHMETAFILE"), CF_ENHMETAFILE },
		{ _T("CF_HDROP"), CF_HDROP },
		{ _T("CF_LOCALE"), CF_LOCALE },
		{ _T("CF_OWNERDISPLAY"), CF_OWNERDISPLAY },
		{ _T("CF_DSPTEXT"), CF_DSPTEXT },
		{ _T("CF_DSPBITMAP"), CF_DSPBITMAP },
		{ _T("CF_DSPMETAFILEPICT"), CF_DSPMETAFILEPICT },
		{ _T("CF_DSPENHMETAFILE"), CF_DSPENHMETAFILE },
		{ _T("CF_DIBV5"), CF_DIBV5 },
	} };
	for (const NamedFormat& format : formats)
	{
		if (STRCMP(cbName, format.name) == 0)
		{
			return format.id;
		}
	}

	// Registered clipboard formats are in the range 0xC000..0xFFFF, so they fit in a CLIPFORMAT.
	return static_cast<CLIPFORMAT>(::RegisterClipboardFormat(cbName));
}

//Do not change these these are stored in the database
CString GetFormatName(CLIPFORMAT cbType)
{
	/** @brief A standard clipboard format and the name it is stored under. */
	struct FormatName
	{
		/** @brief The format. */
		CLIPFORMAT id{};
		/** @brief The stored name. */
		const TCHAR* name{};
	};
	static constexpr std::array<FormatName, 22> formats{ {
		{ CF_TEXT, _T("CF_TEXT") },
		{ CF_BITMAP, _T("CF_BITMAP") },
		{ CF_METAFILEPICT, _T("CF_METAFILEPICT") },
		{ CF_SYLK, _T("CF_SYLK") },
		{ CF_DIF, _T("CF_DIF") },
		{ CF_TIFF, _T("CF_TIFF") },
		{ CF_OEMTEXT, _T("CF_OEMTEXT") },
		{ CF_DIB, _T("CF_DIB") },
		{ CF_PALETTE, _T("CF_PALETTE") },
		{ CF_PENDATA, _T("CF_PENDATA") },
		{ CF_RIFF, _T("CF_RIFF") },
		{ CF_WAVE, _T("CF_WAVE") },
		{ CF_UNICODETEXT, _T("CF_UNICODETEXT") },
		{ CF_ENHMETAFILE, _T("CF_ENHMETAFILE") },
		{ CF_HDROP, _T("CF_HDROP") },
		{ CF_LOCALE, _T("CF_LOCALE") },
		{ CF_OWNERDISPLAY, _T("CF_OWNERDISPLAY") },
		{ CF_DSPTEXT, _T("CF_DSPTEXT") },
		{ CF_DSPBITMAP, _T("CF_DSPBITMAP") },
		{ CF_DSPMETAFILEPICT, _T("CF_DSPMETAFILEPICT") },
		{ CF_DSPENHMETAFILE, _T("CF_DSPENHMETAFILE") },
		{ CF_DIBV5, _T("CF_DIBV5") },
	} };
	for (const FormatName& format : formats)
	{
		if (format.id == cbType)
		{
			return format.name;
		}
	}

	//Not a default type get the name from the clipboard
	if (cbType != 0)
	{
		// zero-initialized: a failed call leaves an empty name; upstream returned the
		// uninitialized buffer, so a failed call gave stack garbage
		TCHAR szFormat[256]{};
		GetClipboardFormatName(cbType, szFormat, _countof(szFormat));
		return szFormat;
	}

	return "ERROR";
}

CString GetFilePath(CString csFileName)
{
	long lSlash = csFileName.ReverseFind('\\');
	
	if(lSlash > -1)
	{
		csFileName = csFileName.Left(lSlash + 1);
	}
	
	return csFileName;
}

CString GetFileName(CString csFileName)
{
	long lSlash = csFileName.ReverseFind('\\');
	if(lSlash > -1)
	{
		csFileName = csFileName.Right(csFileName.GetLength() - lSlash - 1);
	}

	return csFileName;
}


/****************************************************************************************************
BOOL CALLBACK MyMonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
***************************************************************************************************/
struct MONITOR_ENUM_PARAM
{
	long	lFlags;				// Flags
	LPRECT	pVirtualRect;		// Ptr to rect that receives the results, or the src of the monitor search method
	int		iMonitor;			// Ndx to the mointor to look at, -1 for all, -or- result of the monitor search method
	int		nMonitorCount;		// Total number of monitors found, -1 for monitor search method

	/**
	 * @brief Whether pVirtualRect lies wholly outside a monitor (a shared edge counts as inside).
	 * @param monitor The monitor's rect.
	 * @return True when pVirtualRect is left of, right of, above or below the monitor.
	 */
	bool IsOutside(const RECT& monitor) const
	{
		return (pVirtualRect->right < monitor.left) ||
			(pVirtualRect->left > monitor.right) ||
			(pVirtualRect->bottom < monitor.top) ||
			(pVirtualRect->top > monitor.bottom);
	}
};
#define	MONITOR_SEARCH_METOHD	0x00000001
BOOL CALLBACK MyMonitorEnumProc(HMONITOR /*hMonitor*/, HDC /*hdcMonitor*/, LPRECT lprcMonitor, LPARAM dwData)
{
	// Typecast param
	MONITOR_ENUM_PARAM* pParam = (MONITOR_ENUM_PARAM*)dwData;
	if(pParam)
	{
		// If a dest rect was passed
		if(pParam->pVirtualRect)
		{
			// If MONITOR_SEARCH_METOHD then we are being asked for the index of the monitor
			// that the rect falls inside of
			if(pParam->lFlags & MONITOR_SEARCH_METOHD)
			{
				if(!pParam->IsOutside(*lprcMonitor))
				{
					// This is the one
					pParam->iMonitor = pParam->nMonitorCount;

					// Stop the enumeration
					return FALSE;
				}
			}
			else
			{
				if(pParam->iMonitor == pParam->nMonitorCount)
				{
					*pParam->pVirtualRect = *lprcMonitor;
				}
				else
					if(pParam->iMonitor == -1)
					{
						pParam->pVirtualRect->left = min(pParam->pVirtualRect->left, lprcMonitor->left);
						pParam->pVirtualRect->top = min(pParam->pVirtualRect->top, lprcMonitor->top);
						pParam->pVirtualRect->right = max(pParam->pVirtualRect->right, lprcMonitor->right);
						pParam->pVirtualRect->bottom = max(pParam->pVirtualRect->bottom, lprcMonitor->bottom);
					}
			}
		}
		
		// Up the count if necessary
		pParam->nMonitorCount++;
	}
	return TRUE;
}

// Every supported Windows is NT. The former GetVersionEx check passed an OSVERSIONINFO without
// dwOSVersionInfoSize set, so it failed or read stack garbage and picked a branch by chance.
int GetScreenWidth(void)
{
	const int width{ GetSystemMetrics(SM_CXSCREEN) };
	const int height{ GetSystemMetrics(SM_CYSCREEN) };
	switch(width)
	{
	default: // also 640, 800 and 1024
		return(width);
	case 1280:
		if(height == 480)
		{
			return(width / 2);
		}
		return(width);
	case 1600:
		if(height == 600)
		{
			return(width / 2);
		}
		return(width);
	case 2048:
		if(height == 768)
		{
			return(width / 2);
		}
		return(width);
	}
}

int GetScreenHeight(void)
{
	const int width{ GetSystemMetrics(SM_CXSCREEN) };
	const int height{ GetSystemMetrics(SM_CYSCREEN) };
	switch(height)
	{
	default: // also 480, 600 and 768
		return(height);
	case 960:
		if(width == 640)
		{
			return(height / 2);
		}
		return(height);
	case 1200:
		if(width == 800)
		{
			return(height / 2);
		}
		return(height);
	case 1536:
		if(width == 1024)
		{
			return(height / 2);
		}
		return(height);
	}
}

/*------------------------------------------------------------------*\
ID based Globals
\*------------------------------------------------------------------*/

long NewGroupID(int parentID, CString text)
{
	long lID=0;
	CTime time;
	time = CTime::GetCurrentTime();
	
	try
	{
		if(text.IsEmpty())
			text = time.Format("NewGroup %y/%m/%d %H:%M:%S");

		// bound values: the name is stored as typed (no quote doubling) and the time keeps 64 bits
		CppSQLite3Statement insert = theApp.m_db.compileStatement(
			_T("insert into Main (lDate, mText, lDontAutoDelete, bIsGroup, lParentID, stickyClipOrder, stickyClipGroupOrder) values(?, ?, ?, 1, ?, -(2147483647), -(2147483647));"));
		insert.bindInt64(1, time.GetTime());
		insert.bind(2, text);
		insert.bindInt64(3, time.GetTime());
		insert.bind(4, parentID);

		lID = (long)theApp.m_db.InsertReturningId(insert);
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Creating the group %s failed: %s"), text.GetString(), e.errorMessage()));
		return 0;
	}
	
	return lID;
}

BOOL DeleteAllIDs()
{
	try
	{
		theApp.m_db.execDML(_T("DELETE FROM Data;"));
		theApp.m_db.execDML(_T("DELETE FROM Main;"));
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Deleting all clips failed: %s"), e.errorMessage()));
		return FALSE;
	}

	return TRUE;
}

BOOL DeleteFormats(int parentID, ARRAY& formatIDs)
{	
	if(formatIDs.GetSize() <= 0)
		return TRUE;
		
	try
	{
		//Delete the requested data formats
		INT_PTR count = formatIDs.GetSize();
		for(int i = 0; i < count; i++)
		{
			theApp.m_db.execDMLEx(_T("DELETE FROM Data WHERE lID = %d;"), formatIDs[i]);
		}

		CClip clip;
		if(clip.LoadFormats(parentID))
		{
			DWORD CRC = clip.GenerateCRC();

			//Update the main table with new size
			theApp.m_db.execDMLEx(_T("UPDATE Main SET CRC = %d WHERE lID = %d"), CRC, parentID);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Deleting the selected formats of clip %d failed: %s"), parentID, e.errorMessage()));
		return FALSE;
	}
		
	return TRUE;
}

CRect CenterRect(CRect startingRect)
{
	CRect crMonitor;

	HMONITOR monitorHandle = MonitorFromPoint(startingRect.TopLeft(), MONITOR_DEFAULTTONEAREST);
	if (monitorHandle == NULL)
	{
		monitorHandle = MonitorFromPoint(startingRect.TopLeft(), MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO lpmi;
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}
	else
	{
		MONITORINFO lpmi;
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}

	return CenterRectFromRect(startingRect, crMonitor);
}

CRect CenterRectFromRect(CRect startingRect, CRect outerRect)
{
	CPoint center = outerRect.CenterPoint();

	CRect centerRect;

	centerRect.left = center.x - (startingRect.Width() / 2);
	centerRect.top = center.y - (startingRect.Height() / 2);
	centerRect.right = centerRect.left + startingRect.Width();
	centerRect.bottom = centerRect.top + startingRect.Height();

	return centerRect;
}

CRect DefaultMonitorRect()
{
	CRect crMonitor;
	CRect invalidRect(INT32_MAX, INT32_MAX, INT32_MAX, INT32_MAX);
	HMONITOR monitorHandle = MonitorFromPoint(invalidRect.TopLeft(), MONITOR_DEFAULTTOPRIMARY);
	MONITORINFO lpmi;
	lpmi.cbSize = sizeof(MONITORINFO);
	if (GetMonitorInfo(monitorHandle, &lpmi))
	{
		crMonitor.CopyRect(&lpmi.rcWork);
	}

	return crMonitor;
}

CRect MonitorRectFromRect(CRect rect)
{
	CRect crMonitor;

	HMONITOR monitorHandle = MonitorFromPoint(rect.TopLeft(), MONITOR_DEFAULTTONEAREST);
	if (monitorHandle == NULL)
	{
		monitorHandle = MonitorFromPoint(rect.TopLeft(), MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO lpmi;
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}
	else
	{
		MONITORINFO lpmi;
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}

	return crMonitor;
}

BOOL EnsureWindowVisible(CRect *pcrRect)
{
	BOOL ret = FALSE;

	CRect crMonitor;

	HMONITOR monitorHandle = MonitorFromRect(pcrRect, MONITOR_DEFAULTTONEAREST);
	if (monitorHandle == NULL)
	{
		monitorHandle = MonitorFromRect(pcrRect, MONITOR_DEFAULTTOPRIMARY);
		MONITORINFO lpmi;
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);

			*pcrRect = CenterRectFromRect(*pcrRect, crMonitor);
		}
	}
	else
	{
		MONITORINFO lpmi;
		lpmi.cbSize = sizeof(MONITORINFO);
		if (GetMonitorInfo(monitorHandle, &lpmi))
		{
			crMonitor.CopyRect(&lpmi.rcWork);
		}
	}	

	/** @brief One axis of a rect: its low edge (left or top) and its high edge (right or bottom). */
	struct Axis
	{
		/** @brief The low edge. */
		LONG RECT::* low{};
		/** @brief The high edge. */
		LONG RECT::* high{};
	};
	// horizontal first (left, right), then vertical (top, bottom)
	static constexpr std::array<Axis, 2> axes{ { { &RECT::left, &RECT::right }, { &RECT::top, &RECT::bottom } } };
	for (const Axis& axis : axes)
	{
		bool movedLow = false;
		//Validate the left (top)
		long lDiff = (*pcrRect).*axis.low - crMonitor.*axis.low;
		if (lDiff < 0)
		{
			(*pcrRect).*axis.low += abs(lDiff);
			(*pcrRect).*axis.high += abs(lDiff);
			ret = TRUE;
			movedLow = true;
		}

		//Right side (bottom)
		lDiff = (*pcrRect).*axis.high - crMonitor.*axis.high;
		if (lDiff > 0)
		{
			if (movedLow == false)
			{
				(*pcrRect).*axis.low -= abs(lDiff);
			}
			(*pcrRect).*axis.high -= abs(lDiff);
			ret = TRUE;
		}
	}

	return ret;
}

__int64 GetLastWriteTime(const CString &csFile)
{
	__int64 nLastWrite = 0;
	CFileFind finder;
	BOOL bResult = finder.FindFile(csFile);

	if (bResult)
	{
		finder.FindNextFile();

		FILETIME ft;
		finder.GetLastWriteTime(&ft);

		memcpy(&nLastWrite, &ft, sizeof(ft));
	}

	return nLastWrite;
}

typedef struct 
{
	DWORD ownerpid;
	DWORD childpid;
} windowinfo;

BOOL CALLBACK EnumChildWindowsCallback(HWND hWnd, LPARAM lp) 
{
	windowinfo* info = (windowinfo*)lp;
	DWORD pid = 0;
	GetWindowThreadProcessId(hWnd, &pid);
	if (pid != info->ownerpid) 
		info->childpid = pid;
	return TRUE;
}

CString UWP_AppName(HWND active_window, DWORD ownerpid)
{
	CString uwpAppName;
	windowinfo info = { 0 };
	info.ownerpid = ownerpid;
	info.childpid = info.ownerpid;
	EnumChildWindows(active_window, EnumChildWindowsCallback, (LPARAM)&info);
	HANDLE active_process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, info.childpid);
	if (active_process != NULL)
	{
		WCHAR image_name[MAX_PATH] = { 0 };
		DWORD bufsize = MAX_PATH;
		QueryFullProcessImageName(active_process, 0, image_name, &bufsize);
		CloseHandle(active_process);

		nsPath::CPath path(image_name);
		uwpAppName = path.GetName();
	}

	return uwpAppName;
}

CString GetProcessName(HWND hWnd, DWORD processId) 
{
	ULONGLONG startTick = GetTickCount64();

	CString	strProcessName;
	DWORD Id = processId;
	if (Id == 0)
	{		
		GetWindowThreadProcessId(hWnd, &Id);
	}

	HANDLE active_process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Id);
	if (active_process != NULL)
	{
		WCHAR image_name[MAX_PATH] = { 0 };
		DWORD bufsize = MAX_PATH;
		QueryFullProcessImageName(active_process, 0, image_name, &bufsize);
		CloseHandle(active_process);

		nsPath::CPath path(image_name);
		strProcessName = path.GetName();
	}

	if (strProcessName == _T(""))
	{
		Log(StrF(_T("failed to get process name from open process, LastError: %d, looping over process names to find process"), GetLastError()));

		PROCESSENTRY32 processEntry = { 0 };

		HANDLE hSnapShot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		processEntry.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(hSnapShot, &processEntry))
		{
			do
			{
				if (processEntry.th32ProcessID == Id)
				{
					strProcessName = processEntry.szExeFile;
					break;
				}
			} while (Process32Next(hSnapShot, &processEntry));
		}

		CloseHandle(hSnapShot);
	}

	//uwp apps are wrapped in another app called, if this has focus then try and find the child uwp process
	if (strProcessName == _T("ApplicationFrameHost.exe"))
	{
		strProcessName = UWP_AppName(hWnd, Id);
	}

	ULONGLONG endTick = GetTickCount64();
	ULONGLONG diff = endTick - startTick;
	if(diff > 5)
	{
		Log(StrF(_T("GetProcessName Time (ms): %llu, pid: %d, name: %s"), diff, Id, strProcessName.GetString()));
	}

	return strProcessName;
}

bool IsRunningLimited()
{
	LPCTSTR pszSubKey = _T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Policies\\System");
	LPCTSTR pszValue = _T("EnableLUA");
	DWORD dwType = 0;
	DWORD dwValue = 0;
	DWORD dwValueSize = sizeof(DWORD);

	if(ERROR_SUCCESS != SHGetValue(HKEY_LOCAL_MACHINE, pszSubKey, pszValue, &dwType, &dwValue, &dwValueSize))
	{
		//failed to read the reg key: assume we don't have access and we are running as a limited app
		OutputDebugString(_T("Ditto - Failed to read registry entry finding UAC, Running as limited application"));
		return true;
	}

	if(dwValue == 1)
	{
		OutputDebugString(_T("Ditto - UAC ENABLED, Running as limited application"));
		return true;
	}

	OutputDebugString(_T("Ditto - Running as standard application"));	
	return false;
}

void DeleteDittoTempFiles(BOOL checkFileLastAccess)
{
	CString csDir = CGetSetOptions::GetPath(PATH_REMOTE_FILES);
	if (FileExists(csDir))
	{
		DeleteFolderFiles(csDir, checkFileLastAccess, CTimeSpan(0, 1, 0, 0));
	}

	csDir = CGetSetOptions::GetPath(PATH_DRAG_FILES);
	if (FileExists(csDir))
	{
		DeleteFolderFiles(csDir, checkFileLastAccess, CTimeSpan(0, 1, 0, 0));
	}

	csDir = CGetSetOptions::GetPath(PATH_CLIP_DIFF);
	if (FileExists(csDir))
	{
		DeleteFolderFiles(csDir, checkFileLastAccess, CTimeSpan(0, 1, 0, 0));
	}
}

void DeleteFolderFiles(CString csDir, BOOL checkFileLastAccess, CTimeSpan lastAccessOffset)
{
	// only Ditto's own temp folders are emptied
	static constexpr std::array<const TCHAR*, 4> tempFolderMarkers{ _T("\\ReceivedFiles\\"), _T("\\DragFiles\\"), _T("ClipCompare"), _T("EditClips") };
	if (std::none_of(tempFolderMarkers.begin(), tempFolderMarkers.end(), [&csDir](const TCHAR* marker) { return csDir.Find(marker) != -1; }))
		return;

	Log(StrF(_T("Deleting files in Folder %s Check Last Access %d"), csDir.GetString(), checkFileLastAccess));

	FIX_CSTRING_PATH(csDir);

	CTime ctOld = CTime::GetCurrentTime();
	CTime ctFile;
	ctOld -= lastAccessOffset;

	CFileFind Find;

	CString csFindString;
	csFindString.Format(_T("%s*.*"), csDir.GetString());

	BOOL bFound = Find.FindFile(csFindString);
	while(bFound)
	{
		bFound = Find.FindNextFile();

		if(Find.IsDots())
			continue;

		if(checkFileLastAccess &&
			Find.GetLastAccessTime(ctFile))
		{
			//Delete the remote copied file if it hasn't been used for the last day
			if(ctFile < ctOld)
			{
				Log(StrF(_T("Deleting temp file %s"), Find.GetFilePath().GetString()));
				DeleteFile(Find.GetFilePath());
			}
		}
		else
		{
			Log(StrF(_T("Deleting temp file %s"), Find.GetFilePath().GetString()));
			DeleteFile(Find.GetFilePath());
		}
	}
}

__int64 FileSize(const TCHAR *fileName)
{
	struct _stat64  buf;
	if (_wstat64((wchar_t const*)fileName, &buf) != 0)
		return -1; // error, could use errno to find out more

	return buf.st_size;
}

int FindNoCaseAndInsert(CString& mainStr, CString& findStr, CString preInsert, CString postInsert, int linesPerRow)
{
	return CMarkerInserter::Insert(mainStr, findStr, preInsert, postInsert, linesPerRow);
}

int CMarkerInserter::Insert(CString& mainStr, CString& findStr, CString preInsert, CString postInsert, int linesPerRow)
{
	int replaceCount = 0;

	//Prevent infinite loop when user tries to replace nothing.
	if (findStr != "")
	{
		const InsertResult inserted{ InsertMarkers(mainStr, findStr, preInsert, postInsert) };
		replaceCount = inserted.replaceCount;

		TrimLeadingLines(mainStr, inserted.firstFindPos, linesPerRow);

		if(replaceCount > 0)
		{
			//use unprintable characters so it doesn't find copied html to convert
			mainStr.Replace(_T("\r\n"), _T("\x01\x05\x02"));
			mainStr.Replace(_T("\r"), _T("\x01\x05\x02"));
			mainStr.Replace(_T("\n"), _T("\x01\x05\x02"));
		}
	}

	return replaceCount;
}

CMarkerInserter::InsertResult CMarkerInserter::InsertMarkers(CString& mainStr, CString& findStr, const CString& preInsert, const CString& postInsert)
{
	InsertResult result{};

	int oldLen = findStr.GetLength();

	int foundPos = 0;
	int startFindPos = 0;
	int newPos = 0;
	int insertedLength = 0;

	CString mainLow(theApp.m_icuString.ToLowerStringEx(mainStr));
	CString findLow(theApp.m_icuString.ToLowerStringEx(findStr));
	findLow.MakeLower();

	int preLength = preInsert.GetLength();
	int postLength = postInsert.GetLength();

	while(TRUE)
	{
		foundPos = mainLow.Find(findLow, startFindPos);
		if (foundPos < 0)
			break;

		if (result.replaceCount == 0)
		{
			result.firstFindPos = foundPos + preLength;
		}

		newPos = foundPos + insertedLength;

		mainStr.Insert(newPos, preInsert);
		mainStr.Insert(newPos + preLength + oldLen, postInsert);

		startFindPos = foundPos + oldLen;

		insertedLength += preLength + postLength;

		result.replaceCount++;

		//safety check, make sure we don't look forever
		if (result.replaceCount > 100)
			break;
	}

	return result;
}

void CMarkerInserter::TrimLeadingLines(CString& mainStr, int firstFindPos, int linesPerRow)
{
	int foundPos = 0;
	int startFindPos = 0;
	int line = 0;
	int prevLinePos = 0;
	int prevPrevLinePos = 0;

	while (TRUE)
	{
		foundPos = mainStr.Find(_T("\n"), startFindPos);
		if (foundPos < 0)
			break;

		if (firstFindPos < foundPos)
		{
			if (line > linesPerRow - 1)
			{
				int lineStart = prevLinePos;
				if (linesPerRow > 1)
				{
					lineStart = prevPrevLinePos;
				}

				mainStr = _T("... ") + mainStr.Mid(lineStart + 1);
			}

			break;
		}

		startFindPos = foundPos + 1;
		prevPrevLinePos = prevLinePos;
		prevLinePos = foundPos;

		line++;

		//safety check, make sure we don't look forever
		if (line > 1000)
			break;
	}
}

void OnInitMenuPopupEx(CMenu *pPopupMenu, UINT /*nIndex*/, BOOL /*bSysMenu*/, CWnd *pWnd)
{
	CMenuPopupUpdater::Update(pPopupMenu, pWnd);
}

void CMenuPopupUpdater::Update(CMenu *pPopupMenu, CWnd *pWnd)
{
	ASSERT(pPopupMenu != NULL);
	// Check the enabled state of various menu items.

	CCmdUI state;
	state.m_pMenu = pPopupMenu;
	ASSERT(state.m_pOther == NULL);
	ASSERT(state.m_pParentMenu == NULL);

	FindParentMenu(state, pPopupMenu, pWnd);

	state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
		state.m_nIndex++)
	{
		UpdateItem(state, pPopupMenu, pWnd);
	}
}

void CMenuPopupUpdater::FindParentMenu(CCmdUI& state, CMenu *pPopupMenu, CWnd *pWnd)
{
	// Determine if menu is popup in top-level menu and set m_pOther to
	// it if so (m_pParentMenu == NULL indicates that it is secondary popup).
	HMENU hParentMenu{};
	if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu)
	{
		state.m_pParentMenu = pPopupMenu;    // Parent == child for tracking popup.
	}
	else if ((hParentMenu = ::GetMenu(pWnd->m_hWnd)) != NULL)
	{
		CWnd* pParent = pWnd;
		// Child windows don't have menus--need to go to the top!
		if (pParent != NULL &&
			(hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
		{
			int nIndexMax = ::GetMenuItemCount(hParentMenu);
			for (int nMenuIndex = 0; nMenuIndex < nIndexMax; nMenuIndex++)
			{
				if (::GetSubMenu(hParentMenu, nMenuIndex) == pPopupMenu->m_hMenu)
				{
					// When popup is found, m_pParentMenu is containing menu.
					state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
					break;
				}
			}
		}
	}
}

void CMenuPopupUpdater::UpdateItem(CCmdUI& state, CMenu *pPopupMenu, CWnd *pWnd)
{
	state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
	if (state.m_nID == 0)
		return; // Menu separator or invalid cmd - ignore it.

	ASSERT(state.m_pOther == NULL);
	ASSERT(state.m_pMenu != NULL);
	if (state.m_nID == (UINT)-1)
	{
		// Possibly a popup menu, route to first item of that popup.
		state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
		if (state.m_pSubMenu == NULL ||
			(state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
			state.m_nID == (UINT)-1)
		{
			return;       // First item of popup can't be routed to.
		}
		state.DoUpdate(pWnd, TRUE);   // Popups are never auto disabled.
	}
	else
	{
		// Normal menu item.
		// Auto enable/disable if frame window has m_bAutoMenuEnable
		// set and command is _not_ a system command.
		state.m_pSubMenu = NULL;
		state.DoUpdate(pWnd, FALSE);
	}

	AdjustForMenuChanges(state, pPopupMenu);
}

void CMenuPopupUpdater::AdjustForMenuChanges(CCmdUI& state, CMenu *pPopupMenu)
{
	// Adjust for menu deletions and additions.
	UINT nCount = pPopupMenu->GetMenuItemCount();
	if (nCount < state.m_nIndexMax)
	{
		state.m_nIndex -= (state.m_nIndexMax - nCount);
		while (state.m_nIndex < nCount &&
			pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
		{
			state.m_nIndex++;
		}
	}
	state.m_nIndexMax = nCount;
}

CString NewGuidString()
{
	CString guidString;

	GUID guid{};
	const HRESULT hr = CoCreateGuid(&guid);
	if (FAILED(hr))
	{
		throw std::runtime_error("CoCreateGuid failed with HRESULT " + std::to_string(hr));
	}
	guidString.Format(_T("%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"),
		guid.Data1, guid.Data2, guid.Data3,
		guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
		guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	return guidString.MakeLower();
}

CString FolderPath(int folderId)
{
	CString folder = _T("");
	if (folderId > 0)
	{
		try
		{
			CStringArray arr;
			for (int i = 0; i < 100; i++)
			{
				CppSQLite3Query parent = theApp.m_db.execQueryEx(_T("SELECT lID, mText, lParentID FROM Main WHERE lID = %d"), folderId);
				if (parent.eof() == false)
				{
					arr.Add(parent.getStringField(_T("mText")));
					folderId = parent.getIntField(_T("lParentID"));
				}
				else
				{
					break;
				}
			}

			folder = _T("Group Path: \\");
			for (INT_PTR folderPos = arr.GetCount() - 1; folderPos >= 0; folderPos--)
			{
				folder += _T("\\");
				folder += arr[folderPos];
			}
		}
		catch (CppSQLite3Exception& e)
		{
			CErrorReport::Show(StrF(_T("Reading the group path of group %d failed: %s"), folderId, e.errorMessage()));
			return _T("");
		}
	}

	return folder;
}

BOOL DarkAppWindows10Setting()
{
	BOOL darkMode = false;
	HKEY hkKey;
	long lResult = ::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), NULL, KEY_READ, &hkKey);
	if (lResult == ERROR_SUCCESS)
	{
		DWORD buffer;
		DWORD len = sizeof(buffer);
		DWORD type;

		lResult = ::RegQueryValueEx(hkKey, _T("AppsUseLightTheme"), 0, &type, (LPBYTE)&buffer, &len);

		if (lResult == ERROR_SUCCESS)
		{
			darkMode = (buffer == 0);
		}

		RegCloseKey(hkKey);
	}

	return darkMode;
}

DWORD Windows10AccentColor()
{
	DWORD color = MAXDWORD;
	HKEY hkKey;
	long lResult = ::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\DWM"), NULL, KEY_READ, &hkKey);
	if (lResult == ERROR_SUCCESS)
	{
		DWORD buffer;
		DWORD len = sizeof(buffer);
		DWORD type;

		lResult = ::RegQueryValueEx(hkKey, _T("ColorizationColor"), 0, &type, (LPBYTE)&buffer, &len);

		if (lResult == ERROR_SUCCESS)
		{
			color = buffer;
		}

		RegCloseKey(hkKey);
	}

	return color;
}

BOOL Windows10ColorTitleBar()
{
	BOOL colorTitleBar = FALSE;
	HKEY hkKey;
	long lResult = ::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\DWM"), NULL, KEY_READ, &hkKey);
	if (lResult == ERROR_SUCCESS)
	{
		DWORD buffer;
		DWORD len = sizeof(buffer);
		DWORD type;

		lResult = ::RegQueryValueEx(hkKey, _T("ColorPrevalence"), 0, &type, (LPBYTE)&buffer, &len);

		if (lResult == ERROR_SUCCESS)
		{
			colorTitleBar = (buffer == 1);
		}

		RegCloseKey(hkKey);
	}

	return colorTitleBar;
}

BOOL RestoreDbPrompt(HWND hwnd)
{
	BOOL ret = false;

	OPENFILENAME ofn;
	TCHAR szFile[400];
	TCHAR szDir[400];

	memset(&szFile, 0, sizeof(szFile));
	memset(szDir, 0, sizeof(szDir));
	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = _T("Ditto database backups (.zdb)\0*.zdb\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	//ofn.lpstrInitialDir = szDir;
	ofn.lpstrDefExt = _T("zdb");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	if (GetOpenFileName(&ofn))
	{
		CWaitCursor wait;

		CString dbPath = CGetSetOptions::GetDBPath();
		CString backupPath(CFileDialogPath::From(ofn));
		ret = RestoreDB(backupPath);
	}

	return ret;
}

BOOL BackupDbPrompt(HWND hwnd)
{
	BOOL ret = FALSE;

	OPENFILENAME ofn;
	TCHAR szFile[400];
	TCHAR szDir[400];

	memset(&szFile, 0, sizeof(szFile));
	memset(szDir, 0, sizeof(szDir));
	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = _T("Ditto database backups (.zdb)\0*.zdb\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrDefExt = _T("zdb");
	// a save dialog: the file may be new (no OFN_FILEMUSTEXIST)
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	if (GetSaveFileName(&ofn))
	{
		CWaitCursor wait;

		CString dbPath = CGetSetOptions::GetDBPath();
		CString backupPath(CFileDialogPath::From(ofn));
		ret = BackupDB(dbPath, backupPath);
	}

	return ret;
}

int WordCount(const CString &text)
{
	constexpr int outsideWord = 0;
	constexpr int insideWord = 1;

	int state = outsideWord;
	unsigned wc = 0; // word count

	// Scan all characters one by one
	for(int pos = 0; pos < text.GetLength(); pos++)	
	{
		auto str = text[pos];
		
		if (str == ' ' || str == '\r' || str == '\n' || str == '\t')
		{
			state = outsideWord;
		}
		else if (state == outsideWord)
		{
			state = insideWord;
			wc++;
		}
	}

	return wc;
}

CString GetVersionString(VersionInfo version)
{
	CString csLine;
	csLine.Format(_T("%02i.%02i.%02i.%02i"),
		version.Major,
		version.Minor,
		version.Revision,
		version.Build);

	return csLine;
}

VersionInfo GetRunningVersion()
{
	// Ditto.exe always carries a version resource: not finding it means a broken build
	const CString csFileName = CGetSetOptions::GetExeFileName();

	DWORD dwHandle{};
	const DWORD dwSize = GetFileVersionInfoSize(csFileName, &dwHandle);
	if (dwSize == 0)
	{
		throw std::runtime_error("Ditto.exe has no version resource (GetFileVersionInfoSize error " + std::to_string(::GetLastError()) + ")");
	}

	std::vector<BYTE> data(dwSize);
	// The handle parameter of GetFileVersionInfo is ignored and must be 0.
	if (GetFileVersionInfo(csFileName, 0, dwSize, data.data()) == 0)
	{
		throw std::runtime_error("reading Ditto.exe's version resource failed (error " + std::to_string(::GetLastError()) + ")");
	}

	VS_FIXEDFILEINFO* lpFFI{};
	UINT iBuffSize{};
	if (VerQueryValue(data.data(), _T("\\"), reinterpret_cast<LPVOID*>(&lpFFI), &iBuffSize) == 0 || iBuffSize < sizeof(VS_FIXEDFILEINFO))
	{
		throw std::runtime_error("Ditto.exe's version resource has no fixed file info");
	}

	VersionInfo verInfo;
	verInfo.Major = (lpFFI->dwProductVersionMS >> 16) & 0xffff;
	verInfo.Minor = (lpFFI->dwProductVersionMS >> 0) & 0xffff;
	verInfo.Revision = (lpFFI->dwProductVersionLS >> 16) & 0xffff;
	verInfo.Build = (lpFFI->dwProductVersionLS >> 0) & 0xffff;
	return verInfo;
}

#pragma once

#include <shlwapi.h>

#include "GlobalBytes.h"
#include "ClipboardFormatError.h"

#include <array>
#include <new>
#include <string>

#define DITTO_ADD_IN_VERSION 1

typedef enum
{
	eFuncType_PRE_PASTE
}FunctionType;

class CFunction
{
public:
	CStringA m_csFunction;
	CString m_csDisplayName;
	CString m_csDetailDescription;
};

class CDittoAddinInfo
{
public:
	CDittoAddinInfo()
	{
		m_nPrivateVersion = DITTO_ADD_IN_VERSION;
		m_AddinVersion = 0;
		m_nSizeOfThis = sizeof(CDittoAddinInfo);
	}
	
	bool ValidateSize() const  { return m_nSizeOfThis == sizeof(CDittoAddinInfo); }
	int PrivateVersion() const { return m_nPrivateVersion; }

	int m_nSizeOfThis;
	CString m_Name;
	int m_AddinVersion;

private:
	int m_nPrivateVersion;
};

class CDittoInfo
{
public:
	CDittoInfo()
	{
		m_nPrivateVersion = DITTO_ADD_IN_VERSION;
		m_nVersion = 0;
		m_hWndDitto = NULL;
		m_nSizeOfThis = sizeof(CDittoInfo);
	}

	bool ValidateSize() const  { return m_nSizeOfThis == sizeof(CDittoInfo); }
	int PrivateVersion() const { return m_nPrivateVersion; }

	int m_nSizeOfThis;
	int m_nVersion;
	CString m_csSqliteVersion;
	CString m_csLanguageCode; //http://www.loc.gov/standards/iso639-2/php/code_list.php
	CString m_csDatabasePath;
	HWND m_hWndDitto;

private:
	int m_nPrivateVersion;
};

class DittoAddinHelpers
{
public:
	// Copies ulBufLen bytes into hDest; throws when hDest is not a lockable block of at least that size
	static void CopyToGlobalHP(HGLOBAL hDest, LPVOID pBuf, ULONG ulBufLen)
	{
		DittoCore::GlobalBytes dest(hDest);
		if (pBuf == nullptr || ulBufLen > dest.WritableBytes().size())
		{
			throw DittoCore::ClipboardFormatError("copy of " + std::to_string(ulBufLen) + " bytes does not fit a block of " + std::to_string(dest.WritableBytes().size()));
		}
		memcpy(dest.WritableBytes().data(), pBuf, ulBufLen);
	}

	static HGLOBAL NewGlobalP(LPVOID pBuf, UINT nLen)
	{
		HGLOBAL hDest = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, nLen);
		if (hDest == nullptr)
		{
			throw std::bad_alloc();
		}
		CopyToGlobalHP(hDest, pBuf, nLen);
		return hDest;
	}


	//Do not change these these are stored in the database
	static CLIPFORMAT GetFormatID(LPCTSTR cbName)
	{
		for (const StandardFormat& standardFormat : s_standardFormats)
		{
			if (standardFormat.parsedFromName && StrCmp(cbName, standardFormat.name) == 0)
				return standardFormat.format;
		}

		// Registered formats are in 0xC000-0xFFFF (0 on failure), so they fit a CLIPFORMAT
		return static_cast<CLIPFORMAT>(::RegisterClipboardFormat(cbName));
	}

	//Do not change these these are stored in the database
	static CString GetFormatName(CLIPFORMAT cbType)
	{
		for (const StandardFormat& standardFormat : s_standardFormats)
		{
			if (standardFormat.format == cbType)
				return standardFormat.name;
		}

		//Not a default type get the name from the clipboard
		if (cbType != 0)
		{
			TCHAR szFormat[256];
			GetClipboardFormatName(cbType, szFormat, 256);
			return szFormat;
		}

		return _T("ERROR");
	}

private:
	/** @brief A standard clipboard format and the name Ditto stores for it in the database. */
	struct StandardFormat
	{
		/** @brief The format id (CF_*). */
		CLIPFORMAT format{};
		/** @brief The stored name. */
		const TCHAR* name{};
		/** @brief True when GetFormatID maps the name back to the id (CF_BITMAP is only named, never parsed). */
		bool parsedFromName{};
	};

	/** @brief The standard formats, in the order GetFormatName and GetFormatID check them. Do not change: the names are stored in the database. */
	static constexpr std::array<StandardFormat, 21> s_standardFormats{ {
		{ CF_TEXT, _T("CF_TEXT"), true },
		{ CF_BITMAP, _T("CF_BITMAP"), false },
		{ CF_METAFILEPICT, _T("CF_METAFILEPICT"), true },
		{ CF_SYLK, _T("CF_SYLK"), true },
		{ CF_DIF, _T("CF_DIF"), true },
		{ CF_TIFF, _T("CF_TIFF"), true },
		{ CF_OEMTEXT, _T("CF_OEMTEXT"), true },
		{ CF_DIB, _T("CF_DIB"), true },
		{ CF_PALETTE, _T("CF_PALETTE"), true },
		{ CF_PENDATA, _T("CF_PENDATA"), true },
		{ CF_RIFF, _T("CF_RIFF"), true },
		{ CF_WAVE, _T("CF_WAVE"), true },
		{ CF_UNICODETEXT, _T("CF_UNICODETEXT"), true },
		{ CF_ENHMETAFILE, _T("CF_ENHMETAFILE"), true },
		{ CF_HDROP, _T("CF_HDROP"), true },
		{ CF_LOCALE, _T("CF_LOCALE"), true },
		{ CF_OWNERDISPLAY, _T("CF_OWNERDISPLAY"), true },
		{ CF_DSPTEXT, _T("CF_DSPTEXT"), true },
		{ CF_DSPBITMAP, _T("CF_DSPBITMAP"), true },
		{ CF_DSPMETAFILEPICT, _T("CF_DSPMETAFILEPICT"), true },
		{ CF_DSPENHMETAFILE, _T("CF_DSPENHMETAFILE"), true },
	} };
};

class AddToDbStickyEnum
{
public:
	enum AddToDbSticky
	{
		INVALID,
		MAKE_TOP_STICKY,
		MAKE_LAST_STICKY,
		REPLACE_TOP_STICKY
	};
};
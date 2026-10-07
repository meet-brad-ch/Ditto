#include "stdafx.h"
#include "ClipboardFormats.h"
#include <array>

// https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
std::vector<CLIPFORMAT> CClipboardFormats::GetSystemClipFormats()
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
CLIPFORMAT CClipboardFormats::GetFormatID(LPCTSTR cbName)
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
		if (_tcscmp(cbName, format.name) == 0)
		{
			return format.id;
		}
	}

	// Registered clipboard formats are in the range 0xC000..0xFFFF, so they fit in a CLIPFORMAT.
	return static_cast<CLIPFORMAT>(::RegisterClipboardFormat(cbName));
}

//Do not change these these are stored in the database
CString CClipboardFormats::GetFormatName(CLIPFORMAT cbType)
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

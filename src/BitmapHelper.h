// BitmapHelper.h: interface for the CBitmapHelper class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_BITMAPHELPER_H__641D941B_5487_4F85_BFC1_012F2083A8B6__INCLUDED_)
#define AFX_BITMAPHELPER_H__641D941B_5487_4F85_BFC1_012F2083A8B6__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "clip.h"

class CBitmapHelper
{
public:
	CBitmapHelper();
	virtual ~CBitmapHelper();

	/**
	 * @brief Whether a packed DIB has a Windows 3.0 style header (BITMAPINFOHEADER): its first
	 * DWORD, the header size, is sizeof(BITMAPINFOHEADER).
	 * @param lpbi The packed DIB.
	 * @return true for a BITMAPINFOHEADER DIB.
	 */
	static bool		IsWin30Dib(const void* lpbi)
	{
		return (*static_cast<const DWORD*>(lpbi)) == sizeof(BITMAPINFOHEADER);
	}

	static int		GetCBitmapWidth(const CBitmap& cbm);
	static int		GetCBitmapHeight(const CBitmap& cbm);
	/**
	 * @brief Draws a clip format's image (CF_DIB or PNG) into a new bitmap, scaled down to a
	 *        maximum height.
	 * @param settings The application's settings (the fast thumbnail mode).
	 * @param pngFormat The registered "PNG" format (CRegisteredClipboardFormats::Png()).
	 * @param pClip2 The CClipFormat that holds the image.
	 * @param pDC The device context the bitmap is made compatible with.
	 * @param pBitMap Receives the bitmap.
	 * @param nMaxHeight The maximum height.
	 * @return TRUE when the bitmap was made.
	 */
	static BOOL		GetCBitmap(CGetSetOptions& settings, CLIPFORMAT pngFormat, void* pClip2, CDC* pDC, CBitmap* pBitMap, int nMaxHeight);
	/**
	 * @brief Draws the images (CF_DIB or PNG) of clip formats side by side or stacked into a new bitmap.
	 * @param clips The formats; the ones that are not images are skipped.
	 * @param pngFormat The registered "PNG" format (CRegisteredClipboardFormats::Png()).
	 * @param pDC The device context the bitmap is made compatible with.
	 * @param pBitMap Receives the bitmap.
	 * @param horizontal TRUE: side by side; FALSE: stacked.
	 * @return TRUE when an image was drawn.
	 */
	static BOOL		GetCBitmap(CClipFormats& clips, CLIPFORMAT pngFormat, CDC* pDC, CBitmap* pBitMap, BOOL horizontal);
	static HANDLE	hBitmapToDIB(HBITMAP hBitmap, DWORD dwCompression, HPALETTE hPal);
	static WORD		PaletteSize(LPSTR lpDIB);
	static WORD		DIBNumColors(LPSTR lpDIB);
	static bool		DrawDIB(CDC* pDC, HANDLE hData, int nLeft, int nRight, int& nWidth);

private:
	// The size of the images of clips placed side by side (horizontal) or stacked; pngFormat: the registered "PNG" format.
	static CSize	MeasureImages(CClipFormats& clips, CLIPFORMAT pngFormat, BOOL horizontal);

};

#endif // !defined(AFX_BITMAPHELPER_H__641D941B_5487_4F85_BFC1_012F2083A8B6__INCLUDED_)

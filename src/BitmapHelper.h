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
	static BOOL		GetCBitmap(void* pClip2, CDC* pDC, CBitmap* pBitMap, int nMaxHeight);
	static BOOL		GetCBitmap(CClipFormats& clips, CDC* pDC, CBitmap* pBitMap, BOOL horizontal);
	static HANDLE	hBitmapToDIB(HBITMAP hBitmap, DWORD dwCompression, HPALETTE hPal);
	static WORD		PaletteSize(LPSTR lpDIB);
	static WORD		DIBNumColors(LPSTR lpDIB);
	static bool		DrawDIB(CDC* pDC, HANDLE hData, int nLeft, int nRight, int& nWidth);

private:
	// The size of the images of clips placed side by side (horizontal) or stacked.
	static CSize	MeasureImages(CClipFormats& clips, BOOL horizontal);

};

#endif // !defined(AFX_BITMAPHELPER_H__641D941B_5487_4F85_BFC1_012F2083A8B6__INCLUDED_)

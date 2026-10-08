#include "stdafx.h"
#include ".\ImageFormatAggregator.h"
#include "Misc.h"
#include "BitmapHelper.h"

#include <memory>
#include <type_traits>

CImageFormatAggregator::CImageFormatAggregator(BOOL horizontally, CLIPFORMAT pngFormat) :
	m_pngFormat(pngFormat)
{
	m_horizontally = horizontally;
}

CImageFormatAggregator::~CImageFormatAggregator(void)
{
	// the images are freed here, on every path; upstream freed them only after a successful
	// GetHGlobal, so a failed or abandoned aggregation leaked them
	int count = (int)m_images.GetCount();
	for (int i = 0; i < count; i++)
	{
		CClipFormat clip = m_images[i];
		clip.AutoDeleteData(true);
		clip.Free();
	}
}

bool CImageFormatAggregator::AddClip(LPVOID lpData, int nDataSize, int /*nPos*/, int /*nCount*/, UINT cfType)
{
	HGLOBAL hGlobal = CGlobalMemory::NewGlobalP(lpData, nDataSize);

	// Clipboard format ids are 16-bit values, so they fit a CLIPFORMAT
	CClipFormat data(static_cast<CLIPFORMAT>(cfType), hGlobal);
	//m_images owns the data now
	data.AutoDeleteData(false);

	m_images.Add(data);

	return true;
}

HGLOBAL CImageFormatAggregator::GetHGlobal()
{
	// the window DC is released on every path; upstream never released it
	const HWND window = GetActiveWindow();
	const auto releaseDc = [window](HDC dc) { ::ReleaseDC(window, dc); };
	const std::unique_ptr<std::remove_pointer_t<HDC>, decltype(releaseDc)> dc(::GetDC(window), releaseDc);

	CBitmap bitmap;
	if (CBitmapHelper::GetCBitmap(m_images, m_pngFormat, CDC::FromHandle(dc.get()), &bitmap, m_horizontally) == FALSE)
	{
		bitmap.DeleteObject();
		return NULL;
	}

	HPALETTE hPal = NULL;
	auto returnHGlobal = CBitmapHelper::hBitmapToDIB((HBITMAP)bitmap, BI_RGB, hPal);

	bitmap.DeleteObject();

	return returnHGlobal;
}

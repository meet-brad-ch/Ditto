#pragma once
#include "stdafx.h"
#include "IClipAggregator.h"
#include "Clip.h"

class CImageFormatAggregator : public IClipAggregator
{
public:
	/**
	 * @brief Creates an empty image aggregator.
	 * @param horizontally TRUE: the images are joined side by side; FALSE: stacked.
	 * @param pngFormat The registered "PNG" format (CRegisteredClipboardFormats::Png()).
	 */
	CImageFormatAggregator(BOOL horizontally, CLIPFORMAT pngFormat);
	~CImageFormatAggregator(void);

	virtual bool AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType);
	virtual HGLOBAL GetHGlobal();

protected:
	CClipFormats m_images;
	BOOL m_horizontally;
	/** @brief The registered "PNG" format. */
	CLIPFORMAT m_pngFormat{};
};

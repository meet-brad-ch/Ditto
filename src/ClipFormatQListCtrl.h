#pragma once

#include "Clip.h"

class CClipFormatQListCtrl : public CClipFormat
{
public:
	CClipFormatQListCtrl(void);
	~CClipFormatQListCtrl(void);

	int m_clipRow;
	bool m_convertedToSmallImage;
	INT64 m_counter;

	/**
	 * @brief Replaces the image data (CF_DIB or PNG) once with a DIB scaled down to a height.
	 * @param settings The application's settings (the fast thumbnail mode).
	 * @param pDc The device context used for the scaling.
	 * @param height The maximum height.
	 * @return The (scaled) DIB data; NULL for another format or when scaling failed.
	 */
	HGLOBAL GetDibFittingToHeight(CGetSetOptions& settings, CDC *pDc, int height);
};


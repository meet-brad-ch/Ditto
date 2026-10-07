#pragma once

#include <array>
#include <memory>
#include <span>

#include "CGdiPlusBitmap.h"
#include "DPI.h"

class CGdiImageDrawer
{
public:
	CGdiImageDrawer();
	~CGdiImageDrawer();

	BOOL LoadStdImage(UINT id, LPCTSTR pType);
	BOOL LoadStdImageDPI(int dpi, UINT id96, UINT id120, UINT id144, UINT id168, UINT id192, LPCTSTR pType, UINT id225 = 0, UINT id250 = 0, UINT id275 = 0, UINT id300 = 0, UINT id325 = 0, UINT id350 = 0);
	void Draw(CDC* pScreenDC, CDPI &dpi, CWnd *pWnd, int posX, int posY, bool mouseHover, bool mouseDown, int forceWidth = INT_MAX, int forceHeight = INT_MAX);
	void Draw(CDC* pScreenDC, CDPI &dpi, CWnd *pWnd, CRect rc, bool mouseHover, bool mouseDown);
	BOOL LoadRaw(unsigned char* bitmapData, int imageSize);

	UINT ImageWidth() { return m_pStdImage->m_pBitmap->GetWidth(); }
	UINT ImageHeight() { return m_pStdImage->m_pBitmap->GetHeight(); }

	void Reset();

private:
	/** @brief One candidate image of LoadStdImageDPI: used when the DPI is at least minDpi. */
	struct DpiImageChoice
	{
		/** @brief Lowest DPI that uses this image. */
		int minDpi{};
		/** @brief Resource id of the image. */
		UINT id{};
		/** @brief true if the image is optional: an id of 0 skips this entry. */
		bool optional{};
	};

	/** @brief Picks the resource id of the first matching DPI choice.
	 *  @param dpi Current DPI.
	 *  @param choices Candidates, largest DPI first; the last one must always match.
	 *  @return The chosen resource id (0 if no entry matches). */
	static UINT PickDpiImageId(int dpi, std::span<const DpiImageChoice> choices);

protected:
	/** @brief The loaded image (owned); empty until a Load call or after Reset. */
	std::unique_ptr<CGdiPlusBitmapResource> m_pStdImage{};
	//CDC*	m_pCurBtn;		// current pointer to one of the above
	//CDC		m_dcStd;		// standard button

	//CDC m_dcBk;
};


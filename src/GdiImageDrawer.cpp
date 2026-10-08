#include "stdafx.h"
#include "GdiImageDrawer.h"
#include "MemDC.h"
#include "CP_Main.h"

CGdiImageDrawer::CGdiImageDrawer()
{
}

CGdiImageDrawer::~CGdiImageDrawer()
{
}

void CGdiImageDrawer::Reset()
{
	m_pStdImage.reset();
}

BOOL CGdiImageDrawer::LoadStdImage(UINT id, LPCTSTR pType)
{
	m_pStdImage = std::make_unique<CGdiPlusBitmapResource>();
	return m_pStdImage->Load(id, pType);
}

BOOL CGdiImageDrawer::LoadRaw(unsigned char* bitmapData, int imageSize)
{
	m_pStdImage = std::make_unique<CGdiPlusBitmapResource>();
	return m_pStdImage->LoadRaw(bitmapData, imageSize);
}

BOOL CGdiImageDrawer::LoadStdImageDPI(int dpi, UINT id96, UINT id120, UINT id144, UINT id168, UINT id192, LPCTSTR pType, UINT id225, UINT id250, UINT id275, UINT id300, UINT id325, UINT id350)
{
	// first entry that matches wins; the large sizes are optional (id 0 = not given)
	const std::array<DpiImageChoice, 11> choices{ { { 336, id350, true },
													{ 312, id325, true },
													{ 288, id300, true },
													{ 264, id275, true },
													{ 240, id250, true },
													{ 216, id225, true },
													{ 192, id192, false },
													{ 168, id168, false },
													{ 144, id144, false },
													{ 120, id120, false },
													{ INT_MIN, id96, false } } };

	BOOL ret = LoadStdImage(PickDpiImageId(dpi, choices), pType);

	return ret;
}

UINT CGdiImageDrawer::PickDpiImageId(int dpi, std::span<const DpiImageChoice> choices)
{
	UINT id = 0;
	for (const DpiImageChoice& choice : choices)
	{
		if (dpi >= choice.minDpi && (!choice.optional || choice.id != 0))
		{
			id = choice.id;
			break;
		}
	}
	return id;
}

void CGdiImageDrawer::Draw(CDC* pScreenDC, CDPI& dpi, CWnd* pWnd, CRect rc, bool mouseHover, bool mouseDown)
{
	int width = m_pStdImage->m_pBitmap->GetWidth();
	int height = m_pStdImage->m_pBitmap->GetHeight();

	int x = rc.left + (rc.Width() / 2) - (width / 2);
	int y = rc.top + (rc.Height() / 2) - (height / 2);

	Draw(pScreenDC, dpi, pWnd, x, y, mouseHover, mouseDown);
}

void CGdiImageDrawer::Draw(CDC* pScreenDC, CDPI& dpi, CWnd* pWnd, int posX, int posY, bool /*mouseHover*/, bool mouseDown, int forceWidth, int forceHeight)
{
	int width = m_pStdImage->m_pBitmap->GetWidth();
	if (forceWidth != INT_MAX)
		width = forceWidth;
	int height = m_pStdImage->m_pBitmap->GetHeight();
	if (forceHeight != INT_MAX)
		height = forceHeight;

	CRect rectWithBorder(posX, posY, posX + width, posY + height);

	CDC dcBk;
	CBitmap bmp;
	CClientDC clDC(pWnd);

	//Copy the background over the entire area
	dcBk.CreateCompatibleDC(&clDC);
	bmp.CreateCompatibleBitmap(&clDC, 1, 1);
	dcBk.SelectObject(&bmp);
	dcBk.BitBlt(0, 0, 1, 1, &clDC, rectWithBorder.left - 1, rectWithBorder.top, SRCCOPY);

	bmp.DeleteObject();

	//Draw the png file
	if (mouseDown)
	{
		int one = dpi.Scale(1);
		posX += one;
		posY += one;
	}

	//ImageAttributes ia;
	//

	//ColorMap blackToRed;
	//blackToRed.oldColor = Color(255, 110, 114, 122);  // black
	//blackToRed.newColor = Color(255, 255, 0, 0);// red
	//ia.SetRemapTable(1, &blackToRed);

	Gdiplus::Graphics graphics(pScreenDC->m_hDC);
	graphics.DrawImage(*m_pStdImage, posX, posY, width, height);

	//RectF grect; grect.X = posX, grect.Y = posY; grect.Width = width; grect.Height = height;
	//graphics.DrawImage(*m_pStdImage, grect, 0, 0, width, height, UnitPixel, &ia);


	//If we are hoving over then draw the border
	//if(mouseHover && mouseDown == false)
	//{
	//	pScreenDC->Draw3dRect(rectWithBorder, RGB(255, 255, 255), RGB(255, 255, 255));
	//}
}

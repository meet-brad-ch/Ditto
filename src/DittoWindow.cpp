#include "stdafx.h"
#include ".\dittowindow.h"
#include "CP_Main.h"
#include "Options.h"

CDittoWindow::CDittoWindow(void)
{
	m_captionBorderWidth = m_dpi.Scale(25);

	m_borderSize = 2;
	m_bMouseOverChevron = false;
	m_bMouseDownOnChevron = false;
	m_bMouseDownOnClose = false;
	m_bMouseOverClose = false;
	m_bMouseDownOnMinimize = false;
	m_bMouseOverMinimize = false;
	m_bMouseDownOnMaximize = false;
	m_bMouseOverMaximize = false;
	m_bDrawClose = true;
	m_bDrawChevron = true;
	m_bDrawMaximize = true;
	m_bDrawMinimize = true;
	m_bMinimized = false;
	m_crCloseBT.SetRectEmpty();
	m_crChevronBT.SetRectEmpty();
	m_crMaximizeBT.SetRectEmpty();
	m_crMinimizeBT.SetRectEmpty();
	m_CaptionColorLeft = RGB(255, 255, 255);
	m_CaptionColorRight = RGB(204, 204, 204);
	m_CaptionTextColor = RGB(191, 191, 191);
	m_border = RGB(204, 204, 204);
	m_sendWMClose = true;
	m_customWindowTitle = _T("");
	m_useCustomWindowTitle = false;
	m_buttonDownOnCaption = false;
	m_crFullSizeWindow.SetRectEmpty();	
	m_captionPosition = CGetSetOptions::CaptionOnRight;
	
}

CDittoWindow::~CDittoWindow(void)
{
}

void CDittoWindow::DoCreate(CWnd *pWnd)
{
	m_dpi.SetHwnd(pWnd->m_hWnd);
	

	m_VertFont.CreateFont(-m_dpi.Scale(19), 0, -900, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET,
							OUT_DEFAULT_PRECIS,	CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, 
							DEFAULT_PITCH|FF_SWISS, _T("Segoe UI"));

	m_HorFont.CreateFont(-m_dpi.Scale(19), 0, 0, 0, 500, FALSE, FALSE, 0, DEFAULT_CHARSET,
						OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,DEFAULT_QUALITY,
						DEFAULT_PITCH|FF_SWISS, _T("Segoe UI"));

	SetTitleTextHeight(pWnd);
	
	m_closeButton.LoadStdImageDPI(m_dpi.GetDPI(), Close_Black_16_16, Close_Black_20_20, Close_Black_24_24, Close_Black_28, Close_Black_32_32, _T("PNG"), close_36, close_40, close_44, close_48, close_52, close_56);
	m_chevronRightButton.LoadStdImageDPI(m_dpi.GetDPI(), ChevronRight_Black_16_16, ChevronRight_Black_20_20, ChevronRight_Black_24_24, ChevronRight_Black_28, ChevronRight_Black_32_32, _T("PNG"), ChevronRight_Black_36, ChevronRight_Black_40, ChevronRight_Black_44, ChevronRight_Black_48, ChevronRight_Black_52, ChevronRight_Black_56);
	m_chevronLeftButton.LoadStdImageDPI(m_dpi.GetDPI(), ChevronLeft_Black_16_16, ChevronLeft_Black_20_20, ChevronLeft_Black_24_24, ChevronLeft_Black_28, ChevronLeft_Black_32_32, _T("PNG"), ChevronLeft_Black_36, ChevronLeft_Black_40, ChevronLeft_Black_44, ChevronLeft_Black_48, ChevronLeft_Black_52, ChevronLeft_Black_56);
	m_maximizeButton.LoadStdImageDPI(m_dpi.GetDPI(), IDB_MAXIMIZE_16_16, maximize_20, maximize_24, maximize_28, maximize_32, _T("PNG"), maximize_36, maximize_40, maximize_44, maximize_48, maximize_52, maximize_56);
	m_minimizeButton.LoadStdImageDPI(m_dpi.GetDPI(), minimize_16, minimize_20, minimize_24, minimize_28, minimize_32, _T("PNG"), minimize_36, minimize_40, minimize_44, minimize_48, minimize_52, minimize_56);
	//m_windowIcon.LoadStdImageDPI(NewWindowIcon_24_14, NewWindowIcon_30, NewWindowIcon_36, NewWindowIcon_48, _T("PNG"));
}

void CDittoWindow::DoNcCalcSize(BOOL /*bCalcValidRects*/,NCCALCSIZE_PARAMS FAR* lpncsp)
{
	//Decrease the client area	
	if (m_captionPosition == CGetSetOptions::CaptionOnLeft)
		lpncsp->rgrc[0].left += m_captionBorderWidth;
	else
		lpncsp->rgrc[0].left += m_borderSize;

	if (m_captionPosition == CGetSetOptions::CaptionOnTop)
		lpncsp->rgrc[0].top += m_captionBorderWidth;
	else
		lpncsp->rgrc[0].top += m_borderSize;

	if (m_captionPosition == CGetSetOptions::CaptionOnRight)
		lpncsp->rgrc[0].right -= m_captionBorderWidth;
	else
		lpncsp->rgrc[0].right -= m_borderSize;

	if (m_captionPosition == CGetSetOptions::CaptionOnBottom)
		lpncsp->rgrc[0].bottom -= m_captionBorderWidth;
	else
		lpncsp->rgrc[0].bottom -= m_borderSize;
}

UINT CDittoWindow::DoNcHitTest(CWnd *pWnd, CPoint point) 
{
	CRect crWindow;
	pWnd->GetWindowRect(crWindow);

	if(crWindow.PtInRect(point) == false)
	{
		// Not handled: callers compare the result with -1, which is UINT_MAX as a UINT
		return UINT_MAX;
	}
	
	int x = point.x - crWindow.left;
	int y = point.y - crWindow.top;

	CPoint myLocal(x, y);

	//http://stackoverflow.com/questions/521147/the-curious-problem-of-the-missing-wm-nclbuttonup-message-when-a-window-isnt-ma
	//workaround for l button up not coming after a lbutton down
	if (IsOnCaptionButton(myLocal))
	{
		return HTBORDER;
	}

	// UINT_MAX below: the point is not on that part of the frame
	if(m_bMinimized == false)
	{
		UINT corner = HitTestTopCorners(crWindow, point);
		if (corner == UINT_MAX)
			corner = HitTestBottomCorners(crWindow, point);
		if (corner != UINT_MAX)
			return corner;
	}

	UINT edge = HitTestTopBottomEdges(crWindow, point);
	if (edge == UINT_MAX)
		edge = HitTestLeftRightEdges(crWindow, point);
	if (edge != UINT_MAX)
		return edge;

	if (IsInCaption(crWindow, point))
		return HTCAPTION;

	// Not handled: callers compare the result with -1, which is UINT_MAX as a UINT
	return UINT_MAX;
}

bool CDittoWindow::IsOnCaptionButton(const CPoint& myLocal) const
{
	return m_crCloseBT.PtInRect(myLocal) ||
		m_crChevronBT.PtInRect(myLocal) ||
		m_crMinimizeBT.PtInRect(myLocal) ||
		m_crMaximizeBT.PtInRect(myLocal);
}

UINT CDittoWindow::HitTestTopCorners(const CRect& crWindow, const CPoint& point) const
{
	if ((point.y < crWindow.top + m_borderSize * 4) &&
		(point.x < crWindow.left + m_borderSize * 4))
		return HTTOPLEFT;
	else if ((point.y < crWindow.top + m_borderSize * 4) &&
		(point.x > crWindow.right - m_borderSize * 4))
		return HTTOPRIGHT;
	return UINT_MAX;
}

UINT CDittoWindow::HitTestBottomCorners(const CRect& crWindow, const CPoint& point) const
{
	if ((point.y > crWindow.bottom - m_borderSize * 4) &&
		(point.x > crWindow.right - m_borderSize * 4))
		return HTBOTTOMRIGHT;
	else if ((point.y > crWindow.bottom - m_borderSize * 4) &&
		(point.x < crWindow.left + m_borderSize * 4))
		return HTBOTTOMLEFT;
	return UINT_MAX;
}

UINT CDittoWindow::HitTestTopBottomEdges(const CRect& crWindow, const CPoint& point) const
{
	if((((m_captionPosition == CGetSetOptions::CaptionOnTop) || (m_captionPosition == CGetSetOptions::CaptionOnBottom)) &&
		(m_bMinimized)) == false)
	{
		if (point.y < crWindow.top + m_borderSize * 2)
			return HTTOP;
		if (point.y > crWindow.bottom - m_borderSize * 2)
			return HTBOTTOM;
	}
	return UINT_MAX;
}

UINT CDittoWindow::HitTestLeftRightEdges(const CRect& crWindow, const CPoint& point) const
{
	if((((m_captionPosition == CGetSetOptions::CaptionOnLeft) || (m_captionPosition == CGetSetOptions::CaptionOnRight)) &&
		(m_bMinimized)) == false)
	{
		if (point.x > crWindow.right - m_borderSize * 2)
			return HTRIGHT;
		if (point.x < crWindow.left + m_borderSize * 2)
			return HTLEFT;
	}
	return UINT_MAX;
}

bool CDittoWindow::IsInCaption(const CRect& crWindow, const CPoint& point) const
{
	switch (m_captionPosition)
	{
	case CGetSetOptions::CaptionOnRight:
		return point.x > crWindow.right - m_captionBorderWidth;
	case CGetSetOptions::CaptionOnBottom:
		return point.y > crWindow.bottom - m_captionBorderWidth;
	case CGetSetOptions::CaptionOnLeft:
		return point.x < crWindow.left + m_captionBorderWidth;
	case CGetSetOptions::CaptionOnTop:
		return point.y < crWindow.top + m_captionBorderWidth;
	}
	return false;
}

const std::array<CDittoWindow::ButtonOffset, 5> CDittoWindow::s_buttonOffsets{{
	{ 24, 8 },
	{ 48, 32 },
	{ 72, 56 },
	{ 96, 80 },
	{ 104, 104 },
}};

int CDittoWindow::IndexToPos(int index, bool horizontal)
{
	if (index < 0 || index >= static_cast<int>(s_buttonOffsets.size()))
	{
		return 0;
	}

	const ButtonOffset& offset = s_buttonOffsets[static_cast<size_t>(index)];
	return m_dpi.Scale(horizontal ? offset.horizontal : offset.vertical);
}

void CDittoWindow::DoNcPaint(CWnd *pWnd)
{
	CWindowDC dc(pWnd);

	CRect rcFrame;
	pWnd->GetWindowRect(rcFrame);
	pWnd->ScreenToClient(rcFrame);

	CRect rc;
	pWnd->GetClientRect(rc);
	pWnd->ClientToScreen(rc);

	long lWidth = rcFrame.Width();

	// Draw the window border
	CRect rcBorder(0, 0, lWidth, rcFrame.Height());

	int border = m_dpi.Scale(2);
	int widthHeight = m_dpi.Scale(16);

	DrawFrameBorder(dc, rcBorder, border);

	const ButtonSlots slots{AssignButtonSlots()};

	CaptionLayout layout{};
	if(m_captionPosition == CGetSetOptions::CaptionOnRight)
	{
		layout = LayoutRightCaption(rcBorder, border, widthHeight, slots);
	}
	if (m_captionPosition == CGetSetOptions::CaptionOnLeft)
	{
		layout = LayoutLeftCaption(rcBorder, border, widthHeight, slots);
	}
	if (m_captionPosition == CGetSetOptions::CaptionOnTop)
	{
		layout = LayoutTopCaption(rcBorder, widthHeight, slots);
	}
	if (m_captionPosition == CGetSetOptions::CaptionOnBottom)
	{
		layout = LayoutBottomCaption(rcBorder, border, widthHeight, slots);
	}

	FillCaption(dc, layout);

	DrawCaptionText(dc, pWnd, layout);

	DrawWindowIcon(dc, pWnd);
	DrawChevronBtn(dc, pWnd);
	DrawCloseBtn(dc, pWnd);
	DrawMaximizeBtn(dc, pWnd);
	DrawMinimizeBtn(dc, pWnd);
}

void CDittoWindow::DrawFrameBorder(CWindowDC &dc, CRect &rcBorder, int border)
{
	for (int x = 0; x < border; x++)
	{
		dc.Draw3dRect(rcBorder, m_border, m_border);
		rcBorder.DeflateRect(1, 1, 1, 1);
	}
}

CDittoWindow::ButtonSlots CDittoWindow::AssignButtonSlots() const
{
	ButtonSlots slots{};

	if (m_bDrawClose)
	{
		slots.close = slots.count++;
	}
	if (m_bDrawChevron)
	{
		slots.chevron = slots.count++;
	}
	if (m_bDrawMaximize)
	{
		slots.maximize = slots.count++;
	}
	if (m_bDrawMinimize)
	{
		slots.minimize = slots.count++;
	}

	return slots;
}

CDittoWindow::CaptionLayout CDittoWindow::LayoutRightCaption(const CRect &rcBorder, int border, int widthHeight, const ButtonSlots &slots)
{
	CaptionLayout layout{};
	layout.rightRect.SetRect(rcBorder.right - (m_captionBorderWidth - border), rcBorder.top, rcBorder.right, rcBorder.top + IndexToPos(slots.count, false));
	layout.leftRect.SetRect(rcBorder.right - (m_captionBorderWidth - border), rcBorder.top + IndexToPos(slots.count, false), rcBorder.right, rcBorder.bottom);

	layout.textRect.SetRect(rcBorder.right, layout.rightRect.bottom + m_dpi.Scale(10), rcBorder.right - m_captionBorderWidth, rcBorder.bottom - m_dpi.Scale(1));

	SetVerticalButtonRects(layout.rightRect.left, layout.rightRect.right, widthHeight, slots);

	m_crWindowIconBT.SetRect(rcBorder.right - m_dpi.Scale(24), rcBorder.bottom - m_dpi.Scale(28), rcBorder.right - m_dpi.Scale(2), rcBorder.bottom);

	layout.vertical = TRUE;
	return layout;
}

CDittoWindow::CaptionLayout CDittoWindow::LayoutLeftCaption(const CRect &rcBorder, int border, int widthHeight, const ButtonSlots &slots)
{
	CaptionLayout layout{};
	layout.rightRect.SetRect(rcBorder.left, rcBorder.top, rcBorder.left + m_captionBorderWidth - border, rcBorder.top + IndexToPos(slots.count, false));
	layout.leftRect.SetRect(rcBorder.left, rcBorder.top + IndexToPos(slots.count, false), rcBorder.left + m_captionBorderWidth - border, rcBorder.bottom);

	layout.textRect.SetRect(rcBorder.left + m_captionBorderWidth - m_dpi.Scale(0), layout.rightRect.bottom + m_dpi.Scale(10), rcBorder.left - m_dpi.Scale(5), rcBorder.bottom - m_dpi.Scale(1));

	SetVerticalButtonRects(layout.rightRect.left, layout.rightRect.right, widthHeight, slots);

	m_crWindowIconBT.SetRect(rcBorder.left + m_dpi.Scale(0), rcBorder.bottom - m_dpi.Scale(28), rcBorder.left + m_dpi.Scale(25), rcBorder.bottom);

	layout.vertical = TRUE;
	return layout;
}

CDittoWindow::CaptionLayout CDittoWindow::LayoutTopCaption(const CRect &rcBorder, int widthHeight, const ButtonSlots &slots)
{
	CaptionLayout layout{};
	layout.leftRect.SetRect(rcBorder.left, rcBorder.top, rcBorder.right - IndexToPos(slots.count-1, true)- m_dpi.Scale(8), m_captionBorderWidth);
	layout.rightRect.SetRect(layout.leftRect.right, rcBorder.top, rcBorder.right, m_captionBorderWidth);

	layout.textRect.SetRect(layout.leftRect.right, layout.leftRect.top, layout.leftRect.right, layout.leftRect.bottom);

	int top = layout.rightRect.top;
	int bottom = layout.rightRect.bottom;

	SetHorizontalButtonRects(rcBorder.right, top, bottom, widthHeight, slots);
	int left = rcBorder.left + m_dpi.Scale(10);
	m_crWindowIconBT.SetRect(left, top, left + m_dpi.Scale(24), bottom);

	layout.vertical = FALSE;
	return layout;
}

CDittoWindow::CaptionLayout CDittoWindow::LayoutBottomCaption(const CRect &rcBorder, int border, int widthHeight, const ButtonSlots &slots)
{
	CaptionLayout layout{};
	layout.leftRect.SetRect(rcBorder.left, rcBorder.bottom- m_captionBorderWidth - border, rcBorder.right - IndexToPos(slots.count - 1, true) - m_dpi.Scale(8), rcBorder.bottom);
	layout.rightRect.SetRect(layout.leftRect.right, rcBorder.bottom - m_captionBorderWidth - border, rcBorder.right, rcBorder.bottom);

	layout.textRect.SetRect(layout.leftRect.right, layout.leftRect.top, layout.leftRect.right, layout.leftRect.bottom);

	int top = layout.rightRect.top;
	int bottom = layout.rightRect.bottom;

	SetHorizontalButtonRects(rcBorder.right, top, bottom, widthHeight, slots);

	int left = rcBorder.left + m_dpi.Scale(10);
	m_crWindowIconBT.SetRect(left, top, left + m_dpi.Scale(24), bottom);

	layout.vertical = FALSE;
	return layout;
}

void CDittoWindow::SetVerticalButtonRects(int left, int right, int widthHeight, const ButtonSlots &slots)
{
	int top = IndexToPos(slots.close, false);
	m_crCloseBT.SetRect(left, top, right, top + widthHeight);

	top = IndexToPos(slots.chevron, false);
	m_crChevronBT.SetRect(left, top, right, top + widthHeight);

	top = IndexToPos(slots.maximize, false);
	m_crMaximizeBT.SetRect(left, top, right, top + widthHeight);

	top = IndexToPos(slots.minimize, false);
	m_crMinimizeBT.SetRect(left, top, right, top + widthHeight);
}

void CDittoWindow::SetHorizontalButtonRects(int rightEdge, int top, int bottom, int widthHeight, const ButtonSlots &slots)
{
	int left = rightEdge - IndexToPos(slots.close, true);
	m_crCloseBT.SetRect(left, top, left + widthHeight, bottom);

	left = rightEdge - IndexToPos(slots.chevron, true);
	m_crChevronBT.SetRect(left, top, left + widthHeight, bottom);

	left = rightEdge - IndexToPos(slots.maximize, true);
	m_crMaximizeBT.SetRect(left, top, left + widthHeight, bottom);

	left = rightEdge - IndexToPos(slots.minimize, true);
	m_crMinimizeBT.SetRect(left, top, left + widthHeight, bottom);
}

void CDittoWindow::FillCaption(CWindowDC &dc, const CaptionLayout &layout)
{
	HBRUSH leftColor = CreateSolidBrush(m_CaptionColorLeft);
	HBRUSH rightColor = CreateSolidBrush(m_CaptionColorRight);

	::FillRect(dc, &layout.leftRect, leftColor);
	::FillRect(dc, &layout.rightRect, rightColor);

	DeleteObject(leftColor);
	DeleteObject(rightColor);
}

void CDittoWindow::DrawCaptionText(CWindowDC &dc, CWnd *pWnd, CaptionLayout &layout)
{
	int nOldBKMode = dc.SetBkMode(TRANSPARENT);
	COLORREF oldColor = dc.SetTextColor(m_CaptionTextColor);

	CFont *pOldFont = NULL;
	if (layout.vertical)
		pOldFont = dc.SelectObject(&m_VertFont);
	else
		pOldFont = dc.SelectObject(&m_HorFont);

	CString csText = m_customWindowTitle;
	if (m_useCustomWindowTitle == false)
	{
		pWnd->GetWindowText(csText);
	}

	CRect &textRect = layout.textRect;
	int flags = DT_SINGLELINE;
	if (layout.vertical == false)
	{
		CRect size(0, 0, 0, 0);
		dc.DrawText(csText, size, DT_CALCRECT);
		textRect.left = textRect.right - size.Width() - m_dpi.Scale(10);

		flags |= DT_VCENTER;
	}
	else
	{
		CRect size(0, 0, 0, 0);
		dc.DrawText(csText, size, DT_CALCRECT| DT_SINGLELINE);

		int rectWidth = textRect.left - textRect.right;
		int offset = rectWidth / 2 - m_titleTextHeight / 2;
		//textRect.right += 30;
		//I don't understand where the 4 is coming from but it's always 4 pixals from the right so adjust for this
		textRect.left -= (offset - m_dpi.Scale(4));
	}

	dc.DrawText(csText, textRect, flags);

	dc.SelectObject(pOldFont);
	dc.SetBkMode(nOldBKMode);
	dc.SetTextColor(oldColor);
}

void CDittoWindow::DrawChevronBtn(CWindowDC &dc, CWnd *pWnd)
{
	if(m_bDrawChevron == false)
	{
		return;
	}
		
	if(this->m_bMinimized)
	{
		m_chevronLeftButton.Draw(&dc, m_dpi, pWnd, m_crChevronBT, m_bMouseOverChevron, m_bMouseDownOnChevron);
	}
	else
	{
		m_chevronRightButton.Draw(&dc, m_dpi, pWnd, m_crChevronBT, m_bMouseOverChevron, m_bMouseDownOnChevron);
	}
}

void CDittoWindow::DrawWindowIcon(CWindowDC & /*dc*/, CWnd * /*pWnd*/)
{
	//m_windowIcon.Draw(&dc, pWnd, m_crWindowIconBT.left, m_crWindowIconBT.top, false, false);
}

void CDittoWindow::DrawCloseBtn(CWindowDC &dc, CWnd *pWnd)
{
	if(m_bDrawClose == false)
	{
		return;
	}
	
	m_closeButton.Draw(&dc, m_dpi, pWnd, m_crCloseBT, m_bMouseOverClose, m_bMouseDownOnClose);
}

void CDittoWindow::DrawMinimizeBtn(CWindowDC &dc, CWnd *pWnd)
{
	if(m_bDrawMinimize == false)
	{
		return;
	}

	m_minimizeButton.Draw(&dc, m_dpi, pWnd, m_crMinimizeBT, m_bMouseOverMinimize, m_bMouseDownOnMinimize);
}

void CDittoWindow::DrawMaximizeBtn(CWindowDC &dc, CWnd *pWnd)
{
	if(m_bDrawMaximize == false)
	{
		return;
	}

	m_maximizeButton.Draw(&dc, m_dpi, pWnd, m_crMaximizeBT, m_bMouseOverMaximize, m_bMouseDownOnMaximize);
}

int CDittoWindow::DoNcLButtonDown(CWnd *pWnd, UINT nHitTest, CPoint point) 
{
	switch (nHitTest)
	{
	case HTCAPTION:
		m_buttonDownOnCaption = true;
		break;
	default:
		m_buttonDownOnCaption = false;
	}

	int buttonPressed = 0;
	//ReleaseCapture();
	CPoint clPoint(point);
	pWnd->ScreenToClient(&clPoint);

	if (m_captionPosition == CGetSetOptions::CaptionOnLeft)
	{
		clPoint.x += m_captionBorderWidth;
	}
	else
	{
		clPoint.x += m_borderSize;
	}

	if (m_captionPosition == CGetSetOptions::CaptionOnTop)
	{
		clPoint.y += m_captionBorderWidth;
	}
	else
	{
		clPoint.y += m_borderSize;
	}	

	if(m_crCloseBT.PtInRect(clPoint))
	{
		m_bMouseDownOnClose = true;
		//InvalidateRect(pWnd->m_hWnd, m_crCloseBT, TRUE);
		//pWnd->InvalidateRect(m_crCloseBT);
		//pWnd->UpdateWindow();
		//DoNcPaint(pWnd);
		RedrawWindow(pWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
		buttonPressed = ButtonClose;
	}
	else if(m_crChevronBT.PtInRect(clPoint))
	{
		m_bMouseDownOnChevron = true;
		RedrawWindow(pWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
		buttonPressed = ButtonChevron;
	}
	else if(m_crMinimizeBT.PtInRect(clPoint))
	{
		m_bMouseDownOnMinimize = true;
		RedrawWindow(pWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
		buttonPressed = ButtonMinimize;
	}
	else if(m_crMaximizeBT.PtInRect(clPoint))
	{
		m_bMouseDownOnMaximize = true;
		RedrawWindow(pWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);
		buttonPressed = ButtonMaximize;
	}

	return buttonPressed;
}

long CDittoWindow::DoNcLButtonUp(CWnd *pWnd, UINT /*nHitTest*/, CPoint point)
{
	m_buttonDownOnCaption = false;

	CRect crWindow;
	pWnd->GetWindowRect(crWindow);

	CPoint localPoint(point.x - crWindow.left, point.y - crWindow.top);

	long lRet = 0;
	if(m_bMouseDownOnClose)
	{
		lRet = ReleaseCloseButton(pWnd, localPoint);
	}
	else if(m_bMouseDownOnChevron)
	{
		lRet = ReleaseChevronButton(pWnd, localPoint);
	}
	else if(m_bMouseDownOnMinimize)
	{
		lRet = ReleaseMinimizeButton(pWnd, localPoint);
	}
	else if(m_bMouseDownOnMaximize)
	{
		lRet = ReleaseMaximizeButton(pWnd, localPoint);
	}

	return lRet;
}

long CDittoWindow::ReleaseCloseButton(CWnd *pWnd, const CPoint &localPoint)
{
	m_bMouseDownOnClose = false;
	m_bMouseOverClose = false;

	RedrawWindow(pWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

	if(m_crCloseBT.PtInRect(localPoint))
	{
		if(m_sendWMClose)
		{
			pWnd->SendMessage(WM_CLOSE, 0, 0);
		}
		return ButtonClose;
	}
	return 0;
}

long CDittoWindow::ReleaseChevronButton(CWnd *pWnd, const CPoint &localPoint)
{
	m_bMouseDownOnChevron = false;
	m_bMouseOverChevron = false;

	RedrawWindow(pWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

	if(m_crChevronBT.PtInRect(localPoint))
	{
		return ButtonChevron;
	}
	return 0;
}

long CDittoWindow::ReleaseMinimizeButton(CWnd *pWnd, const CPoint &localPoint)
{
	m_bMouseDownOnMinimize = false;
	m_bMouseOverMinimize = false;

	RedrawWindow(pWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

	if(m_crMinimizeBT.PtInRect(localPoint))
	{
		pWnd->ShowWindow(SW_MINIMIZE);
		return ButtonMinimize;
	}
	return 0;
}

long CDittoWindow::ReleaseMaximizeButton(CWnd *pWnd, const CPoint &localPoint)
{
	m_bMouseDownOnMaximize = false;
	m_bMouseOverMaximize = false;

	RedrawWindow(pWnd->m_hWnd, NULL, NULL, RDW_FRAME | RDW_INVALIDATE);

	if(m_crMaximizeBT.PtInRect(localPoint))
	{
		if(pWnd->GetStyle() & WS_MAXIMIZE)
			pWnd->ShowWindow(SW_RESTORE);
		else
			pWnd->ShowWindow(SW_SHOWMAXIMIZED);

		return ButtonMaximize;
	}
	return 0;
}

bool CDittoWindow::DoPreTranslateMessage(MSG* /*pMsg*/)
{
	return true;
}

void CDittoWindow::SetCaptionOn(CWnd *pWnd, int nPos, bool bOnstartup, int captionSize, int captionFontSize)
{
	m_captionFontSize = captionFontSize;

	m_VertFont.DeleteObject();
	m_VertFont.CreateFont(-m_dpi.Scale(captionFontSize), 0, -900, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));

	m_HorFont.DeleteObject();
	m_HorFont.CreateFont(-m_dpi.Scale(captionFontSize), 0, 0, 0, 500, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));

	SetTitleTextHeight(pWnd);

	m_captionPosition = nPos;

	int oldWidth = m_captionBorderWidth;
	m_captionBorderWidth = m_dpi.Scale(captionSize);	
		
	if(!bOnstartup)
	{
		pWnd->SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED|SWP_NOMOVE|SWP_NOSIZE|SWP_NOZORDER);
	}

	pWnd->Invalidate();
	pWnd->RedrawWindow();

	if (oldWidth != m_captionBorderWidth)
	{
		::SetWindowPos(pWnd->m_hWnd, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
	}
}

void CDittoWindow::SetTitleTextHeight(CWnd *pWnd)
{
	CWindowDC dc(pWnd);
	CFont *pOldFont = dc.SelectObject(&m_HorFont);
	CRect size(0, 0, 0, 0);
	dc.DrawText(_T("W"), size, DT_CALCRECT);
	m_titleTextHeight = size.Height();
	dc.SelectObject(pOldFont);
}

bool CDittoWindow::SetCaptionColors(COLORREF left, COLORREF right, COLORREF border)
{
	m_CaptionColorLeft = left;
	m_CaptionColorRight = right;
	m_border = border;

	return true;
}

void CDittoWindow::SetCaptionTextColor(COLORREF color)
{
	m_CaptionTextColor = color;
}

void CDittoWindow::MinMaxWindow(CWnd *pWnd, long lOption)
{
	if ((m_bMinimized) && (lOption == ForceMin))
		return;

	if ((m_bMinimized == false) && (lOption == ForceMax))
		return;

	// the caption position decides where the window shrinks to; any other position does nothing
	if (IsKnownCaptionPosition() == false)
		return;

	if (m_bMinimized == false)
	{
		MinimizeToCaption(pWnd);
	}
	else
	{
		RestoreFromCaption(pWnd);
	}
}

bool CDittoWindow::IsKnownCaptionPosition() const
{
	return m_captionPosition == CGetSetOptions::CaptionOnRight ||
		m_captionPosition == CGetSetOptions::CaptionOnLeft ||
		m_captionPosition == CGetSetOptions::CaptionOnTop ||
		m_captionPosition == CGetSetOptions::CaptionOnBottom;
}

void CDittoWindow::MinimizeToCaption(CWnd *pWnd)
{
	pWnd->GetWindowRect(m_crFullSizeWindow);
	const WindowPlacement placement{MinimizedPlacement()};
	pWnd->MoveWindow(placement.x, placement.y, placement.width, placement.height);
	m_bMinimized = true;
	m_TimeMinimized = COleDateTime::GetCurrentTime();
}

void CDittoWindow::RestoreFromCaption(CWnd *pWnd)
{
	CRect cr;
	pWnd->GetWindowRect(cr);
	const WindowPlacement placement{RestoredPlacement(cr)};
	pWnd->MoveWindow(placement.x, placement.y, placement.width, placement.height);

	m_crFullSizeWindow.SetRectEmpty();
	m_bMinimized = false;
	m_TimeMaximized = COleDateTime::GetCurrentTime();
	::SetForegroundWindow(pWnd->GetSafeHwnd());
}

CDittoWindow::WindowPlacement CDittoWindow::MinimizedPlacement() const
{
	switch (m_captionPosition)
	{
	case CGetSetOptions::CaptionOnRight:
		return WindowPlacement{m_crFullSizeWindow.right - m_captionBorderWidth,
			m_crFullSizeWindow.top, m_captionBorderWidth,
			m_crFullSizeWindow.Height()};
	case CGetSetOptions::CaptionOnLeft:
		return WindowPlacement{m_crFullSizeWindow.left,
			m_crFullSizeWindow.top, m_captionBorderWidth,
			m_crFullSizeWindow.Height()};
	case CGetSetOptions::CaptionOnTop:
		return WindowPlacement{m_crFullSizeWindow.left,
			m_crFullSizeWindow.top,
			m_crFullSizeWindow.Width(),
			m_captionBorderWidth};
	default: // CGetSetOptions::CaptionOnBottom (MinMaxWindow handles only the four caption positions)
		return WindowPlacement{m_crFullSizeWindow.left,
			m_crFullSizeWindow.bottom - m_captionBorderWidth,
			m_crFullSizeWindow.Width(),
			m_captionBorderWidth};
	}
}

CDittoWindow::WindowPlacement CDittoWindow::RestoredPlacement(const CRect &cr) const
{
	switch (m_captionPosition)
	{
	case CGetSetOptions::CaptionOnRight:
		return WindowPlacement{cr.right - m_crFullSizeWindow.Width(),
			cr.top, m_crFullSizeWindow.Width(), cr.Height()};
	case CGetSetOptions::CaptionOnLeft:
		return WindowPlacement{cr.left, cr.top,
			m_crFullSizeWindow.Width(), cr.Height()};
	case CGetSetOptions::CaptionOnTop:
		return WindowPlacement{cr.left, cr.top,
			cr.Width(), m_crFullSizeWindow.Height()};
	default: // CGetSetOptions::CaptionOnBottom (MinMaxWindow handles only the four caption positions)
		return WindowPlacement{cr.left,
			cr.bottom - m_crFullSizeWindow.Height(),
			cr.Width(), m_crFullSizeWindow.Height()};
	}
}

void CDittoWindow::OnDpiChanged(CWnd *pParent, int dpi)
{
	m_dpi.Update(dpi);

	m_captionBorderWidth = m_dpi.Scale(25);
	m_borderSize = m_dpi.Scale(2);

	m_VertFont.DeleteObject();
	m_HorFont.DeleteObject();

	m_VertFont.CreateFont(-m_dpi.Scale(m_captionFontSize), 0, -900, 0, 400, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));

	m_HorFont.CreateFont(-m_dpi.Scale(m_captionFontSize), 0, 0, 0, 500, FALSE, FALSE, 0, DEFAULT_CHARSET,
		OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
		DEFAULT_PITCH | FF_SWISS, _T("Segoe UI"));

	m_closeButton.Reset();
	m_closeButton.LoadStdImageDPI(m_dpi.GetDPI(), Close_Black_16_16, Close_Black_20_20, Close_Black_24_24, Close_Black_28, Close_Black_32_32, _T("PNG"), close_36, close_40, close_44, close_48, close_52, close_56);

	m_chevronRightButton.Reset();
	m_chevronRightButton.LoadStdImageDPI(m_dpi.GetDPI(), ChevronRight_Black_16_16, ChevronRight_Black_20_20, ChevronRight_Black_24_24, ChevronRight_Black_28, ChevronRight_Black_32_32, _T("PNG"), ChevronRight_Black_36, ChevronRight_Black_40, ChevronRight_Black_44, ChevronRight_Black_48, ChevronRight_Black_52, ChevronRight_Black_56);
	
	m_chevronLeftButton.Reset();
	m_chevronLeftButton.LoadStdImageDPI(m_dpi.GetDPI(), ChevronLeft_Black_16_16, ChevronLeft_Black_20_20, ChevronLeft_Black_24_24, ChevronLeft_Black_28, ChevronLeft_Black_32_32, _T("PNG"), ChevronLeft_Black_36, ChevronLeft_Black_40, ChevronLeft_Black_44, ChevronLeft_Black_48, ChevronLeft_Black_52, ChevronLeft_Black_56);

	m_maximizeButton.Reset();
	m_maximizeButton.LoadStdImageDPI(m_dpi.GetDPI(), IDB_MAXIMIZE_16_16, maximize_20, maximize_24, maximize_28, maximize_32, _T("PNG"), maximize_36, maximize_40, maximize_44, maximize_48, maximize_52, maximize_56);

	m_minimizeButton.Reset();
	m_minimizeButton.LoadStdImageDPI(m_dpi.GetDPI(), minimize_16, minimize_20, minimize_24, minimize_28, minimize_32, _T("PNG"), minimize_36, minimize_40, minimize_44, minimize_48, minimize_52, minimize_56);

	SetTitleTextHeight(pParent);

	/*pParent->SetWindowPos(NULL, 0, 0, 0, 0, SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER);

	pParent->Invalidate();
	pParent->RedrawWindow();*/

	//::SetWindowPos(pParent->m_hWnd, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
}
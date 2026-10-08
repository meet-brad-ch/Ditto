// QRCodeViewer.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "QRCodeViewer.h"
#include "MainTableFunctions.h"
#include "ErrorReport.h"

// QRCodeViewer

IMPLEMENT_DYNAMIC(QRCodeViewer, CWnd)

QRCodeViewer::QRCodeViewer()
{
}

QRCodeViewer::~QRCodeViewer()
{
}

BEGIN_MESSAGE_MAP(QRCodeViewer, CWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_NCHITTEST()
	ON_WM_NCPAINT()
	ON_WM_NCCALCSIZE()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCLBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_WM_CTLCOLOR()
	ON_WM_WINDOWPOSCHANGING()
	ON_WM_TIMER()
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_WM_MOVING()
	ON_WM_ENTERSIZEMOVE()
END_MESSAGE_MAP()


BOOL QRCodeViewer::LoadQrBitmap(std::vector<std::byte> bitmap)
{
	// before the window exists: a bitmap GDI+ cannot read fails the load (the caller reports
	// it); LoadRaw copies the bytes into its own buffer
	return m_qrCodeDrawer.LoadRaw(reinterpret_cast<unsigned char*>(bitmap.data()), static_cast<int>(bitmap.size()));
}

BOOL QRCodeViewer::CreateEx(CWnd *pParentWnd, CString desc, int rowHeight, LOGFONT logFont)
{
	CGetSetOptions &settings = theApp.Services().Settings();

	// Get the class name and create the window
	CString szClassName = AfxRegisterWndClass(CS_CLASSDC | CS_SAVEBITS, LoadCursor(NULL, IDC_ARROW));

	m_descRowHeight = rowHeight;
	m_descBackground.DeleteObject();
	m_descBackground.CreateSolidBrush(RGB(255, 255, 255));
	m_logFont = logFont;
	m_originalFontHeight = logFont.lfHeight;

	if(CWnd::CreateEx(0, szClassName, _T(""), WS_POPUP, 0, 0, 0, 0, NULL, 0, NULL))
	{	
		m_font.CreateFontIndirect(&logFont);

		// the QR code is still shown without its description; MoveControls skips a missing m_desc
		if (m_desc.Create(CMainTableFunctions::GetDisplayText(settings.m_nLinesPerRow, desc, settings.m_bDescShowLeadingWhiteSpace), WS_CHILD|WS_VISIBLE, CRect(0,0,0,0), this, 2))
		{
			m_desc.SetFont(&m_font);
		}
		else
		{
			CErrorReport::Show(CStringUtil::Format(_T("Ditto could not create the QR code description (CStatic::Create failed, error %u)."), ::GetLastError()));
		}

		m_DittoWindow.DoCreate(this);
		m_DittoWindow.SetCaptionColors(settings.m_Theme.CaptionLeft(), settings.m_Theme.CaptionRight(), settings.m_Theme.Border());
		m_DittoWindow.SetCaptionOn(this, settings.GetCaptionPos(), true, settings.m_Theme.GetCaptionSize(), settings.m_Theme.GetCaptionFontSize());
		m_DittoWindow.m_bDrawMinimize = false;
		m_DittoWindow.m_bDrawMaximize = true;
		m_DittoWindow.m_bDrawChevron = false;
		m_DittoWindow.m_sendWMClose = false;


		CRect parentRect;
		pParentWnd->GetWindowRect(&parentRect);

		CRect rect;
		rect.left = parentRect.left;
		rect.top = parentRect.top;

		rect.right = rect.left + m_DittoWindow.m_borderSize + m_DittoWindow.m_borderSize + m_qrCodeDrawer.ImageWidth() + (settings.GetQRCodeBorderPixels() * 2);
		if (m_DittoWindow.m_captionPosition == CGetSetOptions::CaptionOnLeft ||
			m_DittoWindow.m_captionPosition == CGetSetOptions::CaptionOnRight)
		{
			rect.right += m_DittoWindow.m_captionBorderWidth;
		}
		rect.bottom = rect.top + m_DittoWindow.m_borderSize + m_DittoWindow.m_borderSize + rowHeight + 5 + m_qrCodeDrawer.ImageHeight() + (settings.GetQRCodeBorderPixels() * 2);
		
		CRect center = CMonitorGeometry::CenterRect(rect);

		CMonitorGeometry::EnsureWindowVisible(&center);

		::MoveWindow(m_hWnd, center.left, center.top, center.Width(), center.Height(), TRUE);

		MoveControls();

		SetFocus();
	}
	else
	{
		return FALSE;
	}

	return TRUE;
}

void QRCodeViewer::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	this->Invalidate();

	MoveControls();
}

void QRCodeViewer::MoveControls()
{
	CRect crRect;
	GetClientRect(crRect);
	int cx = crRect.Width();
	int cy = crRect.Height();

	if(m_desc.m_hWnd != NULL)
	{
		m_desc.MoveWindow(m_DittoWindow.m_dpi.Scale(5), cy - m_DittoWindow.m_dpi.Scale(m_descRowHeight) - m_DittoWindow.m_dpi.Scale(5), cx - m_DittoWindow.m_dpi.Scale(10), m_DittoWindow.m_dpi.Scale(m_descRowHeight));
	}
}

void QRCodeViewer::OnPaint()
{
	CPaintDC dc(this);

	CRect thisRect;
	GetClientRect(thisRect);
	thisRect.bottom -= m_DittoWindow.m_dpi.Scale(m_descRowHeight) - m_DittoWindow.m_dpi.Scale(5);
	
	CGetSetOptions &settings = theApp.Services().Settings();
	int width = thisRect.Width() - (settings.GetQRCodeBorderPixels() * 2);
	int height = min(width, (thisRect.Height() - (settings.GetQRCodeBorderPixels() * 2)));
	width = min(width, height);
		
	CRect imageRect(0, 0, width, height);

	CRect centerRect = CMonitorGeometry::CenterRectFromRect(imageRect, thisRect);

	m_qrCodeDrawer.Draw(&dc, m_DittoWindow.m_dpi, this, centerRect.left, centerRect.top, false, false, width, height);
}

BOOL QRCodeViewer::PreTranslateMessage(MSG *pMsg)
{
	m_DittoWindow.DoPreTranslateMessage(pMsg);

	switch(pMsg->message)
	{
	case WM_KEYDOWN:

		switch(pMsg->wParam)
		{
		case VK_ESCAPE:
			::SendMessage(m_hWnd, WM_CLOSE, 0, 0);
			return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}
	
void QRCodeViewer::PostNcDestroy()
{
    CWnd::PostNcDestroy();

    delete this; // ownership: the window (a self-deleting window ends here)
}

void QRCodeViewer::OnNcPaint()
{
	m_DittoWindow.DoNcPaint(this);
}

void QRCodeViewer::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
	CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);

	m_DittoWindow.DoNcCalcSize(bCalcValidRects, lpncsp);
}

LRESULT QRCodeViewer::OnNcHitTest(CPoint point) 
{
	UINT Ret = m_DittoWindow.DoNcHitTest(this, point);
	if(Ret == -1)
		return CWnd::OnNcHitTest(point);

	return Ret;
}

BOOL QRCodeViewer::OnEraseBkgnd(CDC* pDC) 
{
	CRect rect;
	GetClientRect(&rect);
	CBrush myBrush(RGB(255, 255, 255));
	CBrush *pOld = pDC->SelectObject(&myBrush);
	pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);
	pDC->SelectObject(pOld);

	return TRUE;
}

void QRCodeViewer::OnNcLButtonDown(UINT nHitTest, CPoint point) 
{
	int buttonPressed = m_DittoWindow.DoNcLButtonDown(this, nHitTest, point);

	if (buttonPressed != 0)
	{
		SetTimer(TimerButtonUp, 100, NULL);
	}

	CWnd::OnNcLButtonDown(nHitTest, point);
}

void QRCodeViewer::OnNcLButtonUp(UINT nHitTest, CPoint point) 
{
	long lRet = m_DittoWindow.DoNcLButtonUp(this, nHitTest, point);

	switch(lRet)
	{
	case CDittoWindow::ButtonClose:
		::PostMessage(m_hWnd, WM_CLOSE, 0, 0);
		break;
	}

	KillTimer(TimerButtonUp);

	CWnd::OnNcLButtonUp(nHitTest, point);
}

HBRUSH QRCodeViewer::OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor)
{
	HBRUSH hbr = CWnd::OnCtlColor(pDC, pWnd, nCtlColor);
	if(pWnd->GetDlgCtrlID() == 2)
	{
		pDC->SetBkColor(RGB(255,255,255));

		return static_cast<HBRUSH>(m_descBackground.GetSafeHandle());
	}

	// TODO:  Return a different brush if the default is not desired
	return hbr;
}
void QRCodeViewer::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanging(lpwndpos);
}

void QRCodeViewer::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
		case TimerButtonUp:
		{
			// the high bit (0x8000) is the "down" bit; upstream tested 0x100, which is never set
			if ((GetKeyState(VK_LBUTTON) & 0x8000) == 0)
			{
				m_DittoWindow.DoNcLButtonUp(this, 0, CPoint(0, 0));
				KillTimer(TimerButtonUp);
			}
			break;
		}
	}

	CWnd::OnTimer(nIDEvent);
}

LRESULT QRCodeViewer::OnDpiChanged(WPARAM wParam, LPARAM lParam)
{
	int dpi = HIWORD(wParam);
	m_DittoWindow.OnDpiChanged(this, dpi);

	RECT* const prcNewWindow = (RECT*)lParam;
	SetWindowPos(NULL,
		prcNewWindow->left,
		prcNewWindow->top,
		prcNewWindow->right - prcNewWindow->left,
		prcNewWindow->bottom - prcNewWindow->top,
		SWP_NOZORDER | SWP_NOACTIVATE);

	CLogger::Write(CStringUtil::Format(_T("QRCodeViewer::OnDpiChanged dpi: %d width: %d, height: %d"), dpi, (prcNewWindow->right - prcNewWindow->left), (prcNewWindow->bottom - prcNewWindow->top)));

	MoveControls();

	m_logFont.lfHeight = m_DittoWindow.m_dpi.Scale(m_originalFontHeight);

	m_font.DeleteObject();
	m_font.CreateFontIndirect(&m_logFont);
	// the description is missing when its creation failed (reported in CreateEx)
	if (m_desc.GetSafeHwnd() != NULL)
	{
		m_desc.SetFont(&m_font);
	}

	return TRUE;
}

void QRCodeViewer::OnMoving(UINT fwSide, LPRECT pRect)
{
	CWnd::OnMoving(fwSide, pRect);
	m_snap.OnSnapMoving(m_hWnd, pRect);
}

void QRCodeViewer::OnEnterSizeMove()
{
	m_snap.OnSnapEnterSizeMove(m_hWnd);
	CWnd::OnEnterSizeMove();
}
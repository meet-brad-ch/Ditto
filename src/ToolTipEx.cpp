#include "stdafx.h"
#include "cp_main.h"
#include "ToolTipEx.h"
#include "BitmapHelper.h"
#include "Options.h"
#include "ActionEnums.h"
#include <Richedit.h>
#include <algorithm>

/////////////////////////////////////////////////////////////////////////////
// CToolTipEx

CToolTipEx::CToolTipEx(): m_dwTextStyle(DT_EXPANDTABS | DT_EXTERNALLEADING |
                       DT_NOPREFIX | DT_WORDBREAK), m_rectMargin(2, 2, 3, 3),
                        m_pNotifyWnd(NULL), m_clipId(0), m_clipRow(-1)
{
	m_showPersistant = false;
	m_pToolTipActions = NULL;
	m_bMaxSetTimer = false;
	m_lDelayMaxSeconds = 2;
	m_showingText = false;
	m_showingRTF = false;
}

CToolTipEx::~CToolTipEx()
{
    m_Font.DeleteObject();
	m_clipDataFont.DeleteObject();
}

CGetSetOptions& CToolTipEx::Settings() const
{
	return theApp.Services().Settings();
}

BEGIN_MESSAGE_MAP(CToolTipEx, CWnd)
	//{{AFX_MSG_MAP(CToolTipEx)
	ON_WM_PAINT()
	ON_WM_SIZE()
	ON_WM_NCHITTEST()
	ON_WM_ACTIVATE()
	ON_WM_TIMER()
	ON_WM_NCLBUTTONDBLCLK()
	ON_WM_NCPAINT()
	ON_WM_NCCALCSIZE()
	ON_WM_NCLBUTTONDOWN()
	ON_WM_NCMOUSEMOVE()
	ON_WM_NCLBUTTONUP()
	ON_WM_ERASEBKGND()
	ON_COMMAND(ID_FIRST_REMEMBERWINDOWPOSITION, &CToolTipEx::OnRememberwindowposition)
	ON_COMMAND(ID_FIRST_SIZEWINDOWTOCONTENT, &CToolTipEx::OnSizewindowtocontent)
	ON_COMMAND(ID_FIRST_SCALEIMAGESTOFITWINDOW, &CToolTipEx::OnScaleimagestofitwindow)
	ON_COMMAND(2, OnOptions)
	ON_WM_RBUTTONDOWN()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_FIRST_HIDEDESCRIPTIONWINDOWONM, &CToolTipEx::OnFirstHidedescriptionwindowonm)
	ON_COMMAND(ID_FIRST_WRAPTEXT, &CToolTipEx::OnFirstWraptext)
	ON_WM_WINDOWPOSCHANGING()
	ON_COMMAND(ID_FIRST_ALWAYSONTOP, &CToolTipEx::OnFirstAlwaysontop)
	ON_NOTIFY(EN_MSGFILTER, 1, &CToolTipEx::OnEnMsgfilterRichedit21)
	ON_MESSAGE(WM_DPICHANGED, OnDpiChanged)
	ON_WM_MOVING()
	ON_WM_ENTERSIZEMOVE()
	ON_WM_HSCROLL()
	ON_MESSAGE(CDittoMessage::RefreshFooter, OnRefreshFooter)
	ON_COMMAND(ID_FIRST_VIEWTEXT, &CToolTipEx::OnFirstViewtext)
	ON_COMMAND(ID_FIRST_VIEWRTF, &CToolTipEx::OnFirstViewrtf)
	ON_COMMAND(ID_FIRST_VIEWASIMAGE, &CToolTipEx::OnFirstViewImage)
	ON_UPDATE_COMMAND_UI(ID_FIRST_VIEWTEXT, &CToolTipEx::OnUpdateFirstViewtext)
	ON_UPDATE_COMMAND_UI(ID_FIRST_VIEWRTF, &CToolTipEx::OnUpdateFirstViewrtf)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CToolTipEx message handlers

BOOL CToolTipEx::Create(CWnd *pParentWnd)
{
	m_saveWindowLockout = true;

    // Get the class name and create the window
    CString szClassName = AfxRegisterWndClass(CS_CLASSDC | CS_SAVEBITS, LoadCursor(NULL, IDC_ARROW));

    // Create the window - just don't show it yet.
    if( !CWnd::CreateEx(0, szClassName, _T(""), WS_POPUP,
       0, 0, 0, 0, pParentWnd->GetSafeHwnd(), 0, NULL))
    {
        return FALSE;
    }	

	HICON b = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 64, 64, LR_SHARED);
	SetIcon(b, TRUE);

	//CString szClassName2 = AfxRegisterWndClass(CS_CLASSDC | CS_SAVEBITS, LoadCursor(NULL, IDC_ARROW));
	//BOOL b = m_imageViewer.Create(_T(""), szClassName2, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL, CRect(0, 0, 0, 0), this, 3);
	m_imageViewer.Create(this);
	
	
	m_DittoWindow.DoCreate(this);
	m_DittoWindow.SetCaptionColors(Settings().m_Theme.CaptionLeft(), Settings().m_Theme.CaptionRight(), Settings().m_Theme.Border());
	m_DittoWindow.SetCaptionOn(this, Settings().GetCaptionPos(), true, Settings().m_Theme.GetCaptionSize(), Settings().m_Theme.GetCaptionFontSize());
	m_DittoWindow.m_bDrawMaximize = false;
	m_DittoWindow.m_bDrawMinimize = false;
	m_DittoWindow.m_bDrawChevron = true;
	m_DittoWindow.m_sendWMClose = false;

    m_RichEdit.Create(_T(""), _T(""), WS_CHILD | WS_VISIBLE | WS_VSCROLL |
		WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL |
                      ES_AUTOHSCROLL, CRect(10, 10, 100, 200), this, 1);

    m_RichEdit.SetReadOnly();
    m_RichEdit.SetBackgroundColor(FALSE, Settings().m_Theme.DescriptionWindowBG());

	m_RichEdit.SetEventMask(m_RichEdit.GetEventMask() | ENM_SELCHANGE | ENM_MOUSEEVENTS | ENM_SCROLLEVENTS);
	m_RichEdit.SetAutoURLDetect(TRUE);
	
	ApplyWordWrap();	   

	m_optionsButton.Create(NULL, WS_CHILD | BS_OWNERDRAW | WS_TABSTOP, CRect(0, 0, 0, 0), this, 2);
	m_optionsButton.LoadStdImageDPI(m_DittoWindow.m_dpi.GetDPI(), IDB_COG_16_16, IDB_COG_20_20, IDB_COG_24_24, cog_28, IDB_COG_32_32, _T("PNG"));
	m_optionsButton.SetToolTipText(theApp.Services().Language().GetString(_T("DescriptionOptionsTooltip"), _T("Description Options")));
	m_optionsButton.ShowWindow(SW_SHOW);

	m_clipDataStatic.Create(_T("some text"), WS_CHILD | WS_VISIBLE | SS_SIMPLE, CRect(0, 0, 0, 0), this, 3);
	m_folderPathStatic.Create(_T("some text"), WS_CHILD | WS_VISIBLE | SS_SIMPLE, CRect(0, 0, 0, 0), this, 4);

	m_clipDataFont.CreateFont(-m_DittoWindow.m_dpi.Scale(11), 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 3, 2, 1, 34, _T("Segoe UI"));

	m_Font.CreateFont(-m_DittoWindow.m_dpi.Scale(13), 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 3, 2, 1, 34, _T("Segoe UI"));
	m_fontHeight = -13;

	m_clipDataStatic.SetFont(&m_clipDataFont);
	m_clipDataStatic.SetBkColor(Settings().m_Theme.DescriptionWindowBG());
	m_clipDataStatic.SetTextColor(Settings().m_Theme.DescriptionWindowText());

	m_folderPathStatic.SetFont(&m_clipDataFont);
	m_folderPathStatic.SetBkColor(Settings().m_Theme.DescriptionWindowBG());
	m_folderPathStatic.SetTextColor(Settings().m_Theme.DescriptionWindowText());
	
	m_saveWindowLockout = false;

    return TRUE;
}

BOOL CToolTipEx::Show(CPoint point)
{
	m_showingText = false;
	m_showingRTF = false;
	m_showingImage = false;

	CRect rect;

	if(Settings().GetSizeDescWindowToContent() == FALSE)
	{
		rect = RectFromSavedSize(point);
	}
	else
	{
		rect = RectSizedToContent(point);
	}

	if (m_csText.GetLength())
	{
		int wordCount = CStringUtil::WordCount(m_csText);
		m_clipData = CStringUtil::Format(_T("%s | Length: %d | Words: %d"), m_originalClipData.GetString(), m_csText.GetLength(), wordCount);
	}

	m_clipDataStatic.SetWindowText(m_clipData);
	m_folderPathStatic.SetWindowText(m_folderPath);

	if (m_DittoWindow.m_bMinimized)
	{
		//m_DittoWindow.MinMaxWindow(this, CDittoWindow::ForceMax);
		m_DittoWindow.m_bMinimized = false;
	}

	m_saveWindowLockout = true;
	MoveWindow(rect);
	MoveControls();

	if (m_imageViewer.m_pGdiplusBitmap)
	{
		m_imageViewer.UpdateBitmapSize(true);
	}

	ShowContentWindow();

	ShowWindow(SW_SHOWNA);
	//this->Invalidate();
	//this->UpdateWindow();

	m_saveWindowLockout = false;

	return TRUE;
}

CRect CToolTipEx::RectFromSavedSize(CPoint point)
{
	CRect rect;
	rect.left = point.x;
	rect.top = point.y;
	CSize size;
	Settings().GetDescWndSize(size);
	rect.right = rect.left + m_DittoWindow.m_dpi.Scale(size.cx);
	rect.bottom = rect.top + m_DittoWindow.m_dpi.Scale(size.cy);

	CMonitorGeometry::EnsureWindowVisible(&rect);
	return rect;
}

CRect CToolTipEx::RectSizedToContent(CPoint point)
{
	CRect rect = GetBoundsRect();

	//account for the scroll bars
	rect.right += 20;
	rect.bottom += 20;

	if (m_imageViewer.m_pGdiplusBitmap)
	{
		int nWidth = m_imageViewer.m_pGdiplusBitmap->GetWidth() + ::GetSystemMetrics(SM_CXVSCROLL);
		int nHeight = m_imageViewer.m_pGdiplusBitmap->GetHeight() + ::GetSystemMetrics(SM_CYHSCROLL);

		rect.right = rect.left + nWidth;
		rect.bottom = rect.top + nHeight;
	}


	long lNewWidth = (long)rect.Width() + (long)(rect.Width() *1.25);
	rect.right = rect.left + lNewWidth;

	long lNewHeight = (long)rect.Height() + (long)(rect.Height() *1.25);
	rect.bottom = rect.top + lNewHeight;

	ClientToScreen(rect);

	CRect cr(point, point);
	CRect rcScreen = CMonitorGeometry::MonitorRectFromRect(cr);

	//ensure that we don't go outside the screen
	if(point.x < rcScreen.left)
	{
		point.x = rcScreen.left + 5;
	}
	if(point.y < rcScreen.top)
	{
		point.y = rcScreen.top + 5;
	}

	rcScreen.DeflateRect(0, 0, 5, 5);

	long width = rect.Width();
	long height = rect.Height();

	if (width < 500)
	{
		width = 500;
	}

	rect.left = point.x;
	rect.top = point.y;
	rect.right = rect.left + width;
	rect.bottom = rect.top + height;

	if (rect.right > rcScreen.right)
	{
		rect.right = rcScreen.right;
	}
	if (rect.bottom > rcScreen.bottom)
	{
		rect.bottom = rcScreen.bottom;
	}
	return rect;
}

void CToolTipEx::ShowContentWindow()
{
	if (m_imageViewer.m_pGdiplusBitmap)
	{
		int percent = static_cast<int>((m_imageViewer.m_scale - 1.0) * 100.0);
		m_clipData = m_originalClipData + _T(" | ") + CStringUtil::Format(_T("%d x %d, %d%%"), m_imageViewer.m_pGdiplusBitmap->GetWidth(), m_imageViewer.m_pGdiplusBitmap->GetHeight(), percent);

		//OutputDebugString(_T("Showing image editor\r\n"));

		m_RichEdit.ShowWindow(SW_HIDE);

		m_imageViewer.ShowWindow(SW_SHOW);

		m_showingImage = true;
	}
	else
	{
		m_imageViewer.ShowWindow(SW_HIDE);

		m_RichEdit.ShowWindow(SW_SHOW);

		//OutputDebugString(_T("Showing rich text\r\n"));
		if (m_csRTF.GetLength() > 0)
		{
			m_showingRTF = true;
		}
		else
		{
			m_showingText = true;
		}
	}
}

void CToolTipEx::GetWindowRectEx(LPRECT lpRect)
{
	if (m_DittoWindow.m_bMinimized)
	{
		*lpRect = m_DittoWindow.m_crFullSizeWindow;
		return;
	}

	CWnd::GetWindowRect(lpRect);
}

BOOL CToolTipEx::Hide()
{
	m_imageViewer.m_pGdiplusBitmap.reset();

	SaveWindowSize();	
	ShowWindow(SW_HIDE);

	m_csRTF = "";
	m_csText = "";
	m_clipId = 0;
	m_clipRow = -1;
	m_searchText = _T("");
	m_showPersistant = false;

	return TRUE;
}

void CToolTipEx::OnNcLButtonDblClk(UINT nHitTest, CPoint point)
{
	// toggle ShowPersistent when we double click the caption
	if (nHitTest == HTCAPTION)
	{
		OnFirstAlwaysontop();
	}

	CWnd::OnNcLButtonDblClk(nHitTest, point);
}

void CToolTipEx::SaveWindowSize()
{
	if (::IsWindowVisible(m_hWnd))
	{
		CRect rect;

		if (m_DittoWindow.m_bMinimized)
		{
			rect = m_DittoWindow.m_crFullSizeWindow;
		}
		else
		{
			this->GetWindowRect(&rect);
		}

		CSize s = rect.Size();
		Settings().SetDescWndSize(CSize(m_DittoWindow.m_dpi.UnScale(s.cx), m_DittoWindow.m_dpi.UnScale(s.cy)));
		Settings().SetDescWndPoint(rect.TopLeft());

		OutputDebugString(_T("Saving tooltip size"));
	}
}

void CToolTipEx::PostNcDestroy()
{
	CWnd::PostNcDestroy();

	delete this; // ownership: the window (a self-deleting window ends here)
}

BOOL CToolTipEx::PreTranslateMessage(MSG *pMsg)
{
	m_DittoWindow.DoPreTranslateMessage(pMsg);

	switch (pMsg->message)
	{
        case WM_KEYDOWN:
			if (HandleCopyKey(pMsg))
				return TRUE;
			break;
		case WM_RBUTTONDOWN:
			if (HandleContentRButtonDown())
				return TRUE;
			break;
		case WM_LBUTTONUP:
			FocusParentAfterLButtonUp();
			break;
    }

	CheckToolTipActions(pMsg);

    return CWnd::PreTranslateMessage(pMsg);
}

bool CToolTipEx::HandleCopyKey(MSG *pMsg)
{
	switch(pMsg->wParam)
	{
	case 'C':
		if(GetKeyState(VK_CONTROL) &0x8000)
		{
			m_RichEdit.Copy();
			theApp.Services().State().SetCopyReason(CopyReasonEnum::COPY_FROM_TOOLTIP);
			return true;
		}
		break;
	}
	return false;
}

bool CToolTipEx::HandleContentRButtonDown()
{
	auto f = GetFocus();
	if (f != NULL &&
		(m_RichEdit.m_hWnd == f->m_hWnd ||
		m_imageViewer.m_hWnd == f->m_hWnd))
	{
		OnOptions();
		return true;
	}
	return false;
}

void CToolTipEx::FocusParentAfterLButtonUp()
{
	auto f = GetFocus();
	if (f != NULL &&
		m_RichEdit.m_hWnd != f->m_hWnd &&
		m_optionsButton.m_hWnd != f->m_hWnd)
	{
		auto p = GetParent();
		if (p != NULL)
		{
			p->SetFocus();
		}
	}
}

void CToolTipEx::CheckToolTipActions(MSG *pMsg)
{
	if (m_pToolTipActions != NULL)
	{
		CAccel a;
		if (m_pToolTipActions->OnMsg(pMsg, a, Settings().m_doubleKeyStrokeTimeout))
		{
			switch (a.Cmd)
			{
			case ActionEnums::CLOSEWINDOW:
				/*if (this->m_showPersistant &&
					m_DittoWindow.m_bMinimized == false)
				{
					m_DittoWindow.MinMaxWindow(this, CDittoWindow::ForceMin);
					theApp.Services().ActiveWindow().ReleaseFocus();

					return TRUE;
				}*/
				break;
			}
		}
	}
}

BOOL CToolTipEx::OnMsg(MSG *pMsg)
{
    if(FALSE == IsWindowVisible())
    {
        return FALSE;
    }

	const UINT message = pMsg->message;

	if (message == WM_WINDOWPOSCHANGING || message == WM_LBUTTONDOWN)
	{
		HideOnMouseClick();
		return FALSE;
	}
	if (message == WM_KEYDOWN)
	{
		return OnMsgKeyDown(pMsg->wParam);
	}
	if (IsHidingClick(message))
	{
		HideUnlessPersistent();
		return FALSE;
	}
	if (message == WM_MOUSEWHEEL || message == WM_MOUSEHWHEEL)
	{
		ForwardMouseWheel(pMsg);
		return TRUE;
	}

    return FALSE;
}

void CToolTipEx::HideOnMouseClick()
{
	if (m_showPersistant == false)
	{
		if (Settings().GetMouseClickHidesDescription())
		{
			if (!IsCursorInToolTip())
			{
				Hide();
			}
		}
	}
}

void CToolTipEx::HideUnlessPersistent()
{
	if (m_showPersistant == false)
	{
		Hide();
	}
}

bool CToolTipEx::IsHidingClick(UINT message)
{
	return std::find(s_hidingClickMessages.begin(), s_hidingClickMessages.end(), message) != s_hidingClickMessages.end();
}

bool CToolTipEx::IsListKey(WPARAM vk)
{
	return std::find(s_listKeys.begin(), s_listKeys.end(), vk) != s_listKeys.end();
}

BOOL CToolTipEx::OnMsgKeyDown(WPARAM vk)
{
	if(vk == VK_TAB)
	{
		m_RichEdit.SetFocus();
		return TRUE;
	}
	else if (IsListKey(vk))
	{
		return FALSE;
	}

	if (m_pToolTipActions != NULL)
	{
		if (m_pToolTipActions->ContainsKey((int)vk))
		{
			return FALSE;
		}
	}

	HideUnlessPersistent();

	return FALSE;
}

void CToolTipEx::ForwardMouseWheel(MSG *pMsg)
{
	if (m_imageViewer.m_pGdiplusBitmap)
	{
		m_imageViewer.PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
	}
	else
	{
		m_RichEdit.PostMessageW(pMsg->message, pMsg->wParam, pMsg->lParam);
	}
}

CRect CToolTipEx::GetBoundsRect()
{
	ULONGLONG d = GetTickCount64();

    CWindowDC dc(NULL);
	int nLineWidth = 0;

	CRect rect(0, 0, 0, 0);

    if(nLineWidth == 0)
    {
        // Count the number of lines of text
		const TextLines lines{MeasureTextLines()};

		CFont *pOldFont = (CFont*)dc.SelectObject((CFont*)&m_Font);
		CSize size = dc.GetTextExtent(lines.longest);
		dc.SelectObject(pOldFont);

		rect.right = size.cx;
		rect.bottom = size.cy * lines.count;
    }
	
    rect.bottom += m_rectMargin.top + m_rectMargin.bottom + GetSystemMetrics(SM_CYVSCROLL);
    rect.right += m_rectMargin.left + m_rectMargin.right + GetSystemMetrics(SM_CXVSCROLL);

    if(m_imageViewer.m_pGdiplusBitmap)
    {
		int nWidth = m_imageViewer.m_pGdiplusBitmap->GetWidth();
		int nHeight = m_imageViewer.m_pGdiplusBitmap->GetHeight();

        rect.bottom += nHeight;
        if((rect.left + nWidth) > rect.right)
        {
            rect.right = rect.left + nWidth;
        }
    }

	ULONGLONG diff = GetTickCount64() - d;
	if (diff > 10)
	{
		CLogger::Log(CStringUtil::Format(_T("Size To Content: %llu\n"), diff));
	}

    return rect;
}

CToolTipEx::TextLines CToolTipEx::MeasureTextLines()
{
	TextLines lines{};
	int nStart = 0;
	int longestLength = 0;
    do
    {
		lines.count++;

        int newStart = m_csText.Find(_T("\n"), nStart);
		if (newStart < 0)
		{
			int length = m_csText.GetLength() - nStart;
			if (length > longestLength)
			{
				lines.longest = m_csText.Mid(nStart, length);
				longestLength = length;
			}

			break;
		}

		int length = newStart - nStart;
		if(length > longestLength)
		{
			lines.longest = m_csText.Mid(nStart, length);
			longestLength = length;
		}


		nStart = newStart + 1;
    }
    while(nStart >= 0 && lines.count < 100);

	return lines;
}

CString CToolTipEx::GetFieldFromString(CString ref, int nIndex, TCHAR ch)
{
    CString strReturn;
    LPCTSTR pstrStart = ref.LockBuffer();
    LPCTSTR pstrBuffer = pstrStart;
    int nCurrent = 0;
    int nStart = 0;
    int nEnd = 0;
    int nOldStart = 0;

    while(nCurrent <= nIndex &&  *pstrBuffer != _T('\0'))
    {
        if(*pstrBuffer == ch)
        {
            nOldStart = nStart;
            nStart = nEnd + 1;
            nCurrent++;
        }
        nEnd++;
        pstrBuffer++;
    }

    // May have reached the end of the string
    if(*pstrBuffer == _T('\0'))
    {
        nOldStart = nStart;
        nEnd++;
    }

    ref.UnlockBuffer();

    if(nCurrent < nIndex)
    {
        //TRACE1("Warning: GetStringField - Couldn't find field %d.\n", nIndex);
        return strReturn;
    }
    return ref.Mid(nOldStart, nEnd - nOldStart - 1);
}

BOOL CToolTipEx::SetLogFont(LPLOGFONT lpLogFont, BOOL bRedraw /*=TRUE*/)
{
    ASSERT(lpLogFont);
    if(!lpLogFont)
    {
        return FALSE;
    }

    LOGFONT LogFont;

    // Store font as the global default
    memcpy(&LogFont, lpLogFont, sizeof(LOGFONT));

	m_fontHeight = lpLogFont->lfHeight;

	LogFont.lfHeight = m_DittoWindow.m_dpi.Scale(LogFont.lfHeight);

    // Create the actual font object
    m_Font.DeleteObject();
    m_Font.CreateFontIndirect(&LogFont);

    if(bRedraw && ::IsWindow(GetSafeHwnd()))
    {
        Invalidate();
    }

    return TRUE;
}

void CToolTipEx::SetGdiplusBitmap(std::unique_ptr<Gdiplus::Bitmap> gdiplusBitmap)
{
	m_imageViewer.m_pGdiplusBitmap = std::move(gdiplusBitmap);
	m_imageViewer.UpdateBitmapSize(true);
	Invalidate();
}

void CToolTipEx::OnSize(UINT nType, int cx, int cy)
{
    CWnd::OnSize(nType, cx, cy);

    if(::IsWindow(m_RichEdit.GetSafeHwnd()) == FALSE)
    {
        return ;
    }

	MoveControls();
}

void CToolTipEx::MoveControls()
{
	CRect cr;
	GetClientRect(cr);

	int bottom = m_DittoWindow.m_dpi.Scale(22);
	int optionsExtra = 0;

	if (m_folderPath != _T(""))
	{
		bottom += m_DittoWindow.m_dpi.Scale(17);
		optionsExtra += m_DittoWindow.m_dpi.Scale(10);
		m_folderPathStatic.ShowWindow(SW_SHOW);			
	}
	else
	{
		m_folderPathStatic.ShowWindow(SW_HIDE);
	}

	cr.DeflateRect(0, 0, 0, bottom);

	m_RichEdit.MoveWindow(cr);
	m_imageViewer.MoveWindow(cr);

	m_optionsButton.MoveWindow(cr.left, cr.bottom + m_DittoWindow.m_dpi.Scale(3) + optionsExtra, m_DittoWindow.m_dpi.Scale(17), m_DittoWindow.m_dpi.Scale(17));

	m_clipDataStatic.MoveWindow(cr.left + m_DittoWindow.m_dpi.Scale(19), cr.bottom + m_DittoWindow.m_dpi.Scale(4), cr.Width() - cr.left + m_DittoWindow.m_dpi.Scale(19), m_DittoWindow.m_dpi.Scale(20));

	m_folderPathStatic.MoveWindow(cr.left + m_DittoWindow.m_dpi.Scale(19), cr.bottom + m_DittoWindow.m_dpi.Scale(20), cr.Width() - cr.left + m_DittoWindow.m_dpi.Scale(19), m_DittoWindow.m_dpi.Scale(20));

	this->Invalidate();

	if (m_saveWindowLockout == false)
	{
		SetTimer(TimerSaveSize, 250, NULL);
	}
}

BOOL CToolTipEx::IsCursorInToolTip()
{
    CRect cr;
    GetWindowRect(cr);

    CPoint cursorPos;
    GetCursorPos(&cursorPos);

    return cr.PtInRect(cursorPos);
}

void CToolTipEx::SetRTFText(const CStringA &rtf)
{
	//OutputDebugStringA("SetRTF-" + rtf.Left(20) + "\r\n");
    m_RichEdit.SetRTF(rtf);
    m_csRTF = rtf;
	m_RichEdit.SetSel(0, 0);
	
	HighlightSearchText();
}

//void CToolTipEx::SetRTFText(const CString &csRTF)
//{
//	m_RichEdit.SetRTF(csRTF);
//	m_csRTF = csRTF;
//}

void CToolTipEx::SetToolTipText(const CString &csText)
{
	//OutputDebugString(_T("SetRTF-") + csText.Left(20) + "\r\n");

    m_csText = csText;
    m_RichEdit.SetFont(&m_Font);
    m_RichEdit.SetText(csText);
	m_RichEdit.SetSel(0, 0);

	CHARFORMAT cfNew;
	cfNew.cbSize = sizeof(CHARFORMAT);
	cfNew.dwMask = CFM_COLOR;
	cfNew.dwEffects = CFM_COLOR;
	cfNew.dwEffects &= ~CFE_AUTOCOLOR;
	cfNew.crTextColor = Settings().m_Theme.DescriptionWindowText();
	m_RichEdit.SetDefaultCharFormat(cfNew);

	HighlightSearchText();
}

void CToolTipEx::HighlightSearchText()
{
	if (m_searchText.GetLength() <= 0)
		return;

	FINDTEXTEX ft;
	long n = -1;

	ft.lpstrText = m_searchText;
	
	ft.chrg.cpMin = 0;
	ft.chrg.cpMax = -1;

	CHARFORMAT cf;

	cf.cbSize = sizeof(cf);
	cf.dwMask = CFM_COLOR;
	cf.dwEffects = CFE_BOLD | ~static_cast<DWORD>(CFE_AUTOCOLOR);
	cf.crTextColor = RGB(255, 0, 0);

	m_RichEdit.SetRedraw(0);
	auto mask = m_RichEdit.GetEventMask();
	m_RichEdit.SetEventMask(0);


	int matches = 0;
	do 
	{
		ft.chrg.cpMin = n+1;
		n = m_RichEdit.FindText(FR_DOWN, &ft);
		if (n != -1)
		{
			m_RichEdit.SetSel(ft.chrgText);
			m_RichEdit.SetSelectionCharFormat(cf);
		}

		matches++;

	} while (n != -1 && matches < 100);	

	m_RichEdit.SetSel(0, 0);
	m_RichEdit.SetEventMask(mask);
	m_RichEdit.SetRedraw(1);
	m_RichEdit.UpdateWindow();
}

void CToolTipEx::DoSearch()
{
	if (m_searchText.GetLength() <= 0)
		return;

	FINDTEXTEX ft;
	long n = -1;

	ft.lpstrText = m_searchText;

	long start;
	long end;
	m_RichEdit.GetSel(start, end);

	ft.chrg.cpMin = end;
	ft.chrg.cpMax = -1;

	int searchDirection = FR_DOWN;
	if (GetKeyState(VK_SHIFT) & 0x8000)
	{
		searchDirection = 0;
		ft.chrg.cpMin = start;
	}

	n = m_RichEdit.FindText(searchDirection, &ft);
	if (n != -1)
	{
		//if needing to scroll back to the left make sure the full text is visible
		//setSel scrolls to the end of the selection
		int matchStart = ft.chrgText.cpMin;
		if (matchStart < 0)
		{
			matchStart = 0;
		}
		m_RichEdit.SetSel(matchStart, matchStart);
		m_RichEdit.SetSel(ft.chrgText);
	}
	else
	{
		if (searchDirection == 0)
		{
			ft.chrg.cpMin = m_RichEdit.GetTextLength();
		}
		else
		{
			ft.chrg.cpMin = 0;
		}
		ft.chrg.cpMax = -1;

		n = m_RichEdit.FindText(searchDirection, &ft);
		if (n != -1)
		{
			//if needing to scroll back to the left make sure the full text is visible
			//setSel scrolls to the end of the selection
			int matchStart = ft.chrgText.cpMin;
			if (matchStart < 0)
			{
				matchStart = 0;
			}
			m_RichEdit.SetSel(matchStart, matchStart);
			m_RichEdit.SetSel(ft.chrgText);
		}
	}
}

void CToolTipEx::OnActivate(UINT nState, CWnd *pWndOther, BOOL bMinimized)
{
    CWnd::OnActivate(nState, pWndOther, bMinimized);

	if (nState == WA_INACTIVE)
	{		
        if(m_pNotifyWnd)
        {
            m_pNotifyWnd->PostMessage(CQListCtrl::NmInactiveToolTipWnd, 0, 0);
        }
    }
}

void CToolTipEx::OnTimer(UINT_PTR nIDEvent)
{
    switch(nIDEvent)
    {
        case TimerHideWindow:
            Hide();
            PostMessage(WM_DESTROY, 0, 0);
            break;
		case TimerSaveSize:
			SaveWindowSize();
			KillTimer(TimerSaveSize);
			break;
		case TimerButtonUp:
			OnButtonUpTimer();
			break;
		case TimerAutoMax:
			OnAutoMaxTimer();
			break;
    }

    CWnd::OnTimer(nIDEvent);
}

void CToolTipEx::OnButtonUpTimer()
{
	if ((GetKeyState(VK_LBUTTON) & 0x100) == 0)
	{
		m_DittoWindow.DoNcLButtonUp(this, 0, CPoint(0, 0));
		KillTimer(TimerButtonUp);

		auto f = GetFocus();
		if (f != NULL &&
			m_RichEdit.m_hWnd != f->m_hWnd)
		{
			auto p = GetParent();
			if (p != NULL)
			{
				p->SetFocus();
			}
		}
	}
}

void CToolTipEx::OnAutoMaxTimer()
{
	if (m_DittoWindow.m_bMinimized)
	{
		CPoint cp;
		GetCursorPos(&cp);

		UINT nHitTest = (UINT)OnNcHitTest(cp);

		ScreenToClient(&cp);

		if (nHitTest == HTCAPTION)
		{
			if (m_DittoWindow.m_crCloseBT.PtInRect(cp) == false)
			{
				if (m_DittoWindow.m_crMinimizeBT.PtInRect(cp) == false)
				{
					m_DittoWindow.MinMaxWindow(this, CDittoWindow::ForceMax);
				}
			}
		}
	}
	KillTimer(TimerAutoMax);
	m_bMaxSetTimer = false;
}


void CToolTipEx::OnNcPaint()
{
	m_DittoWindow.DoNcPaint(this);
}

void CToolTipEx::OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp) 
{
	CWnd::OnNcCalcSize(bCalcValidRects, lpncsp);

	m_DittoWindow.DoNcCalcSize(bCalcValidRects, lpncsp);
}

LRESULT CToolTipEx::OnNcHitTest(CPoint point) 
{
	UINT Ret = m_DittoWindow.DoNcHitTest(this, point);
	if(Ret == -1)
		return CWnd::OnNcHitTest(point);

	return Ret;
}

void CToolTipEx::OnNcLButtonDown(UINT nHitTest, CPoint point) 
{
	m_DittoWindow.DoNcLButtonDown(this, nHitTest, point);

	SetTimer(TimerButtonUp, 100, NULL);
	
	CWnd::OnNcLButtonDown(nHitTest, point);
}

void CToolTipEx::OnNcLButtonUp(UINT nHitTest, CPoint point) 
{
	long lRet = m_DittoWindow.DoNcLButtonUp(this, nHitTest, point);

	switch(lRet)
	{
	case CDittoWindow::ButtonClose:
		Hide();
		break;
	case CDittoWindow::ButtonChevron:
		m_DittoWindow.MinMaxWindow(this, CDittoWindow::SwapMinMax);
		OnNcPaint();
		break;
	}

	KillTimer(TimerButtonUp);

	auto f = GetFocus();

	if (f != NULL &&
		m_RichEdit.m_hWnd != f->m_hWnd)
	{
		auto p = GetParent();
		if (p != NULL)
		{
			p->SetFocus();
		}
	}

	CWnd::OnNcLButtonUp(nHitTest, point);
}

void CToolTipEx::OnNcMouseMove(UINT nHitTest, CPoint point) 
{
	if ((m_bMaxSetTimer == false) && m_DittoWindow.m_bMinimized)
	{
		COleDateTimeSpan sp = COleDateTime::GetCurrentTime() - m_DittoWindow.m_TimeMinimized;
		if (sp.GetTotalSeconds() >= m_lDelayMaxSeconds)
		{
			SetTimer(TimerAutoMax, Settings().GetTimeBeforeExpandWindow(), NULL);
			m_bMaxSetTimer = true;
		}
	}

	CWnd::OnNcMouseMove(nHitTest, point);
}

void CToolTipEx::OnOptions()
{
	POINT pp;
	CMenu cmPopUp;
	CMenu *cmSubMenu = NULL;

	GetCursorPos(&pp);
	if(cmPopUp.LoadMenu(IDR_DESC_OPTIONS_MENU) != 0)
	{
		cmSubMenu = cmPopUp.GetSubMenu(0);
		if(!cmSubMenu)
		{
			return ;
		}

		GetCursorPos(&pp);

		theApp.Services().Language().UpdateRightClickMenu(cmSubMenu);

		CheckOptionMenuItems(cmSubMenu);

		CheckViewMenuItems(cmSubMenu);

		UpdateMenuShortCut(cmSubMenu, ID_FIRST_WRAPTEXT, ActionEnums::TOGGLE_DESCRIPTION_WORD_WRAP);
		UpdateMenuShortCut(cmSubMenu, ID_FIRST_ALWAYSONTOP, ActionEnums::TOGGLESHOWPERSISTANT);

		if (Settings().GetWrapDescriptionText())
			cmSubMenu->CheckMenuItem(ID_FIRST_WRAPTEXT, MF_CHECKED);
		
		cmSubMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, pp.x, pp.y, this, NULL);
	}
}

void CToolTipEx::CheckOptionMenuItems(CMenu *cmSubMenu)
{
	if(Settings().GetRememberDescPos())
		cmSubMenu->CheckMenuItem(ID_FIRST_REMEMBERWINDOWPOSITION, MF_CHECKED);

	if(Settings().GetSizeDescWindowToContent())
		cmSubMenu->CheckMenuItem(ID_FIRST_SIZEWINDOWTOCONTENT, MF_CHECKED);

	if(Settings().GetScaleImagesToDescWindow())
		cmSubMenu->CheckMenuItem(ID_FIRST_SCALEIMAGESTOFITWINDOW, MF_CHECKED);

	if (Settings().GetMouseClickHidesDescription())
		cmSubMenu->CheckMenuItem(ID_FIRST_HIDEDESCRIPTIONWINDOWONM, MF_CHECKED);

	if (m_showPersistant)
		cmSubMenu->CheckMenuItem(ID_FIRST_ALWAYSONTOP, MF_CHECKED);
}

void CToolTipEx::CheckViewMenuItems(CMenu *cmSubMenu)
{
	if (m_showingText)
	{
		cmSubMenu->CheckMenuItem(ID_FIRST_VIEWTEXT, MF_CHECKED);
	}
	if (m_csText.GetLength() <= 0)
	{
		cmSubMenu->EnableMenuItem(ID_FIRST_VIEWTEXT, MF_DISABLED);
	}

	if (m_showingRTF)
	{
		cmSubMenu->CheckMenuItem(ID_FIRST_VIEWRTF, MF_CHECKED);
	}
	if (m_csRTF.GetLength() <= 0)
	{
		cmSubMenu->EnableMenuItem(ID_FIRST_VIEWRTF, MF_DISABLED);
	}

	if (m_showingImage)
	{
		cmSubMenu->CheckMenuItem(ID_FIRST_VIEWASIMAGE, MF_CHECKED);
	}
	if (m_imageViewer.m_pGdiplusBitmap == nullptr)
	{
		cmSubMenu->EnableMenuItem(ID_FIRST_VIEWASIMAGE, MF_DISABLED);
	}
}

void CToolTipEx::UpdateMenuShortCut(CMenu *subMenu, int id, DWORD action)
{
	if (m_pToolTipActions != NULL)
	{
		CString cs;
		subMenu->GetMenuString(id, cs, MF_BYCOMMAND);
		CString shortcutText = m_pToolTipActions->GetCmdKeyText(action);
		if (shortcutText != _T("") &&
			cs.Find("\t" + shortcutText) < 0)
		{
			cs += "\t";
			cs += shortcutText;
			subMenu->ModifyMenu(id, MF_BYCOMMAND, id, cs);
		}
	}
}

void CToolTipEx::OnRememberwindowposition()
{
	Settings().SetRememberDescPos(!Settings().GetRememberDescPos());
}

void CToolTipEx::OnSizewindowtocontent()
{
	Settings().SetSizeDescWindowToContent(!Settings().GetSizeDescWindowToContent());

	CRect rect;
	this->GetWindowRect(&rect);

	Show(rect.TopLeft());
}

void CToolTipEx::OnScaleimagestofitwindow()
{
	Settings().SetScaleImagesToDescWindow(!Settings().GetScaleImagesToDescWindow());
	m_imageViewer.UpdateBitmapSize(true);
	Invalidate();
}

void CToolTipEx::OnRButtonDown(UINT nFlags, CPoint point)
{
	OnOptions();

	CWnd::OnRButtonDown(nFlags, point);
}


void CToolTipEx::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	if (m_RichEdit.IsWindowVisible())
	{
		m_RichEdit.SetFocus();
	}
}

void CToolTipEx::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rect;
	GetClientRect(rect);
	
	CBrush  Brush, *pOldBrush;
	Brush.CreateSolidBrush(Settings().m_Theme.DescriptionWindowBG());

	pOldBrush = dc.SelectObject(&Brush);

	dc.FillRect(&rect, &Brush);

	// Cleanup
	dc.SelectObject(pOldBrush);
}

void CToolTipEx::OnFirstHidedescriptionwindowonm()
{
	Settings().SetMouseClickHidesDescription(!Settings().GetMouseClickHidesDescription());
}

bool CToolTipEx::ToggleWordWrap()
{
	bool didWordWrap = false;
	if (m_RichEdit.IsWindowVisible())
	{
		OnFirstWraptext();
		didWordWrap = true;
	}

	return didWordWrap;
}

void CToolTipEx::OnFirstWraptext()
{
	Settings().SetWrapDescriptionText(!Settings().GetWrapDescriptionText());	
	ApplyWordWrap();
}

void CToolTipEx::ApplyWordWrap()
{
	if (Settings().GetWrapDescriptionText())
	{
		m_RichEdit.SetTargetDevice(NULL, 0);
	}
	else
	{
		m_RichEdit.SetTargetDevice(NULL, 1);
	}
}

void CToolTipEx::HideWindowInXMilliSeconds(long lms) 
{ 
	SetTimer(TimerHideWindow, lms, NULL); 
}

void CToolTipEx::OnWindowPosChanging(WINDOWPOS* lpwndpos)
{
	CWnd::OnWindowPosChanging(lpwndpos);

	//m_DittoWindow.SnapToEdge(this, lpwndpos);
}


void CToolTipEx::OnFirstAlwaysontop()
{
	m_showPersistant = !m_showPersistant;
	if (m_showPersistant)
	{
		m_DittoWindow.m_customWindowTitle = _T("[Always on top]");
		m_DittoWindow.m_useCustomWindowTitle = true;
		::SetWindowPos(m_hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE | SWP_SHOWWINDOW | SWP_NOACTIVATE);
	}
	else
	{
		m_DittoWindow.m_customWindowTitle = _T("");
		m_DittoWindow.m_useCustomWindowTitle = true;
	}

	::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);	
}



void CToolTipEx::OnEnMsgfilterRichedit21(NMHDR *pNMHDR, LRESULT *pResult)
{
	MSGFILTER *pMsgFilter = reinterpret_cast<MSGFILTER *>(pNMHDR);
	if (pMsgFilter != NULL)
	{
		switch (pMsgFilter->msg)
		{
			//handle click on the rich text control when it doesn't have focus
			//set focus so the first click is handled by the rich text control
		case WM_MOUSEACTIVATE:
			m_RichEdit.SetFocus();
			break;
		case WM_MOUSEHWHEEL:			
			int delta = GET_WHEEL_DELTA_WPARAM(pMsgFilter->wParam);
			if (delta < 0)
			{			
				m_RichEdit.SendMessage(WM_HSCROLL, SB_LINERIGHT, NULL);
			}
			else
			{
				m_RichEdit.SendMessage(WM_HSCROLL, SB_LINELEFT, NULL);
			}
			break;
		}
	}

	*pResult = 0;
}

LRESULT CToolTipEx::OnDpiChanged(WPARAM wParam, LPARAM lParam)
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

	CLogger::Write(CStringUtil::Format(_T("CQPasteWnd::OnDpiChanged dpi: %d width: %d, height: %d"), dpi, (prcNewWindow->right - prcNewWindow->left), (prcNewWindow->bottom - prcNewWindow->top)));

	m_optionsButton.Reset();
	m_optionsButton.LoadStdImageDPI(m_DittoWindow.m_dpi.GetDPI(), IDB_COG_16_16, IDB_COG_20_20, IDB_COG_24_24, cog_28, IDB_COG_32_32, _T("PNG"));

	m_clipDataFont.DeleteObject();
	m_clipDataFont.CreateFont(-m_DittoWindow.m_dpi.Scale(8), 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 3, 2, 1, 34, _T("Segoe UI"));
	
	m_clipDataStatic.SetFont(&m_clipDataFont);
	m_clipDataStatic.SetBkColor(Settings().m_Theme.DescriptionWindowBG());
	m_clipDataStatic.SetTextColor(RGB(80, 80, 80));

	m_folderPathStatic.SetFont(&m_clipDataFont);
	m_folderPathStatic.SetBkColor(Settings().m_Theme.DescriptionWindowBG());
	m_folderPathStatic.SetTextColor(RGB(80, 80, 80));

	LOGFONT lf;
	m_Font.GetLogFont(&lf);
	lf.lfHeight = m_DittoWindow.m_dpi.Scale(m_fontHeight);

	// Create the actual font object
	m_Font.DeleteObject();
	m_Font.CreateFontIndirect(&lf);

	m_RichEdit.SetFont(&m_Font);

	this->MoveControls();
	this->Invalidate();
	this->UpdateWindow();

	return TRUE;
}

void CToolTipEx::OnMoving(UINT fwSide, LPRECT pRect)
{
	CWnd::OnMoving(fwSide, pRect); 

	m_snap.OnSnapMoving(m_hWnd, pRect);
	// TODO: Add your message handler code here
}


void CToolTipEx::OnEnterSizeMove()
{
	m_snap.OnSnapEnterSizeMove(m_hWnd);

	CWnd::OnEnterSizeMove();
}


void CToolTipEx::OnHScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar* /*pScrollBar*/)
{
	//m_scrollHelper.OnHScroll(nSBCode, nPos, pScrollBar);
}

LRESULT CToolTipEx::OnRefreshFooter(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_clipData = m_originalClipData;
	if (m_imageViewer.m_pGdiplusBitmap)
	{
		int percent = static_cast<int>(((m_imageViewer.m_scale) * 100.0) + .5);
		m_clipData = m_originalClipData + _T(" | ") + CStringUtil::Format(_T("%d x %d, %d%%"), m_imageViewer.m_pGdiplusBitmap->GetWidth(), m_imageViewer.m_pGdiplusBitmap->GetHeight(), percent);
	}

	m_clipDataStatic.SetWindowText(m_clipData);
	m_clipDataStatic.Invalidate();

	this->Invalidate();

	return TRUE;
}

void CToolTipEx::OnFirstViewtext()
{
	m_imageViewer.ShowWindow(SW_HIDE);

	m_RichEdit.SetText(m_csText);

	m_RichEdit.SetSel(0, 0);
	HighlightSearchText();

	m_RichEdit.ShowWindow(SW_SHOW);

	this->Invalidate();
	this->UpdateWindow();

	m_showingText = true;
	m_showingRTF = false;
	m_showingImage = false;
}

void CToolTipEx::OnFirstViewrtf()
{
	m_imageViewer.ShowWindow(SW_HIDE);

	m_RichEdit.SetRTF(m_csRTF);

	m_RichEdit.SetSel(0, 0);
	HighlightSearchText();

	m_RichEdit.ShowWindow(SW_SHOW);

	this->Invalidate();
	this->UpdateWindow();

	m_showingText = false;
	m_showingRTF = true;
	m_showingImage = false;
}


void CToolTipEx::OnFirstViewImage()
{
	m_RichEdit.ShowWindow(SW_HIDE);

	if (m_imageViewer.m_pGdiplusBitmap)
	{
		m_imageViewer.UpdateBitmapSize(true);
		m_imageViewer.ShowWindow(SW_SHOW);
	}

	m_showingText = false;
	m_showingRTF = false;
	m_showingImage = true;
}


void CToolTipEx::OnUpdateFirstViewtext(CCmdUI* pCmdUI)
{
	if (m_csText.IsEmpty())
	{
		pCmdUI->Enable(0);
	}
}


void CToolTipEx::OnUpdateFirstViewrtf(CCmdUI* pCmdUI)
{
	if (m_csRTF.IsEmpty())
	{
		pCmdUI->Enable(0);
	}
}



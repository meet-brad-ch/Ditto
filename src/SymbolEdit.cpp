// AeroEdit.cpp : implementation file
//

#include "stdafx.h"
#include "SymbolEdit.h"
#include "cp_main.h"
#include "QListCtrl.h"
#include "..\Shared\TextConvert.h"
#include <tinyxml2.h>
#include <stdexcept>

// CSymbolEdit

IMPLEMENT_DYNAMIC(CSymbolEdit, CEdit)

CSymbolEdit::CSymbolEdit() :
	m_colorPromptText(RGB(127, 127, 127)),
	m_centerTextDiff(0)
{
	m_fontPrompt.CreateFont(
		16,                       // nHeight
		0,                        // nWidth
		0,                        // nEscapement
		0,                        // nOrientation
		FW_NORMAL,                // nWeight
		TRUE,                     // bItalic
		FALSE,                    // bUnderline
		0,                        // cStrikeOut
		DEFAULT_CHARSET,          // nCharSet
		OUT_DEFAULT_PRECIS,       // nOutPrecision
		CLIP_DEFAULT_PRECIS,      // nClipPrecision
		DEFAULT_QUALITY,          // nQuality
		DEFAULT_PITCH | FF_SWISS, // nPitchAndFamily
		_T("Calibri"));

	m_mouseDownOnSearches = false;
	m_mouseHoveringOverSearches = false;
	m_mouseDownOnClose = false;
	m_mouseHoveringOverClose = false;
	m_windowDpi = NULL;

	//m_searchButton.LoadStdImageDPI(Search_16, Search_20, Search_24, Search_32, _T("PNG"));
}

CSymbolEdit::~CSymbolEdit()
{
	DestroyIcon();
}


BEGIN_MESSAGE_MAP(CSymbolEdit, CEdit)
	ON_WM_PAINT()
	ON_MESSAGE(WM_SETFONT, OnSetFont)
	//ON_MESSAGE(WM_EXITMENULOOP, OnMenuExit)
	ON_WM_CTLCOLOR_REFLECT()
	ON_WM_SETFOCUS()
	ON_WM_KILLFOCUS()
	ON_WM_SETCURSOR()
	ON_WM_LBUTTONUP()
	ON_WM_LBUTTONDOWN()
	ON_WM_MOUSEMOVE()
	ON_COMMAND_RANGE(IdRangeStart, (IdRangeStart + s_listMaxCount), OnSelectSearchString)
	ON_WM_EXITSIZEMOVE()
	//ON_WM_ERASEBKGND()
	ON_WM_NCCALCSIZE()
	ON_WM_NCPAINT()
	ON_WM_TIMER()
END_MESSAGE_MAP()

BOOL CSymbolEdit::PreTranslateMessage(MSG* pMsg)
{
	// TODO: Add your specialized code here and/or call the base class
	// Intercept Ctrl + Z (Undo), Ctrl + X (Cut), Ctrl + C (Copy), Ctrl + V (Paste) and Ctrl + A (Select All)
	// before CEdit base class gets a hold of them.
	if (pMsg->message == WM_KEYDOWN &&
		CKeyboard::IsControlPressed())
	{
		if (HandleControlKey(pMsg))
		{
			return TRUE;
		}
	}

	if (pMsg->message == WM_KEYDOWN &&
		HandleKeyDown(pMsg))
	{
		return TRUE;
	}

	return CEdit::PreTranslateMessage(pMsg);
}

bool CSymbolEdit::HandleControlKey(MSG* pMsg)
{
	switch (pMsg->wParam)
	{
	case 'Z':
		Undo();
		return true;
	case 'X':
		Cut();
		return true;
	case 'C':
		CopySelectionOrClip(pMsg);
		return true;
	case 'V':
		Paste();
		return true;
	case 'A':
		SetSel(0, -1);
		return true;
	}

	return false;
}

void CSymbolEdit::CopySelectionOrClip(const MSG* pMsg)
{
	int startChar{};
	int endChar{};
	this->GetSel(startChar, endChar);
	if (startChar == endChar)
	{
		SendKeyToParent(CQListCtrl::NmCopyClip, pMsg);
	}
	else
	{
		Copy();
	}
}

bool CSymbolEdit::SendKeyToParent(UINT message, const MSG* pMsg)
{
	CWnd* pWnd = GetParent();
	if (pWnd)
	{
		pWnd->SendMessage(message, pMsg->wParam, pMsg->lParam);
		return true;
	}

	return false;
}

bool CSymbolEdit::IsHistoryMenuKeyState()
{
	// Ctrl, with or without Shift
	return (GetKeyState(VK_CONTROL) & 0x8000) != 0;
}

bool CSymbolEdit::IsListNavigationKey(WPARAM key)
{
	return key == VK_DOWN ||
		   key == VK_UP ||
		   key == VK_PRIOR ||
		   key == VK_NEXT;
}

bool CSymbolEdit::HandleKeyDown(MSG* pMsg)
{
	if (pMsg->wParam == VK_RETURN)
	{
		HandleReturnKey();
		return true;
	}
	else if (pMsg->wParam == VK_DOWN &&
			 IsHistoryMenuKeyState())
	{
		if (ShowSearchHistoryMenu())
		{
			return true;
		}
	}
	else if (IsListNavigationKey(pMsg->wParam))
	{
		return SendKeyToParent(CQListCtrl::CbUpDown, pMsg);
	}
	else if (pMsg->wParam == VK_DELETE)
	{
		return HandleDeleteKey(pMsg);
	}

	return false;
}

CGetSetOptions& CSymbolEdit::Settings() const
{
	return theApp.Services().Settings();
}

void CSymbolEdit::HandleReturnKey()
{
	CWnd* pWnd = GetParent();
	if (pWnd)
	{
		if (Settings().m_bFindAsYouType)
		{
			pWnd->SendMessage(CQListCtrl::NmSearchEnterPressed, 0, 0);
		}
		else
		{
			//Send a message to the parent to refill the lb from the search
			pWnd->PostMessage(CQListCtrl::CbSearch, 0, 0);
		}

		AddToSearchHistory();
	}
}

bool CSymbolEdit::HandleDeleteKey(const MSG* pMsg)
{
	int startChar{};
	int endChar{};
	this->GetSel(startChar, endChar);
	CString cs;
	this->GetWindowText(cs);
	//if selection is at the end then forward this on to the parent to delete the selected clip
	if (startChar == cs.GetLength() &&
		endChar == cs.GetLength())
	{
		return SendKeyToParent(CQListCtrl::NmDelete, pMsg);
	}

	return false;
}

CString CSymbolEdit::SavePastSearches()
{
	tinyxml2::XMLDocument doc;

	// the document owns the elements it creates
	tinyxml2::XMLElement* outer = doc.NewElement("PastSearches");
	doc.InsertEndChild(outer);

	int count = (int)m_searches.GetCount();
	for (int i = 0; i < count; i++)
	{
		tinyxml2::XMLElement* searchElement = doc.NewElement("Search");

		CStringA t = CTextConvert::UnicodeToUTF8(m_searches[i]);
		searchElement->SetAttribute("text", t);

		outer->InsertEndChild(searchElement);
	}

	tinyxml2::XMLPrinter printer(nullptr, true);
	doc.Print(&printer);
	// the XML is UTF-8; it was converted as ANSI before, which garbled non-ASCII searches
	return CTextConvert::Utf8ToUnicode(printer.CStr());
}

void CSymbolEdit::LoadPastSearches(CString values)
{
	m_searches.RemoveAll();

	tinyxml2::XMLDocument doc;
	CStringA xmlA = CTextConvert::UnicodeToUTF8(values);
	doc.Parse(xmlA);

	const tinyxml2::XMLElement* ItemHeader = doc.FirstChildElement("PastSearches");

	if (ItemHeader != NULL)
	{
		const tinyxml2::XMLElement* ItemElement = ItemHeader->FirstChildElement();

		int count = 0;

		while (ItemElement)
		{
			if (count < s_listMaxCount)
			{
				// the attribute is UTF-8
				CString item = CTextConvert::Utf8ToUnicode(ItemElement->Attribute("text"));

				CString toAdd = item.Left(s_maxSavedSearchLength);
				if (toAdd != _T(""))
				{
					m_searches.Add(toAdd);
				}

				ItemElement = ItemElement->NextSiblingElement();
			}
			else
			{
				break;
			}

			count++;
		}
	}
}

void CSymbolEdit::AddToSearchHistory()
{
	CString cs;
	this->GetWindowText(cs);
	if (cs != _T(""))
	{
		//only save up to 50, had reports of somehow getting extremely large amounts of junk text
		//save and causing memory issues.
		cs = cs.Left(s_maxSavedSearchLength);

		if (m_searches.GetCount() >= s_listMaxCount)
		{
			m_searches.RemoveAt(0);
		}

		bool existing = false;
		int count = (int)m_searches.GetCount();
		for (int i = 0; i < count; i++)
		{
			if (m_searches[i] == cs)
			{
				m_searches.RemoveAt(i);
				m_searches.Add(cs);
				existing = true;
				break;
			}
		}

		if (existing == false)
		{
			m_searches.Add(cs);
		}
	}
}

bool CSymbolEdit::ShowSearchHistoryMenu()
{
	if (m_searches.GetCount() == 0)
	{
		return false;
	}

	CMenu cmPopUp;
	cmPopUp.CreatePopupMenu();

	int count = min((int)m_searches.GetCount(), s_listMaxCount);
	for (int i = count - 1; i >= 0; i--)
	{
		CString text = m_searches[i];

		if (i == count - 1 &&
			m_lastSearchShortCut.Key > 0)
		{
			CString cmdShortcutText = CHotKey::GetHotKeyDisplayStatic(m_lastSearchShortCut.Key);
			if (m_lastSearchShortCut.Key2 != 0)
			{
				CString cmdShortcutText2 = CHotKey::GetHotKeyDisplayStatic(m_lastSearchShortCut.Key2);

				if (cmdShortcutText2.GetLength() > 0)
				{
					cmdShortcutText += _T(" - ");
					cmdShortcutText += cmdShortcutText2;
				}
			}

			text += "\t";
			text += cmdShortcutText;
		}

		cmPopUp.AppendMenuW(MF_STRING, (IdRangeStart + i), text);
	}

	cmPopUp.AppendMenu(MF_SEPARATOR);
	cmPopUp.AppendMenuW(MF_STRING, IdClearList, _T("Clear List"));

	CRect windowRect;
	this->GetWindowRect(&windowRect);
	POINT pp;
	GetCursorPos(&pp);
	POINT x = this->GetCaretPos();
	ClientToScreen(&x);
	x.y += windowRect.Height();

	cmPopUp.TrackPopupMenu(TPM_LEFTALIGN | TPM_TOPALIGN | TPM_RIGHTBUTTON, x.x, x.y, this, NULL);

	Invalidate();

	return true;
}

void CSymbolEdit::DestroyIcon()
{
	// the icon is owned whether it was loaded internally or handed in: destroy it
	m_hSymbolIcon.reset();
}

void CSymbolEdit::PreSubclassWindow()
{
	RecalcLayout();
}

void CSymbolEdit::SetSymbolIcon(HICON hIcon, BOOL redraw)
{
	DestroyIcon();

	m_hSymbolIcon.reset(hIcon);

	RecalcLayout();

	if (redraw)
		Invalidate(TRUE);
}

void CSymbolEdit::SetSymbolIcon(UINT id, BOOL redraw)
{
	DestroyIcon();

	m_hSymbolIcon.reset(static_cast<HICON>(::LoadImage(
		AfxGetResourceHandle(),
		MAKEINTRESOURCE(id),
		IMAGE_ICON,
		16,
		16,
		LR_DEFAULTCOLOR | LR_LOADTRANSPARENT)));

	ASSERT(m_hSymbolIcon != nullptr);

	RecalcLayout();

	if (redraw)
		Invalidate(TRUE);
}

void CSymbolEdit::SetPromptText(CString text, BOOL redraw)
{
	m_strPromptText = text;

	if (redraw)
		Invalidate(TRUE);
}

void CSymbolEdit::SetPromptText(LPCTSTR szText, BOOL redraw)
{
	m_strPromptText = szText;

	if (redraw)
		Invalidate(TRUE);
}

void CSymbolEdit::SetPromptTextColor(COLORREF color, BOOL redraw)
{
	m_colorPromptText = color;

	if (redraw)
		Invalidate(TRUE);
}

void CSymbolEdit::SetPromptFont(CFont& font, BOOL redraw)
{
	LOGFONT lf;
	memset(&lf, 0, sizeof(LOGFONT));

	font.GetLogFont(&lf);
	SetPromptFont(&lf);

	if (redraw)
		Invalidate(TRUE);
}

void CSymbolEdit::SetPromptFont(const LOGFONT* lpLogFont, BOOL redraw)
{
	m_fontPrompt.DeleteObject();
	m_fontPrompt.CreateFontIndirect(lpLogFont);

	if (redraw)
		Invalidate(TRUE);
}

void CSymbolEdit::RecalcLayout()
{
	int width = GetSystemMetrics(SM_CXSMICON);

	if (m_hSymbolIcon)
	{
		DWORD dwMargins = GetMargins();
		SetMargins(LOWORD(dwMargins), width + 6);
	}
	else
	{
		if (m_windowDpi != NULL)
		{
			SetMargins(m_windowDpi->Scale(4), m_windowDpi->Scale(34));
		}
	}
}

void CSymbolEdit::OnPaint()
{
	if (m_windowDpi == NULL)
	{
		// before SetDpiInfo there is no scale for the buttons: the edit control paints itself
		CEdit::OnPaint();
		return;
	}

	CPaintDC dc(this);

	CRect rect;
	GetClientRect(&rect);

	DWORD margins = GetMargins();

	CRect textRect(rect);
	textRect.left += LOWORD(margins);
	textRect.right -= HIWORD(margins);

	// Clearing the background
	dc.FillSolidRect(rect, GetSysColor(COLOR_WINDOW));

	if (m_hSymbolIcon)
	{
		DrawSymbolIcon(dc, rect, margins);
	}

	CString text;
	GetWindowText(text);

	DrawTextArea(dc, rect, textRect, text);

	if (text.GetLength() == 0 && m_strPromptText.GetLength() > 0)
	{
		DrawPromptText(dc, textRect);
	}

	DrawButtons(dc, rect, text);

	if (text != m_lastTextOnPaint &&
		text == _T(""))
	{
		::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
	}

	m_lastTextOnPaint = text;
}

void CSymbolEdit::DrawButtons(CDC& dc, const CRect& rect, const CString& text)
{
	int right = rect.right;
	if ((text.GetLength() > 0 || this == GetFocus()))
	{
		m_searchesButtonRect.SetRect(rect.right - m_windowDpi->Scale(18), 0, rect.right, rect.bottom);
		right = rect.right - m_windowDpi->Scale(18);
		m_searchesButton.Draw(&dc, *m_windowDpi, this, m_searchesButtonRect.left, 4, m_mouseHoveringOverSearches, m_mouseDownOnSearches);
	}
	else
	{
		m_searchesButtonRect.SetRect(0, 0, 0, 0);
	}

	if (text.GetLength() > 0)
	{
		m_closeButtonRect.SetRect(right - m_windowDpi->Scale(16), 0, right, rect.bottom);
		m_closeButton.Draw(&dc, *m_windowDpi, this, m_closeButtonRect.left, 4, m_mouseHoveringOverClose, m_mouseDownOnClose);
	}
	else
	{
		m_closeButtonRect.SetRect(0, 0, 0, 0);
	}
}

void CSymbolEdit::DrawSymbolIcon(CDC& dc, CRect& rect, DWORD margins)
{
	// Drawing the icon
	int width = GetSystemMetrics(SM_CXSMICON);
	int height = GetSystemMetrics(SM_CYSMICON);

	::DrawIconEx(
		dc.m_hDC,
		rect.right - width - 1,
		1,
		m_hSymbolIcon.get(),
		width,
		height,
		0,
		NULL,
		DI_NORMAL);

	rect.left += LOWORD(margins) + 1;
	rect.right -= (width + 7);
}

void CSymbolEdit::DrawTextArea(CDC& dc, const CRect& rect, const CRect& textRect, const CString& text)
{
	if (this == GetFocus() || text.GetLength() > 0)
	{
		dc.FillSolidRect(rect, Settings().m_Theme.SearchTextBoxFocusBG());

		//CBrush borderBrush(Settings().m_Theme.SearchTextBoxFocusBorder());
		//dc.FrameRect(rect, &borderBrush);

		//rect.DeflateRect(1, 1, 1, 1);
		//textRect.DeflateRect(0, 1, 1, 1);

		CFont* oldFont = dc.SelectObject(GetFont());

		COLORREF oldColor = dc.GetTextColor();
		dc.SetTextColor(Settings().m_Theme.SearchTextBoxFocusText());

		CRect drawRect(textRect);
		dc.DrawText(text, drawRect, DT_SINGLELINE | DT_INTERNAL | DT_EDITCONTROL | DT_NOPREFIX);

		dc.SelectObject(oldFont);
		dc.SetTextColor(oldColor);
	}
	else
	{
		dc.FillSolidRect(rect, Settings().m_Theme.MainWindowBG());
	}
}

void CSymbolEdit::DrawPromptText(CDC& dc, CRect textRect)
{
	//if we aren't showing the close icon, then use the full space
	textRect.right += m_windowDpi->Scale(16);
	//textRect.right -= LOWORD(margins);

	CFont* oldFont = dc.SelectObject(&m_fontPrompt);
	COLORREF color = dc.GetTextColor();
	dc.SetTextColor(m_colorPromptText);

	dc.DrawText(m_strPromptText, textRect, DT_LEFT | DT_SINGLELINE | DT_EDITCONTROL | DT_VCENTER | DT_NOPREFIX);
	dc.SetTextColor(color);
	dc.SelectObject(oldFont);
}

void CSymbolEdit::OnSize(UINT nType, int cx, int cy)
{
	CEdit::OnSize(nType, cx, cy);

	RecalcLayout();
}

LRESULT CSymbolEdit::OnSetFont(WPARAM wParam, LPARAM lParam)
{
	DefWindowProc(WM_SETFONT, wParam, lParam);

	RecalcLayout();

	return 0;
}

HBRUSH CSymbolEdit::CtlColor(CDC* pDC, UINT /*n*/)
{
	COLORREF color = CLR_INVALID;

	if (::GetFocus() == m_hWnd)
	{
		pDC->SetTextColor(Settings().m_Theme.SearchTextBoxFocusText());
		pDC->SetBkColor(Settings().m_Theme.SearchTextBoxFocusBG());
		color = Settings().m_Theme.SearchTextBoxFocusBG();
	}
	else
	{
		pDC->SetBkColor(Settings().m_Theme.MainWindowBG());
		color = Settings().m_Theme.MainWindowBG();
	}

	if (color != m_lastBrushColor)
	{
		m_brush.DeleteObject();
		m_brush.CreateSolidBrush(color);
		m_lastBrushColor = color;
	}

	return m_brush;
}

void CSymbolEdit::OnSetFocus(CWnd* pOldWnd)
{
	//OutputDebugString(_T("OnSetFocus \r\n"));

	//was seeing issues when refreshing non client area inline, do it delayed
	SetTimer(1, 500, NULL);

	CWnd* pWnd = GetParent();
	if (pWnd)
	{
		if (Settings().m_bFindAsYouType)
		{
			pWnd->SendMessage(CQListCtrl::NmFocusOnSearch, 0, 0);
		}
	}

	CEdit::OnSetFocus(pOldWnd);
}

void CSymbolEdit::OnKillFocus(CWnd* pNewWnd)
{
	//OutputDebugString(_T("OnKillFocus \r\n"));
	AddToSearchHistory();

	//was seeing issues when refreshing non client area inline, do it delayed
	SetTimer(1, 500, NULL);

	CEdit::OnKillFocus(pNewWnd);
}

BOOL CSymbolEdit::OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message)
{
	CPoint pntCursor;
	GetCursorPos(&pntCursor);
	ScreenToClient(&pntCursor);

	if (m_closeButtonRect.PtInRect(pntCursor))
	{
		HCURSOR h = ::LoadCursor(NULL, IDC_ARROW);
		::SetCursor(h);
		return TRUE;
	}

	if (m_searchesButtonRect.PtInRect(pntCursor))
	{
		HCURSOR h = ::LoadCursor(NULL, IDC_ARROW);
		::SetCursor(h);
		return TRUE;
	}

	return CEdit::OnSetCursor(pWnd, nHitTest, message);
}

void CSymbolEdit::OnLButtonUp(UINT nFlags, CPoint point)
{
	if (m_mouseDownOnClose)
	{
		ReleaseCapture();
		InvalidateRect(m_closeButtonRect);
	}

	if (m_mouseDownOnSearches)
	{
		ReleaseCapture();
		InvalidateRect(m_searchesButtonRect);
	}

	m_mouseDownOnClose = false;
	m_mouseDownOnSearches = false;

	if (m_closeButtonRect.PtInRect(point))
	{
		if ((GetWindowTextLength() > 0))
		{
			CWnd* pOwner = GetOwner();
			if (pOwner)
			{
				pOwner->SendMessage(CQListCtrl::NmCancelSearch, 0, 0);
			}
		}
	}

	if (m_searchesButtonRect.PtInRect(point))
	{
		this->ShowSearchHistoryMenu();
	}

	CEdit::OnLButtonUp(nFlags, point);
}

void CSymbolEdit::OnLButtonDown(UINT nFlags, CPoint point)
{
	if (m_closeButtonRect.PtInRect(point))
	{
		m_mouseDownOnClose = true;
		SetCapture();
		InvalidateRect(m_closeButtonRect);
	}
	else
	{
		m_mouseDownOnClose = false;
	}

	if (m_searchesButtonRect.PtInRect(point))
	{
		m_mouseDownOnSearches = true;
		SetCapture();
		InvalidateRect(m_searchesButtonRect);
	}
	else
	{
		m_mouseDownOnSearches = false;
	}

	CEdit::OnLButtonDown(nFlags, point);

	Invalidate();
}

void CSymbolEdit::OnMouseMove(UINT nFlags, CPoint point)
{
	if (m_closeButtonRect.PtInRect(point))
	{
		if (m_mouseHoveringOverClose == false)
		{
			m_mouseHoveringOverClose = true;
			InvalidateRect(m_closeButtonRect);
		}
	}
	else if (m_mouseHoveringOverClose)
	{
		m_mouseHoveringOverClose = false;
		InvalidateRect(m_closeButtonRect);
	}

	if (m_searchesButtonRect.PtInRect(point))
	{
		if (m_mouseHoveringOverSearches == false)
		{
			m_mouseHoveringOverSearches = true;
			InvalidateRect(m_searchesButtonRect);
		}
	}
	else if (m_mouseHoveringOverSearches)
	{
		m_mouseHoveringOverSearches = false;
		InvalidateRect(m_searchesButtonRect);
	}

	CEdit::OnMouseMove(nFlags, point);
}

void CSymbolEdit::OnSelectSearchString(UINT idIn)
{
	int index = idIn - IdRangeStart;

	if (idIn == IdClearList)
	{
		m_searches.RemoveAll();
	}
	else if (index >= 0 &&
			 index < m_searches.GetCount())
	{
		CString cs = m_searches[index];
		this->SetWindowTextW(cs);

		this->SetFocus();
		this->SetSel(static_cast<DWORD>(-1));

		this->Invalidate();

		m_searches.RemoveAt(index);
		m_searches.Add(cs);
	}
}

bool CSymbolEdit::ApplyLastSearch()
{
	bool ret = false;
	if (m_searches.GetCount() > 0)
	{
		CString cs = m_searches[m_searches.GetCount() - 1];
		this->SetWindowTextW(cs);

		this->SetFocus();
		this->SetSel(static_cast<DWORD>(-1));

		this->Invalidate();

		ret = true;
	}

	return ret;
}

void CSymbolEdit::OnDpiChanged()
{
	SetDpiInfo(m_windowDpi);
}

void CSymbolEdit::SetDpiInfo(CDPI* dpi)
{
	if (dpi == NULL)
	{
		throw std::invalid_argument("CSymbolEdit needs the window's DPI");
	}
	m_windowDpi = dpi;

	m_closeButton.Reset();
	m_closeButton.LoadStdImageDPI(m_windowDpi->GetDPI(), search_close_16, search_close_20, search_close_24, search_close_28, search_close_32, _T("PNG"));

	m_searchesButton.Reset();
	m_searchesButton.LoadStdImageDPI(m_windowDpi->GetDPI(), down_16, down_20, down_24, down_28, down_32, _T("PNG"));

	RecalcLayout();

	Invalidate();
}

BOOL CSymbolEdit::OnEraseBkgnd(CDC* /*pDC*/)
{
	// TODO: Add your message handler code here and/or call default

	//return CEdit::OnEraseBkgnd(pDC);
	return FALSE;
}


void CSymbolEdit::OnNcCalcSize(BOOL /*bCalcValidRects*/, NCCALCSIZE_PARAMS* lpncsp)
{
	CString text;
	GetWindowText(text);

	//if (text.GetLength() > 0 || this == GetFocus())
	if (m_windowDpi != NULL)
	{
		lpncsp->rgrc[0].left += m_windowDpi->Scale(1);
		lpncsp->rgrc[0].top += m_windowDpi->Scale(1);
		lpncsp->rgrc[0].right -= m_windowDpi->Scale(1);
		lpncsp->rgrc[0].bottom -= m_windowDpi->Scale(1);


		CRect rectWnd, rectClient;

		////calculate client area height needed for a font
		CFont* pFont = GetFont();
		CRect rectText;


		CDC* pDC = GetDC();

		CFont* pOld = pDC->SelectObject(pFont);
		pDC->DrawText("Ky", rectText, DT_CALCRECT | DT_LEFT);
		int uiVClientHeight = rectText.Height();

		pDC->SelectObject(pOld);
		ReleaseDC(pDC);


		////calculate NC area to center text.

		//GetClientRect(rectClient);
		GetWindowRect(rectWnd);

		rectWnd.DeflateRect(m_windowDpi->Scale(1), m_windowDpi->Scale(1));

		m_centerTextDiff = (rectWnd.Height() - uiVClientHeight) / 2;

		if (m_centerTextDiff < 0 || m_centerTextDiff > uiVClientHeight)
		{
			m_centerTextDiff = 0;
		}

		lpncsp->rgrc[0].top += m_centerTextDiff;
		lpncsp->rgrc[0].bottom -= m_centerTextDiff;
	}

	//ClientToScreen(rectClient);

	//UINT uiCenterOffset = (rectWnd.Height() - uiVClientHeight) / 2;
	//UINT uiCY = (rectWnd.Heig%ht() - rectClient.Height()) / 2;
	//UINT uiCX = (rectWnd.Width() - rectClient.Width()) / 2;

	//rectWnd.OffsetRect(-rectWnd.left, -rectWnd.top);
	//m_rectNCTop = rectWnd;

	//m_rectNCTop.DeflateRect(uiCX, uiCY, uiCX, uiCenterOffset + uiVClientHeight + uiCY);

	//m_rectNCBottom = rectWnd;

	//m_rectNCBottom.DeflateRect(uiCX, uiCenterOffset + uiVClientHeight + uiCY, uiCX, uiCY);

	//lpncsp->rgrc[0].top += uiCenterOffset;
	//lpncsp->rgrc[0].bottom -= uiCenterOffset;

	//lpncsp->rgrc[0].left += uiCX;
	//lpncsp->rgrc[0].right -= uiCY;

	//CEdit::OnNcCalcSize(bCalcValidRects, lpncsp);
}


void CSymbolEdit::OnNcPaint()
{
	if (m_windowDpi == NULL)
	{
		// before SetDpiInfo there is no scale for the border: the edit control paints its frame
		CEdit::OnNcPaint();
		return;
	}

	CString text;
	GetWindowText(text);

	CWindowDC dc(this);

	CRect r;
	this->GetWindowRect(r);
	this->ScreenToClient(r);

	CRect t(0, 0, r.Width(), m_centerTextDiff + m_windowDpi->Scale(1));

	CRect b(0, r.Height() - m_centerTextDiff - m_windowDpi->Scale(1), r.Width(), r.Height());

	COLORREF c = Settings().m_Theme.MainWindowBG();

	if (this == GetFocus() || text.GetLength() > 0)
	{
		dc.FillSolidRect(t, Settings().m_Theme.SearchTextBoxFocusBG());
		dc.FillSolidRect(b, Settings().m_Theme.SearchTextBoxFocusBG());

		c = Settings().m_Theme.SearchTextBoxFocusBorder();
	}
	else
	{
		dc.FillSolidRect(t, Settings().m_Theme.MainWindowBG());
		dc.FillSolidRect(b, Settings().m_Theme.MainWindowBG());
	}

	//if ((text.GetLength() > 0 || this == GetFocus()) && m_windowDpi)
	{
		CWindowDC borderDc(this);

		CRect rcFrame;
		this->GetWindowRect(rcFrame);
		this->ScreenToClient(rcFrame);

		CRect rcBorder(0, 0, rcFrame.Width(), rcFrame.Height());

		int border = m_windowDpi->Scale(1);
		CBrush borderBrush(c);

		for (int x = 0; x < border; x++)
		{
			borderDc.FrameRect(rcBorder, &borderBrush);
			rcBorder.DeflateRect(1, 1, 1, 1);
		}
	}

	//OutputDebugString(_T("OnNCPaint \r\n"));
}

void CSymbolEdit::OnTimer(UINT_PTR nIDEvent)
{
	switch (nIDEvent)
	{
	case 1:
		KillTimer(1);
		//Invalidate();
		::SetWindowPos(m_hWnd, NULL, 0, 0, 0, 0, SWP_DRAWFRAME | SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE);
		break;
	}

	CEdit::OnTimer(nIDEvent);
}

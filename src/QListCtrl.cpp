// QListCtrl.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "QListCtrl.h"
#include "ProcessPaste.h"
#include "BitmapHelper.h"
#include "ClipboardFormatError.h"
#include "ErrorReport.h"
#include "MainTableFunctions.h"
#include "DittoCopyBuffer.h"
#include "ClipDataReader.h"
#include <atlbase.h>
#include "..\Shared\TextConvert.h"
#include <cmath>
#include <vector>
#include <string>
#include <cwchar>   // For swscanf
#include <algorithm> // For std::round
#include <gdiplus.h>
#include <numbers>

/**
 * @brief CSS color helpers of the copied-color drawing: W3C color names, CSS numbers, OKLCH.
 */
class CCssColorParser
{
public:
	/**
	 * @brief Looks up a W3C color name.
	 * @param name the lower-case color name.
	 * @param color receives the color when the name is known.
	 * @return true when the name is a W3C color name.
	 */
	static bool FindColorName(const CString& name, COLORREF& color);

	/**
	 * @brief Parses a CSS numeric value, which can be a percentage or a number.
	 * @param token the value text; surrounding spaces and a trailing '%' are ignored.
	 * @param value receives the number (a percentage is not divided by 100).
	 * @return true when a number was read.
	 */
	static bool ParseCssValue(const CString& token, double& value);

	/**
	 * @brief Converts a color from OKLCH color space to sRGB, clamping out-of-gamut colors.
	 * @param l the lightness, 0-1.
	 * @param c the chroma, 0-0.4 (theoretically unbounded, but practically small).
	 * @param h the hue in degrees, 0-360.
	 * @return the sRGB color.
	 */
	static COLORREF OklchToRgb(double l, double c, double h);

private:
	/**
	 * @brief The W3C color names and their RGB values, built once on first use (thread-safe).
	 * @return the read-only map.
	 */
	static const std::map<CString, COLORREF>& ColorNameMap();

	/**
	 * @brief Builds the W3C color name map.
	 * @return the map of the lower-case names to their colors.
	 */
	static std::map<CString, COLORREF> BuildColorNameMap();
};

const std::map<CString, COLORREF>& CCssColorParser::ColorNameMap()
{
	static const std::map<CString, COLORREF> colorNameMap{ BuildColorNameMap() };
	return colorNameMap;
}

bool CCssColorParser::FindColorName(const CString& name, COLORREF& color)
{
	const std::map<CString, COLORREF>& colorNameMap{ ColorNameMap() };
	auto it = colorNameMap.find(name);
	if (it == colorNameMap.end())
		return false;

	color = it->second;
	return true;
}

std::map<CString, COLORREF> CCssColorParser::BuildColorNameMap()
{
	std::map<CString, COLORREF> names{};
	{
        // Populate the map with W3C named colors
        // A comprehensive list of 148 colors
        names[_T("black")] = RGB(0, 0, 0);
        names[_T("silver")] = RGB(192, 192, 192);
        names[_T("gray")] = RGB(128, 128, 128);
        names[_T("white")] = RGB(255, 255, 255);
        names[_T("maroon")] = RGB(128, 0, 0);
        names[_T("red")] = RGB(255, 0, 0);
        names[_T("purple")] = RGB(128, 0, 128);
        names[_T("fuchsia")] = RGB(255, 0, 255);
        names[_T("green")] = RGB(0, 128, 0);
        names[_T("lime")] = RGB(0, 255, 0);
        names[_T("olive")] = RGB(128, 128, 0);
        names[_T("yellow")] = RGB(255, 255, 0);
        names[_T("navy")] = RGB(0, 0, 128);
        names[_T("blue")] = RGB(0, 0, 255);
        names[_T("teal")] = RGB(0, 128, 128);
        names[_T("aqua")] = RGB(0, 255, 255);
        names[_T("aliceblue")] = RGB(240, 248, 255);
        names[_T("antiquewhite")] = RGB(250, 235, 215);
        names[_T("aquamarine")] = RGB(127, 255, 212);
        names[_T("azure")] = RGB(240, 255, 255);
        names[_T("beige")] = RGB(245, 245, 220);
        names[_T("bisque")] = RGB(255, 228, 196);
        names[_T("blanchedalmond")] = RGB(255, 235, 205);
        names[_T("blueviolet")] = RGB(138, 43, 226);
        names[_T("brown")] = RGB(165, 42, 42);
        names[_T("burlywood")] = RGB(222, 184, 135);
        names[_T("cadetblue")] = RGB(95, 158, 160);
        names[_T("chartreuse")] = RGB(127, 255, 0);
        names[_T("chocolate")] = RGB(210, 105, 30);
        names[_T("coral")] = RGB(255, 127, 80);
        names[_T("cornflowerblue")] = RGB(100, 149, 237);
        names[_T("cornsilk")] = RGB(255, 248, 220);
        names[_T("crimson")] = RGB(220, 20, 60);
        names[_T("cyan")] = RGB(0, 255, 255);
        names[_T("darkblue")] = RGB(0, 0, 139);
        names[_T("darkcyan")] = RGB(0, 139, 139);
        names[_T("darkgoldenrod")] = RGB(184, 134, 11);
        names[_T("darkgray")] = RGB(169, 169, 169);
        names[_T("darkgreen")] = RGB(0, 100, 0);
        names[_T("darkkhaki")] = RGB(189, 183, 107);
        names[_T("darkmagenta")] = RGB(139, 0, 139);
        names[_T("darkolivegreen")] = RGB(85, 107, 47);
        names[_T("darkorange")] = RGB(255, 140, 0);
        names[_T("darkorchid")] = RGB(153, 50, 204);
        names[_T("darkred")] = RGB(139, 0, 0);
        names[_T("darksalmon")] = RGB(233, 150, 122);
        names[_T("darkseagreen")] = RGB(143, 188, 143);
        names[_T("darkslateblue")] = RGB(72, 61, 139);
        names[_T("darkslategray")] = RGB(47, 79, 79);
        names[_T("darkturquoise")] = RGB(0, 206, 209);
        names[_T("darkviolet")] = RGB(148, 0, 211);
        names[_T("deeppink")] = RGB(255, 20, 147);
        names[_T("deepskyblue")] = RGB(0, 191, 255);
        names[_T("dimgray")] = RGB(105, 105, 105);
        names[_T("dodgerblue")] = RGB(30, 144, 255);
        names[_T("firebrick")] = RGB(178, 34, 34);
        names[_T("floralwhite")] = RGB(255, 250, 240);
        names[_T("forestgreen")] = RGB(34, 139, 34);
        names[_T("gainsboro")] = RGB(220, 220, 220);
        names[_T("ghostwhite")] = RGB(248, 248, 255);
        names[_T("gold")] = RGB(255, 215, 0);
        names[_T("goldenrod")] = RGB(218, 165, 32);
        names[_T("greenyellow")] = RGB(173, 255, 47);
        names[_T("honeydew")] = RGB(240, 255, 240);
        names[_T("hotpink")] = RGB(255, 105, 180);
        names[_T("indianred")] = RGB(205, 92, 92);
        names[_T("indigo")] = RGB(75, 0, 130);
        names[_T("ivory")] = RGB(255, 255, 240);
        names[_T("khaki")] = RGB(240, 230, 140);
        names[_T("lavender")] = RGB(230, 230, 250);
        names[_T("lavenderblush")] = RGB(255, 240, 245);
        names[_T("lawngreen")] = RGB(124, 252, 0);
        names[_T("lemonchiffon")] = RGB(255, 250, 205);
        names[_T("lightblue")] = RGB(173, 216, 230);
        names[_T("lightcoral")] = RGB(240, 128, 128);
        names[_T("lightcyan")] = RGB(224, 255, 255);
        names[_T("lightgoldenrodyellow")] = RGB(250, 250, 210);
        names[_T("lightgray")] = RGB(211, 211, 211);
        names[_T("lightgreen")] = RGB(144, 238, 144);
        names[_T("lightpink")] = RGB(255, 182, 193);
        names[_T("lightsalmon")] = RGB(255, 160, 122);
        names[_T("lightseagreen")] = RGB(32, 178, 170);
        names[_T("lightskyblue")] = RGB(135, 206, 250);
        names[_T("lightslategray")] = RGB(119, 136, 153);
        names[_T("lightsteelblue")] = RGB(176, 196, 222);
        names[_T("lightyellow")] = RGB(255, 255, 224);
        names[_T("limegreen")] = RGB(50, 205, 50);
        names[_T("linen")] = RGB(250, 240, 230);
        names[_T("magenta")] = RGB(255, 0, 255);
        names[_T("mediumaquamarine")] = RGB(102, 205, 170);
        names[_T("mediumblue")] = RGB(0, 0, 205);
        names[_T("mediumorchid")] = RGB(186, 85, 211);
        names[_T("mediumpurple")] = RGB(147, 112, 219);
        names[_T("mediumseagreen")] = RGB(60, 179, 113);
        names[_T("mediumslateblue")] = RGB(123, 104, 238);
        names[_T("mediumspringgreen")] = RGB(0, 250, 154);
        names[_T("mediumturquoise")] = RGB(72, 209, 204);
        names[_T("mediumvioletred")] = RGB(199, 21, 133);
        names[_T("midnightblue")] = RGB(25, 25, 112);
        names[_T("mintcream")] = RGB(245, 255, 250);
        names[_T("mistyrose")] = RGB(255, 228, 225);
        names[_T("moccasin")] = RGB(255, 228, 181);
        names[_T("navajowhite")] = RGB(255, 222, 173);
        names[_T("oldlace")] = RGB(253, 245, 230);
        names[_T("olivedrab")] = RGB(107, 142, 35);
        names[_T("orange")] = RGB(255, 165, 0);
        names[_T("orangered")] = RGB(255, 69, 0);
        names[_T("orchid")] = RGB(218, 112, 214);
        names[_T("palegoldenrod")] = RGB(238, 232, 170);
        names[_T("palegreen")] = RGB(152, 251, 152);
        names[_T("paleturquoise")] = RGB(175, 238, 238);
        names[_T("palevioletred")] = RGB(219, 112, 147);
        names[_T("papayawhip")] = RGB(255, 239, 213);
        names[_T("peachpuff")] = RGB(255, 218, 185);
        names[_T("peru")] = RGB(205, 133, 63);
        names[_T("pink")] = RGB(255, 192, 203);
        names[_T("plum")] = RGB(221, 160, 221);
        names[_T("powderblue")] = RGB(176, 224, 230);
        names[_T("rebeccapurple")] = RGB(102, 51, 153);
        names[_T("rosybrown")] = RGB(188, 143, 143);
        names[_T("royalblue")] = RGB(65, 105, 225);
        names[_T("saddlebrown")] = RGB(139, 69, 19);
        names[_T("salmon")] = RGB(250, 128, 114);
        names[_T("sandybrown")] = RGB(244, 164, 96);
        names[_T("seagreen")] = RGB(46, 139, 87);
        names[_T("seashell")] = RGB(255, 245, 238);
        names[_T("sienna")] = RGB(160, 82, 45);
        names[_T("skyblue")] = RGB(135, 206, 235);
        names[_T("slateblue")] = RGB(106, 90, 205);
        names[_T("slategray")] = RGB(112, 128, 144);
        names[_T("snow")] = RGB(255, 250, 250);
        names[_T("springgreen")] = RGB(0, 255, 127);
        names[_T("steelblue")] = RGB(70, 130, 180);
        names[_T("tan")] = RGB(210, 180, 140);
        names[_T("thistle")] = RGB(216, 191, 216);
        names[_T("tomato")] = RGB(255, 99, 71);
        names[_T("turquoise")] = RGB(64, 224, 208);
        names[_T("violet")] = RGB(238, 130, 238);
        names[_T("wheat")] = RGB(245, 222, 179);
        names[_T("whitesmoke")] = RGB(245, 245, 245);
        names[_T("yellowgreen")] = RGB(154, 205, 50);
	}
	return names;
}

/////////////////////////////////////////////////////////////////////////////
// CQListCtrl

CQListCtrl::CQListCtrl()
{
	m_linesPerRow = 1;
	m_windowDpi = NULL;
	m_pToolTip = NULL;
	m_allSelected = false;
	m_rowHeight = 50;
	m_mouseOverScrollAreaStart = 0;
	m_showIfClipWasPasted = TRUE;
	m_bShowTextForFirstTenHotKeys = true;
	m_pToolTipActions = NULL;
}

CQListCtrl::~CQListCtrl()
{
	m_Font.DeleteObject();

	m_boldFont.DeleteObject();
}

CGetSetOptions& CQListCtrl::Settings() const
{
	return theApp.Services().Settings();
}

// returns the position 1-10 if the index is in the FirstTen block else -1
int CQListCtrl::GetFirstTenNum(int index)
{
	// set firstTenNum to the first ten number (1-10) corresponding to the given index
	int firstTenNum = -1; // -1 means that nItem is not in the FirstTen block.

	if (0 <= index && index <= 9)
	{
		firstTenNum = index + Settings().m_firstTenHotKeysStart;
		firstTenNum = firstTenNum % 10;
	}

	return firstTenNum;
}

BEGIN_MESSAGE_MAP(CQListCtrl, CListCtrl)
	//{{AFX_MSG_MAP(CQListCtrl)
	ON_NOTIFY_REFLECT(LVN_KEYDOWN, OnKeydown)
	ON_NOTIFY_REFLECT(NM_CUSTOMDRAW, OnCustomdrawList)
	ON_WM_MOUSEMOVE()
	ON_WM_SYSKEYDOWN()
	ON_WM_ERASEBKGND()
	ON_WM_CREATE()
	ON_WM_HSCROLL()
	ON_WM_TIMER()
	ON_NOTIFY_REFLECT(LVN_ITEMCHANGED, OnSelectionChange)
	ON_WM_VSCROLL()
	ON_WM_WINDOWPOSCHANGED()
	//}}AFX_MSG_MAP
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTW, 0, 0xFFFF, OnToolTipText)
	ON_NOTIFY_EX_RANGE(TTN_NEEDTEXTA, 0, 0xFFFF, OnToolTipText)
	ON_WM_KILLFOCUS()
	ON_WM_MEASUREITEM_REFLECT()
	ON_WM_MOUSEHWHEEL()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQListCtrl message handlers

void CQListCtrl::OnKeydown(NMHDR* /*pNMHDR*/, LRESULT* pResult)
{
	*pResult = 0;
}

DROPEFFECT CQListCtrl::OnDragOver(COleDataObject* /*pDataObject*/, DWORD /*dwKeyState*/, CPoint /*point*/)
{
	return DROPEFFECT_COPY;
}

void CQListCtrl::GetSelectionIndexes(ARRAY& arr)
{
	arr.RemoveAll();

	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		arr.Add(GetNextSelectedItem(pos));
	}
}

bool CQListCtrl::PutSelectedItemOnDittoCopyBuffer(long lBuffer)
{
	bool bRet = false;
	ARRAY arr;
	GetSelectionItemData(arr);
	INT_PTR nCount = arr.GetSize();
	if (nCount > 0 && arr[0])
	{
		// false (the error is shown) when the clip was not put on the buffer
		bRet = CDittoCopyBuffer::PutClipOnDittoCopyBuffer(Settings(), theApp.Services().Database(), arr[0], lBuffer);
	}

	return bRet;
}

void CQListCtrl::GetSelectionItemData(ARRAY& arr)
{
	DWORD dwData;
	int i;
	arr.RemoveAll();
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		i = GetNextSelectedItem(pos);
		dwData = GetItemData(i);
		arr.Add(dwData);
	}
}

void CQListCtrl::RemoveAllSelection()
{
	POSITION pos = GetFirstSelectedItemPosition();
	while (pos)
	{
		SetSelection(GetNextSelectedItem(pos), FALSE);
	}
}

BOOL CQListCtrl::SetSelection(int nRow, BOOL bSelect)
{
	if (bSelect)
		return SetItemState(nRow, LVIS_SELECTED, LVIS_SELECTED);
	else
		return SetItemState(nRow, ~static_cast<UINT>(LVIS_SELECTED), LVIS_SELECTED);
}

BOOL CQListCtrl::SetText(int nRow, int nCol, CString cs)
{
	return SetItemText(nRow, nCol, cs);
}

BOOL CQListCtrl::SetCaret(int nRow, BOOL bFocus)
{
	if (bFocus)
		return SetItemState(nRow, LVIS_FOCUSED, LVIS_FOCUSED);
	else
		return SetItemState(nRow, ~static_cast<UINT>(LVIS_FOCUSED), LVIS_FOCUSED);
}

long CQListCtrl::GetCaret()
{
	return GetNextItem(-1, LVNI_FOCUSED);
}

// moves the caret to the given index, selects it, and ensures it is visible.
BOOL CQListCtrl::SetListPos(int index)
{
	if (index < 0 || index >= GetItemCount())
		return FALSE;

	RemoveAllSelection();
	SetCaret(index);
	SetSelection(index);
	ListView_SetSelectionMark(m_hWnd, index);
	EnsureVisible(index, FALSE);

	return TRUE;
}

BOOL CQListCtrl::SetFormattedText(int nRow, int nCol, LPCTSTR lpszFormat, ...)
{
	CString csText;
	va_list vlist;

	ASSERT(AfxIsValidString(lpszFormat));
	va_start(vlist, lpszFormat);
	csText.FormatV(lpszFormat, vlist);
	va_end(vlist);

	return SetText(nRow, nCol, csText);
}

void CQListCtrl::SetNumberOfLinesPerRow(int nLines, bool force)
{
	if (m_linesPerRow != nLines ||
		force)
	{
		m_linesPerRow = nLines;

		CRect rc;
		GetWindowRect(&rc);
		WINDOWPOS wp;
		wp.hwnd = m_hWnd;
		wp.cx = rc.Width();
		wp.cy = rc.Height();
		wp.flags = SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER;
		SendMessage(WM_WINDOWPOSCHANGED, 0, (LPARAM)&wp);
	}
}

void CQListCtrl::MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct)
{
	TEXTMETRIC tm;
	HDC hDC = ::GetDC(NULL);
	CFont* pFont = GetFont();
	HFONT hFontOld = (HFONT)SelectObject(hDC, pFont->GetSafeHandle());
	GetTextMetrics(hDC, &tm);
	if (m_windowDpi != NULL)
	{
		lpMeasureItemStruct->itemHeight = ((tm.tmHeight + tm.tmExternalLeading) * m_linesPerRow) + m_windowDpi->Scale(s_rowBottomBorder);
		m_rowHeight = lpMeasureItemStruct->itemHeight;
	}
	SelectObject(hDC, hFontOld);
	::ReleaseDC(NULL, hDC);
}

void CQListCtrl::OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult)
{
	NMLVCUSTOMDRAW* pLVCD = reinterpret_cast<NMLVCUSTOMDRAW*>(pNMHDR);

	*pResult = 0;

	// Request item-specific notifications if this is the
	// beginning of the paint cycle.
	if (CDDS_PREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		*pResult = CDRF_NOTIFYITEMDRAW;
	}
	else if (CDDS_ITEMPREPAINT == pLVCD->nmcd.dwDrawStage)
	{
		DrawListItem(pLVCD);

		*pResult = CDRF_SKIPDEFAULT;    // We've painted everything.
	}
}

void CQListCtrl::DrawListItem(NMLVCUSTOMDRAW* pLVCD)
{
	LVITEM   rItem;
	int      nItem = static_cast<int>(pLVCD->nmcd.dwItemSpec);
	CDC* pDC = CDC::FromHandle(pLVCD->nmcd.hdc);
	BOOL     bListHasFocus;
	CRect    rcItem;

	bListHasFocus = (GetSafeHwnd() == ::GetFocus());

	// Get the image index and selected/focused state of the
	// item being drawn.
	ZeroMemory(&rItem, sizeof(LVITEM));
	rItem.mask = LVIF_STATE;
	rItem.iItem = nItem;
	rItem.stateMask = LVIS_SELECTED | LVIS_FOCUSED;
	GetItem(&rItem);

	// Get the rect that bounds the text label.
	GetItemRect(nItem, rcItem, LVIR_SELECTBOUNDS);

	COLORREF OldColor = CLR_INVALID;
	int nOldBKMode = -1;

	CString csText;
	CString strSymbols;
	ReadItemText(nItem, csText, strSymbols);

	// Draw the background of the list item.  Colors are selected
	// according to the item's state.
	const RowColors colors{ChooseRowColors(nItem, rItem.state, bListHasFocus)};
	OldColor = pDC->SetTextColor(colors.text);

	pDC->FillSolidRect(rcItem, colors.background);
	nOldBKMode = pDC->SetBkMode(TRANSPARENT);

	CRect rcText = rcItem;
	rcText.left += m_windowDpi->Scale(s_rowLeftBorder);
	rcText.top += m_windowDpi->Scale(1);
	rcText.bottom -= m_windowDpi->Scale(1);

	if (IsPastedClip(strSymbols)) //clip was pasted from ditto
	{
		DrawPastedMarker(pDC, rcItem);
	}

	// set firstTenNum to the first ten number (1-10) corresponding to
	//  the current nItem.
	// -1 means that nItem is not in the FirstTen block.
	int firstTenNum = GetFirstTenNum(nItem);

	if (ShowsFirstTenHotKey(firstTenNum))
	{
		rcText.left += m_windowDpi->Scale(12);
	}
	else
	{
		rcText.left += m_windowDpi->Scale(3);
	}

	// if we are inside a group, don't display the "in group" flag
	bool drawInGroupIcon = ShouldDrawInGroupIcon(strSymbols);

	DrawCopiedColorCode(csText, rcText, pDC);

	DrawBitMap(nItem, rcText, pDC, csText);

	// draw the symbol box
	DrawSymbolIcons(pDC, rcText, strSymbols, drawInGroupIcon);

	DrawItemText(nItem, csText, rcText, pDC);

	// Draw a focus rect around the item if necessary.
	//if(bListHasFocus && (rItem.state & LVIS_FOCUSED))
	//	pDC->DrawFocusRect(rcItem);

	if (ShowsFirstTenHotKey(firstTenNum))
	{
		DrawFirstTenHotKey(pDC, rcItem, firstTenNum);
	}

	// restore the previous values
	if (OldColor != CLR_INVALID)
		pDC->SetTextColor(OldColor);

	if (nOldBKMode > -1)
		pDC->SetBkMode(nOldBKMode);
}

void CQListCtrl::ReadItemText(int nItem, CString& csText, CString& strSymbols)
{
	LPTSTR lpszText = csText.GetBufferSetLength(Settings().m_bDescTextSize);
	GetItemText(nItem, 0, lpszText, Settings().m_bDescTextSize);
	csText.ReleaseBuffer();

	// extract symbols
	int nSymEnd = csText.Find('|');
	if (nSymEnd >= 0)
	{
		strSymbols = csText.Left(nSymEnd);
		csText = csText.Mid(nSymEnd + 1);
	}
}

CQListCtrl::RowColors CQListCtrl::ChooseRowColors(int nItem, UINT state, BOOL bListHasFocus)
{
	if (state & LVIS_SELECTED)
	{
		if (bListHasFocus)
		{
			return RowColors{Settings().m_Theme.ListBoxSelectedBG(), Settings().m_Theme.ListBoxSelectedText()};
		}
		return RowColors{Settings().m_Theme.ListBoxSelectedNoFocusBG(), Settings().m_Theme.ListBoxSelectedNoFocusText()};
	}

	//Shade alternating Rows
	if ((nItem % 2) == 0)
	{
		return RowColors{Settings().m_Theme.ListBoxOddRowsBG(), Settings().m_Theme.ListBoxOddRowsText()};
	}
	return RowColors{Settings().m_Theme.ListBoxEvenRowsBG(), Settings().m_Theme.ListBoxEvenRowsText()};
}

bool CQListCtrl::IsPastedClip(const CString& strSymbols) const
{
	return m_showIfClipWasPasted &&
		strSymbols.GetLength() > 0 &&
		strSymbols.Find(_T("<pasted>")) >= 0;
}

void CQListCtrl::DrawPastedMarker(CDC* pDC, const CRect& rcItem)
{
	CRect pastedRect(rcItem);
	pastedRect.left++;
	pastedRect.right = pastedRect.left + m_windowDpi->Scale(2);

	pDC->FillSolidRect(pastedRect, Settings().m_Theme.ClipPastedColor());
}

bool CQListCtrl::ShowsFirstTenHotKey(int firstTenNum) const
{
	return m_bShowTextForFirstTenHotKeys && firstTenNum >= 0;
}

bool CQListCtrl::ShouldDrawInGroupIcon(const CString& strSymbols)
{
	return (theApp.Services().State().m_GroupID > 0 &&strSymbols.Find(_T("<ingroup>")) >= 0) == false;
}

void CQListCtrl::DrawSymbolIcons(CDC* pDC, CRect& rcText, const CString& strSymbols, bool drawInGroupIcon)
{
	if (strSymbols.GetLength() <= 0)
		return;

	if (strSymbols.Find(_T("<group>")) >= 0) //group
		DrawSymbolIcon(pDC, rcText, m_groupFolder);
	if (strSymbols.Find(_T("<noautodelete>")) >= 0) //don't auto delete
		DrawSymbolIcon(pDC, rcText, m_dontDeleteImage);
	if (strSymbols.Find(_T("<shortcut>")) >= 0) // has shortcut
		DrawSymbolIcon(pDC, rcText, m_shortCutImage);
	if (drawInGroupIcon &&
		strSymbols.Find(_T("<ingroup>")) >= 0) // in group
		DrawSymbolIcon(pDC, rcText, m_inFolderImage);
	// <qpastetext> (has quick paste text) has no icon
	if (strSymbols.Find(_T("<sticky>")) >= 0) //sticky clip
		DrawSymbolIcon(pDC, rcText, m_stickyImage);
}

void CQListCtrl::DrawSymbolIcon(CDC* pDC, CRect& rcText, CGdiImageDrawer& image)
{
	image.Draw(pDC, *m_windowDpi, this, rcText.left, rcText.top, false, false);
	rcText.left += image.ImageWidth() + m_windowDpi->Scale(2);
}

void CQListCtrl::DrawItemText(int nItem, CString& csText, CRect& rcText, CDC* pDC)
{
	if (DrawRtfText(nItem, rcText, pDC) != FALSE)
		return;

	if (HighlightSearchMatches(csText))
	{
		m_htmlTextDrawer.Draw(pDC->m_hDC, csText, csText.GetLength(), rcText, DT_VCENTER | DT_EXPANDTABS | DT_NOPREFIX);
	}
	else
	{
		pDC->DrawText(csText, rcText, DT_VCENTER | DT_EXPANDTABS | DT_NOPREFIX);
	}
}

bool CQListCtrl::HighlightSearchMatches(CString& csText)
{
	auto highlightColor = Settings().m_Theme.SearchTextHighlight();
	//use unprintable characters so it doesn't find copied html to convert
	return m_searchText.GetLength() > 0 &&
		CMarkerInserter::Insert(theApp.Services().IcuString(), csText, m_searchText,CStringUtil::Format(_T("\x01\x04 color='#%02x%02x%02x'\x02"), GetRValue(highlightColor), GetGValue(highlightColor), GetBValue(highlightColor)), _T("\x01\x03\x04\x02"), m_linesPerRow) > 0;
}

void CQListCtrl::DrawFirstTenHotKey(CDC* pDC, const CRect& rcItem, int firstTenNum)
{
	CString cs;
	if (firstTenNum == 10)
		cs = "0";
	else
		cs.Format(_T("%d"), firstTenNum);

	CRect crHotKey = rcItem;

	int extraFromClipWasPaste = 0;
	if (m_showIfClipWasPasted)
		extraFromClipWasPaste = 3;

	crHotKey.right = crHotKey.left + m_windowDpi->Scale(11);
	crHotKey.left += m_windowDpi->Scale(1 + extraFromClipWasPaste);
	crHotKey.top += m_windowDpi->Scale(1 + extraFromClipWasPaste);

	CFont* pOldFont{ pDC->SelectObject(&m_SmallFont) };
	COLORREF localOldTextColor = pDC->SetTextColor(Settings().m_Theme.ListSmallQuickPasteIndexColor());

	CPen pen(PS_SOLID, 0, Settings().m_Theme.ListSmallQuickPasteIndexColor());
	CPen* pOldPen = pDC->SelectObject(&pen);

	pDC->DrawText(cs, crHotKey, DT_BOTTOM);

	pDC->MoveTo(CPoint(rcItem.left + m_windowDpi->Scale(8 + extraFromClipWasPaste), rcItem.top));
	pDC->LineTo(CPoint(rcItem.left + m_windowDpi->Scale(8 + extraFromClipWasPaste), rcItem.bottom));

	pDC->SelectObject(pOldFont);
	pDC->SetTextColor(localOldTextColor);
	pDC->SelectObject(pOldPen);
}


// Helper function implementation to check for valid hex characters
bool CQListCtrl::IsHexString(const CString& str)
{
	if (str.IsEmpty()) {
		return false;
	}
	for (int i = 0; i < str.GetLength(); ++i) {
		if (!iswxdigit(str[i])) {
			return false;
		}
	}
	return true;
}

// Helper function implementation for HSL to RGB conversion
COLORREF CQListCtrl::HslToRgb(double h, double s, double l)
{
	// Input h(0-360), s(0-1), l(0-1)
	// Output COLORREF (RGB)

	double r, g, b;

	if (s == 0)
	{
		r = g = b = l; // Achromatic (gray)
	}
	else
	{
		auto hue2rgb = [&](double p, double q, double t)
			{
				if (t < 0) t += 1;
				if (t > 1) t -= 1;
				if (t < 1.0 / 6.0) return p + (q - p) * 6 * t;
				if (t < 1.0 / 2.0) return q;
				if (t < 2.0 / 3.0) return p + (q - p) * (2.0 / 3.0 - t) * 6;
				return p;
			};

		// Normalize h to 0-1 range
		double h_norm = h / 360.0;

		double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		double p = 2 * l - q;
		r = hue2rgb(p, q, h_norm + 1.0 / 3.0);
		g = hue2rgb(p, q, h_norm);
		b = hue2rgb(p, q, h_norm - 1.0 / 3.0);
	}

	// Scale RGB values to 0-255 and round
	int R = static_cast<int>(std::round(r * 255));
	int G = static_cast<int>(std::round(g * 255));
	int B = static_cast<int>(std::round(b * 255));

	// Clamp values to 0-255 just in case of floating point inaccuracies
	R = max(0, min(255, R));
	G = max(0, min(255, G));
	B = max(0, min(255, B));

	return RGB(R, G, B);
}


// Helper function to parse a CSS numeric value which can be a percentage or a number
bool CCssColorParser::ParseCssValue(const CString& token, double& value)
{
	CString localToken = token;
	localToken.Trim();
	if (localToken.IsEmpty()) return false;

	bool isPercent = (localToken.Right(1) == _T('%'));
	if (isPercent)
	{
		localToken.TrimRight(_T('%'));
	}

	if (_stscanf(localToken, _T("%lf"), &value) == 1)
	{
		return true;
	}
	return false;
}


// Converts a color from OKLCH color space to sRGB.
// l: 0-1, c: 0-0.4 (theoretically unbounded, but practically small), h: 0-360
COLORREF CCssColorParser::OklchToRgb(double l, double c, double h)
{
	// 1. Convert OKLCH to OKLAB
	double h_rad = h * std::numbers::pi / 180.0;
	double a = c * cos(h_rad);
	double b = c * sin(h_rad);

	// 2. Convert OKLAB to XYZ
	// First, convert to an intermediate non-linear LMS-like space
	double l_ = l + 0.3963377774 * a + 0.2158037573 * b;
	double m_ = l - 0.1055613458 * a - 0.0638541728 * b;
	double s_ = l - 0.0894841775 * a - 1.2914855480 * b;

	// Then, convert to linear LMS by undoing the cube-root non-linearity
	double l_linear = l_ * l_ * l_;
	double m_linear = m_ * m_ * m_;
	double s_linear = s_ * s_ * s_;

	// Finally, convert linear LMS to XYZ color space
	double x = +1.2270138511 * l_linear - 0.5577999807 * m_linear + 0.2812561490 * s_linear;
	double y = -0.0405801784 * l_linear + 1.1122568696 * m_linear - 0.0716766787 * s_linear;
	double z = -0.0763812845 * l_linear - 0.4214819784 * m_linear + 1.5861632204 * s_linear;

	// 3. Convert XYZ to linear sRGB
	double r_linear = +3.2404542 * x - 1.5371385 * y - 0.4985314 * z;
	double g_linear = -0.9692660 * x + 1.8760108 * y + 0.0415560 * z;
	double b_linear = +0.0556434 * x - 0.2040259 * y + 1.0572252 * z;

	// 4. Convert linear sRGB to gamma-corrected sRGB
	auto ToSrgb = [](double val) {
		if (val <= 0.0031308) {
			return 12.92 * val;
		}
		return 1.055 * pow(val, 1.0 / 2.4) - 0.055;
	};

	double r_srgb = ToSrgb(r_linear);
	double g_srgb = ToSrgb(g_linear);
	double b_srgb = ToSrgb(b_linear);

	// 5. Scale to 0-255 and clamp (for out-of-gamut colors)
	int R = static_cast<int>(std::round(r_srgb * 255.0));
	int G = static_cast<int>(std::round(g_srgb * 255.0));
	int B = static_cast<int>(std::round(b_srgb * 255.0));

	R = max(0, min(255, R));
	G = max(0, min(255, G));
	B = max(0, min(255, B));

	return RGB(R, G, B);
}


void CQListCtrl::DrawCheckerboard(CDC* pDC, CRect rect)
{
	COLORREF color1 = RGB(255, 255, 255); // White
	COLORREF color2 = RGB(204, 204, 204); // Light grey
	int squareSize = m_windowDpi->Scale(4);
	if (squareSize <= 0)
	{
		squareSize = 4;
	}

	for (int y = rect.top; y < rect.bottom; y += squareSize)
	{
		for (int x = rect.left; x < rect.right; x += squareSize)
		{
			COLORREF color = ((( (x - rect.left) / squareSize) + ((y - rect.top) / squareSize)) % 2 == 0) ? color1 : color2;
			CRect square(x, y, min(x + squareSize, rect.right), min(y + squareSize, rect.bottom));
			pDC->FillSolidRect(square, color);
		}
	}
}


void CQListCtrl::DrawCopiedColorCode(CString& csText, CRect& rcText, CDC* pDC)
{
	if (Settings().m_bDrawCopiedColorCode == FALSE || csText.IsEmpty())
		return;

	// 1. Initial Cleaning and Prep
	CString cleanedText = csText;
	cleanedText.Trim(_T("»"));
	cleanedText.Trim();
	cleanedText.TrimRight(_T(";"));
	cleanedText.Trim();

	if (cleanedText.IsEmpty())
		return;

	CString originalCleanedText = cleanedText;
	CString parseText = cleanedText;
	parseText.MakeLower();

	// 2. Find the colour, 3. draw the colour box
	CopiedColor color{};
	if (ParseCopiedColor(parseText, color) == false)
		return;

	DrawColorBox(pDC, rcText, color);
	csText = originalCleanedText;
}

bool CQListCtrl::ParseCopiedColor(const CString& parseText, CopiedColor& color)
{
	// Check for formats with unique signatures first: W3C named colors, hex colors with # or 0x
	// prefix, the W3C notations rgb(...), hsl(...) and oklch(...); then the non-W3C formats
	constexpr std::array<ColorParser, 6> parsers{
		&CQListCtrl::ParseNamedColor,
		&CQListCtrl::ParsePrefixedHexColor,
		&CQListCtrl::ParseCssRgbColor,
		&CQListCtrl::ParseCssHslColor,
		&CQListCtrl::ParseCssOklchColor,
		&CQListCtrl::ParsePlainRgbColor,
	};
	for (const ColorParser parser : parsers)
	{
		if ((this->*parser)(parseText, color))
			return true;
	}
	return false;
}

bool CQListCtrl::ParseNamedColor(const CString& parseText, CopiedColor& color)
{
	// Check for W3C Named Colors
	COLORREF namedColor{};
	if (CCssColorParser::FindColorName(parseText, namedColor) == false)
		return false;

	color = CopiedColor{namedColor, 255};
	return true;
}

bool CQListCtrl::ParsePrefixedHexColor(const CString& parseText, CopiedColor& color)
{
	// Check for Hex Colors with # or 0x prefix
	if ((parseText.Left(1) == _T('#') || parseText.Left(2) == _T("0x")) == false)
		return false;

	CString hexString = ExpandShorthandHex(StripHexPrefix(parseText));

	if ((IsHexString(hexString) && (hexString.GetLength() == 6 || hexString.GetLength() == 8)) == false)
		return false;

	return ScanHexColor(hexString, color);
}

CString CQListCtrl::StripHexPrefix(const CString& parseText)
{
	CString hexString = parseText;
	if (hexString.Left(1) == _T('#')) hexString.Delete(0, 1);
	else if (hexString.Left(2) == _T("0x")) hexString.Delete(0, 2);
	return hexString;
}

CString CQListCtrl::ExpandShorthandHex(const CString& hexString)
{
	int len = hexString.GetLength();
	if (len != 3 && len != 4)
		return hexString;

	// Expand shorthand
	CString expanded;
	for (int i = 0; i < len; ++i) { expanded += hexString[i]; expanded += hexString[i]; }
	return expanded;
}

bool CQListCtrl::ScanHexColor(const CString& hexString, CopiedColor& color)
{
	unsigned int r = 0, g = 0, b = 0, a = 255;
	if (hexString.GetLength() == 8)
	{
		if (swscanf(hexString, _T("%2x%2x%2x%2x"), &r, &g, &b, &a) != 4)
			return false;

		color = CopiedColor{RGB(r, g, b), static_cast<int>(a)};
		return true;
	}

	// length is 6
	if (swscanf(hexString, _T("%2x%2x%2x"), &r, &g, &b) != 3)
		return false;

	color = CopiedColor{RGB(r, g, b), 255}; // default alpha
	return true;
}

bool CQListCtrl::IsCssFunction(const CString& parseText, const CString& name)
{
	return parseText.Right(1) == _T(")") && parseText.Left(name.GetLength()) == name;
}

std::vector<CString> CQListCtrl::SplitCssArguments(CString content)
{
	content.Replace(_T(','), _T(' '));
	content.Replace(_T('/'), _T(' '));

	std::vector<CString> tokens;
	int curPos = 0;
	CString token;
	while (!(token = content.Tokenize(_T(" "), curPos)).IsEmpty())
	{
		tokens.push_back(token);
	}
	return tokens;
}

bool CQListCtrl::ParseCssValues(const std::vector<CString>& tokens, CssValues& values)
{
	return tokens.size() >= 3 &&
		CCssColorParser::ParseCssValue(tokens[0], values.first) &&
		CCssColorParser::ParseCssValue(tokens[1], values.second) &&
		CCssColorParser::ParseCssValue(tokens[2], values.third);
}

int CQListCtrl::CssAlpha(const std::vector<CString>& tokens)
{
	int alpha = 255;
	double a_val = 1.0;
	if (tokens.size() >= 4 && CCssColorParser::ParseCssValue(tokens[3], a_val))
	{
		if (tokens[3].Find('%') != -1)
		{
			a_val /= 100.0;
		}
		a_val = max(0.0, min(1.0, a_val));
		alpha = static_cast<int>(std::round(a_val * 255.0));
	}
	return alpha;
}

bool CQListCtrl::IsByteValue(double value)
{
	return value >= 0 && value <= 255;
}

bool CQListCtrl::IsPercentValue(double value)
{
	return value >= 0 && value <= 100;
}

bool CQListCtrl::IsRgbByte(int value)
{
	return value >= 0 && value <= 255;
}

bool CQListCtrl::ParseCssRgbColor(const CString& parseText, CopiedColor& color)
{
	if (IsCssFunction(parseText, _T("rgb")) == false)
		return false;

	int prefixLen = (parseText.Left(4) == _T("rgba")) ? 5 : 4;
	const std::vector<CString> tokens = SplitCssArguments(parseText.Mid(prefixLen, parseText.GetLength() - prefixLen - 1));

	CssValues rgb{};
	if (ParseCssValues(tokens, rgb) == false)
		return false;

	if (tokens[0].Find('%') != -1)
	{
		rgb.first = std::round(rgb.first * 2.55);
		rgb.second = std::round(rgb.second * 2.55);
		rgb.third = std::round(rgb.third * 2.55);
	}

	int alpha = CssAlpha(tokens);

	if ((IsByteValue(rgb.first) && IsByteValue(rgb.second) && IsByteValue(rgb.third)) == false)
		return false;

	color = CopiedColor{RGB(static_cast<int>(rgb.first), static_cast<int>(rgb.second), static_cast<int>(rgb.third)), alpha};
	return true;
}

bool CQListCtrl::ParseCssHslColor(const CString& parseText, CopiedColor& color)
{
	if (IsCssFunction(parseText, _T("hsl")) == false)
		return false;

	int prefixLen = (parseText.Left(4) == _T("hsla")) ? 5 : 4;
	CString content = parseText.Mid(prefixLen, parseText.GetLength() - prefixLen - 1);
	// removing "deg" before or after the separators gives the same text: neither touches the other
	content.Replace(_T("deg"), _T(""));
	const std::vector<CString> tokens = SplitCssArguments(content);

	CssValues hsl{};
	if (ParseCssValues(tokens, hsl) == false)
		return false;

	int alpha = CssAlpha(tokens);

	if ((IsPercentValue(hsl.second) && IsPercentValue(hsl.third)) == false)
		return false;

	double h_val = fmod(hsl.first, 360.0);
	if (h_val < 0) h_val += 360.0;
	color = CopiedColor{HslToRgb(h_val, hsl.second / 100.0, hsl.third / 100.0), alpha};
	return true;
}

bool CQListCtrl::ParseCssOklchColor(const CString& parseText, CopiedColor& color)
{
	if (IsCssFunction(parseText, _T("oklch")) == false)
		return false;

	const std::vector<CString> tokens = SplitCssArguments(parseText.Mid(6, parseText.GetLength() - 7));

	CssValues lch{};
	if (ParseCssValues(tokens, lch) == false)
		return false;

	bool l_is_percent = tokens[0].Find('%') != -1;

	double l_normalized = l_is_percent ? lch.first / 100.0 : lch.first;

	int alpha = CssAlpha(tokens);

	if ((l_normalized >= 0 && l_normalized <= 1.0 && lch.second >= 0) == false)
		return false;

	color = CopiedColor{CCssColorParser::OklchToRgb(l_normalized, lch.second, lch.third), alpha};
	return true;
}

bool CQListCtrl::ParsePlainRgbColor(const CString& parseText, CopiedColor& color)
{
	// 4. --- Non-W3C Format Parsing ---
	// the parsers share r, g, b and the consumed character count, in this order
	RgbScan scan{};
	return ParseParenthesizedRgb(parseText, scan, color) ||
		ParseCommaSeparatedRgb(parseText, scan, color) ||
		ParseSpaceSeparatedRgb(parseText, scan, color) ||
		ParseSixDigitHex(parseText, scan, color);
}

bool CQListCtrl::ParseParenthesizedRgb(const CString& parseText, RgbScan& scan, CopiedColor& color)
{
	// Check for parenthesized RGB: "(255, 128, 0)" or "(255 128 0)"
	if ((parseText.Left(1) == _T("(") && parseText.Right(1) == _T(")")) == false)
		return false;

	CString content = parseText.Mid(1, parseText.GetLength() - 2);
	content.Trim();
	content.Replace(_T(','), _T(' '));

	if (swscanf(content, _T("%d %d %d %n"), &scan.r, &scan.g, &scan.b, &scan.charsConsumed) != 3)
		return false;

	CString remainingText = content.Mid(scan.charsConsumed);
	remainingText.Trim();

	if (remainingText.IsEmpty() == false)
		return false;

	return AcceptRgb(scan, color);
}

bool CQListCtrl::ParseCommaSeparatedRgb(const CString& parseText, RgbScan& scan, CopiedColor& color)
{
	// Check for comma-separated RGB: "255, 0, 0"
	if (swscanf(parseText, _T("%d , %d , %d %n"), &scan.r, &scan.g, &scan.b, &scan.charsConsumed) == 3 && scan.charsConsumed == parseText.GetLength())
	{
		return AcceptRgb(scan, color);
	}
	return false;
}

bool CQListCtrl::ParseSpaceSeparatedRgb(const CString& parseText, RgbScan& scan, CopiedColor& color)
{
	// Check for space-separated RGB: "255 128 0"
	if (swscanf(parseText, _T("%d %d %d %n"), &scan.r, &scan.g, &scan.b, &scan.charsConsumed) == 3 && scan.charsConsumed == parseText.GetLength())
	{
		return AcceptRgb(scan, color);
	}
	return false;
}

bool CQListCtrl::ParseSixDigitHex(const CString& parseText, RgbScan& scan, CopiedColor& color)
{
	// Check for 6-digit hex: "FF00CC"
	if ((parseText.GetLength() == 6 && IsHexString(parseText)) == false)
		return false;

	// %x stores an unsigned int; two hex digits always fit a byte.
	// Use %n here as well for consistency, though length check is sufficient.
	unsigned int red{};
	unsigned int green{};
	unsigned int blue{};
	if (swscanf(parseText, _T("%2x%2x%2x%n"), &red, &green, &blue, &scan.charsConsumed) == 3 && scan.charsConsumed == 6)
	{
		color = CopiedColor{RGB(red, green, blue), 255};
		return true;
	}
	return false;
}

bool CQListCtrl::AcceptRgb(const RgbScan& scan, CopiedColor& color)
{
	if ((IsRgbByte(scan.r) && IsRgbByte(scan.g) && IsRgbByte(scan.b)) == false)
		return false;

	color = CopiedColor{RGB(scan.r, scan.g, scan.b), 255};
	return true;
}

void CQListCtrl::DrawColorBox(CDC* pDC, CRect& rcText, const CopiedColor& color)
{
	CRect pastedRect(rcText);
	int boxSize = rcText.Height();
	pastedRect.right = pastedRect.left + boxSize;
	pastedRect.bottom = pastedRect.top + boxSize;

	if (color.alpha < 255)
	{
		DrawCheckerboard(pDC, pastedRect);

		// Use GDI+ for alpha blending
		Gdiplus::Graphics graphics(pDC->GetSafeHdc());
		// every parser gives an alpha in 0-255 (two hex digits or a value clamped to 0-1 times 255)
		Gdiplus::Color gdiplusColor(static_cast<BYTE>(color.alpha), GetRValue(color.color), GetGValue(color.color), GetBValue(color.color));
		Gdiplus::SolidBrush brush(gdiplusColor);
		graphics.FillRectangle(&brush, Gdiplus::Rect(pastedRect.left, pastedRect.top, pastedRect.Width(), pastedRect.Height()));
	}
	else
	{
		// Opaque color is faster with FillSolidRect and doesn't need a checkerboard.
		pDC->FillSolidRect(pastedRect, color.color);
	}

	rcText.left += boxSize + m_windowDpi->Scale(s_rowLeftBorder);
}


BOOL CQListCtrl::DrawRtfText(int nItem, CRect& crRect, CDC* pDC)
{
	if (Settings().m_bDrawRTF == FALSE)
		return FALSE;

	BOOL bRet = FALSE;

	CClipFormat* pThumbnail = GetItem_CF_RTF_ClipFormat(nItem);
	if (pThumbnail == NULL)
		return FALSE;

	// if there's no data, then we're done.
	if (pThumbnail->m_hgData == NULL)
		return FALSE;

	if (m_pFormatter == nullptr)
	{
		m_pFormatter = std::make_unique<CFormattedTextDraw>();
		m_pFormatter->Create();
	}

	if (m_rtfFormater.m_hWnd == NULL)
	{
		m_rtfFormater.Create(_T(""), _T(""), WS_CHILD | WS_VSCROLL |
			WS_HSCROLL | ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL |
			ES_AUTOHSCROLL, CRect(0, 0, 0, 0), this, static_cast<UINT>(-1));
	}

	if (m_pFormatter)
	{
		//somehow ms word places crazy rtf text onto the clipboard and our draw routine doesn't handle that
		//pass the rtf text into a richtext control and get it out and the contorl will clean  the rtf so our routine can draw it
		m_rtfFormater.SetRTF(pThumbnail->GetAsCStringA());
		CString betterRTF = m_rtfFormater.GetRTF();

		CComBSTR bStr(betterRTF);
		m_pFormatter->put_RTFText(bStr);

		m_pFormatter->Draw(pDC->m_hDC, crRect);

		bRet = TRUE;
	}

	return bRet;
}

// DrawBitMap loads a DIB from the DB, draws a crRect thumbnail of the image
//  to pDC and caches that thumbnail as a DIB in m_ThumbNails[ ItemID ].
// ALL items are cached in m_ThumbNails (those without images are cached with NULL m_hgData)
BOOL CQListCtrl::DrawBitMap(int nItem, CRect& crRect, CDC* pDC, const CString& csDescription)
{
	if (Settings().m_bDrawThumbnail == FALSE)
		return FALSE;

	CClipFormatQListCtrl* format = GetItem_CF_DIB_ClipFormat(nItem);
	if (format != NULL)
	{
		try
		{
			HGLOBAL smallImage = format->GetDibFittingToHeight(Settings(), theApp.Services().ClipboardFormats().Png(), pDC, crRect.Height());
			if (smallImage != NULL)
			{
				//Will return the width of the bitmap in nWidth
				int nWidth = 0;
				if (CBitmapHelper::DrawDIB(pDC, smallImage, crRect.left, crRect.top, nWidth))
				{
					// adjust the rect so other information can be drawn next to the thumbnail
					crRect.left += nWidth + 3;
				}
			}
		}
		catch (const DittoCore::ClipboardFormatError& error)
		{
			// the thumbnail is made once per clip; freeing the image keeps the next paint from
			// drawing it again, so the error is shown once
			format->Free();
			CErrorReport::Show(CStringUtil::Format(_T("Ditto cannot draw the clip's image: the image data is malformed (%s)."), CString(error.what()).GetString()));
		}
	}
	else if (csDescription.Find(_T("CF_DIB")) == 0)
	{
		crRect.left += crRect.Height();
	}

	return TRUE;
}

void CQListCtrl::RefreshVisibleRows()
{
	int nTopIndex = GetTopIndex();
	int nLastIndex = nTopIndex + GetCountPerPage();
	RedrawItems(nTopIndex, nLastIndex);
}

void CQListCtrl::RefreshRow(int row)
{
	RedrawItems(row, row);
}

void CQListCtrl::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	CListCtrl::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

BOOL CQListCtrl::OnEraseBkgnd(CDC* pDC)
{

	CRect rect;
	GetClientRect(&rect);
	CBrush myBrush(Settings().m_Theme.MainWindowBG());    // dialog background color
	CBrush* pOld = pDC->SelectObject(&myBrush);
	BOOL bRes = pDC->PatBlt(0, 0, rect.Width(), rect.Height(), PATCOPY);
	pDC->SelectObject(pOld);    // restore old brush
	return bRes;                       // CDialog::OnEraseBkgnd(pDC);

	// Simply returning TRUE seems OK since we do custom item
	//	painting.  However, there is a pixel buffer around the
	//	border of this control (not within the item rects)
	//	which becomes visually corrupt if it is not erased.

	// In most cases, I do not notice the erasure, so I have kept
	//	the call to CListCtrl::OnEraseBkgnd(pDC);

	// However, for some reason, bulk erasure is very noticeable when
	//	shift-scrolling the page to select a block of items, so
	//	I made a special case for that:
	//if(GetSelectedCount() >= 2)
	//	return TRUE;
	//return CListCtrl::OnEraseBkgnd(pDC);
}

BOOL CQListCtrl::OnToolTipText(UINT /*id*/, NMHDR* pNMHDR, LRESULT* pResult)
{
	CString strTipText;

	UINT_PTR nID = pNMHDR->idFrom;

	if (nID == 0)	  	// Notification in NT from automatically
		return FALSE;   	// created tooltip

	::SendMessage(pNMHDR->hwndFrom, TTM_SETMAXTIPWIDTH, 0, 500);

	if (Settings().m_tooltipTimeout > 0)
	{
		::SendMessage(pNMHDR->hwndFrom, TTM_SETDELAYTIME, TTDT_AUTOPOP, MAKELPARAM(Settings().m_tooltipTimeout, 0));
	}

	// Use Item's name as the tool tip. Change this for something different.
	// Like use its file size, etc.
	GetToolTipText((int)nID - 1, strTipText);

	//Replace the tabs with spaces, the tooltip didn't like the \t s
	strTipText.Replace(_T("\t"), _T("  "));

	// the tool tip control asks for ANSI or wide text; each gets its own encoding, kept in a
	// member because the control reads it after this handler returns
	if (pNMHDR->code == TTN_NEEDTEXTA)
	{
		m_toolTipTextA = CStringA(strTipText);
		reinterpret_cast<TOOLTIPTEXTA*>(pNMHDR)->lpszText = const_cast<LPSTR>(m_toolTipTextA.GetString());
	}
	else
	{
		m_toolTipTextW = strTipText;
		reinterpret_cast<TOOLTIPTEXTW*>(pNMHDR)->lpszText = const_cast<LPWSTR>(m_toolTipTextW.GetString());
	}
	* pResult = 0;

	return TRUE;    // message was handled
}

INT_PTR CQListCtrl::OnToolHitTest(CPoint point, TOOLINFO* pTI) const
{
	CRect rect;
	GetClientRect(&rect);
	if (rect.PtInRect(point))
	{
		if (GetItemCount())
		{
			int nTopIndex = GetTopIndex();
			int nBottomIndex = nTopIndex + GetCountPerPage();
			if (nBottomIndex > GetItemCount()) nBottomIndex = GetItemCount();
			for (int nIndex = nTopIndex; nIndex <= nBottomIndex; nIndex++)
			{
				GetItemRect(nIndex, rect, LVIR_BOUNDS);
				if (rect.PtInRect(point))
				{
					pTI->hwnd = m_hWnd;
					pTI->uId = (UINT)(nIndex + 1);
					pTI->lpszText = LPSTR_TEXTCALLBACK;
					pTI->rect = rect;
					pTI->uFlags = TTF_TRANSPARENT;
					return pTI->uId;
				}
			}
		}
	}

	return -1;
}

int CQListCtrl::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CListCtrl::OnCreate(lpCreateStruct) == -1)
		return -1;

	if (Settings().m_tooltipTimeout > 0 ||
		Settings().m_tooltipTimeout == -1)
	{
		EnableToolTips();
	}
	else
	{
		EnableToolTips(FALSE);
	}

	//m_pToolTip = new CToolTipEx;
	//m_pToolTip->Create(this);

	//m_pToolTip->SetNotifyWnd(GetParent());

	return 0;
}

BOOL CQListCtrl::PreTranslateMessage(MSG* pMsg)
{
	if (RunAccelerator(pMsg))
	{
		return TRUE;
	}

	if (IsToolTipValid())
	{
		if (m_pToolTip->OnMsg(pMsg))
			return TRUE;
	}

	switch (pMsg->message)
	{
	case WM_KEYDOWN:
		if (HandleKeyDown(pMsg->wParam, pMsg->lParam))
			return TRUE;
		break; // end case WM_KEYDOWN
	case WM_MOUSEWHEEL:
		// Will be handled by default, but ensure scrollbar updates after
		return PreTranslateMouseWheel(pMsg);

	case WM_VSCROLL:
		ASSERT(FALSE);
		break;
	} // end switch(pMsg->message)

	return CListCtrl::PreTranslateMessage(pMsg);
}

bool CQListCtrl::RunAccelerator(MSG* pMsg)
{
	CAccel a;
	if (m_Accels.OnMsg(pMsg, a, Settings().m_doubleKeyStrokeTimeout) == false)
		return false;

	RunAcceleratorCommand(a);
	return true;
}

void CQListCtrl::RunAcceleratorCommand(const CAccel& a)
{
	switch (a.Cmd)
	{
	case s_copyBufferHotKey1Cmd:
		PutSelectedItemOnDittoCopyBuffer(0);
		break;
	case s_copyBufferHotKey2Cmd:
		PutSelectedItemOnDittoCopyBuffer(1);
		break;
	case s_copyBufferHotKey3Cmd:
		PutSelectedItemOnDittoCopyBuffer(2);
		break;
	default:
		if (a.RefId == CHotKey::PASTE_OPEN_CLIP)
		{
			GetParent()->SendMessage(NmSelectDbId, a.Cmd, 0);
		}
		else if (a.RefId == CHotKey::MOVE_TO_GROUP)
		{
			GetParent()->SendMessage(NmMoveToGroup, a.Cmd, 0);
		}
	}
}

BOOL CQListCtrl::PreTranslateMouseWheel(MSG* pMsg)
{
	BOOL result = CListCtrl::PreTranslateMessage(pMsg);
	CWnd* pParent = GetParent();
	if (pParent && pParent->GetSafeHwnd())
	{
		pParent->PostMessage(NmUpdateScrollBar, TRUE, 0);
	}
	return result;
}

BOOL CQListCtrl::HandleKeyDown(WPARAM wParam, LPARAM lParam)
{
	if (IsToolTipValid())
	{
		MSG Msg;
		Msg.lParam = lParam;
		Msg.wParam = wParam;
		Msg.message = WM_KEYDOWN;
		if (m_pToolTip->OnMsg(&Msg))
			return TRUE;
	}

	WPARAM vk = wParam;

	switch (vk)
	{
	case 'A': // Ctrl-A = Select All
		if (CKeyboard::IsControlPressed())
		{
			SelectAllItems();
			return TRUE;
		}
		break;

	case VK_HOME:
		HandleHomeKey();
		return TRUE;
	} // end switch(vk)

	return FALSE;
}

void CQListCtrl::SelectAllItems()
{
	int nCount = GetItemCount();
	for (int i = 0; i < nCount; i++)
	{
		SetSelection(i);
	}
}

void CQListCtrl::HandleHomeKey()
{
	if (GetKeyState(VK_SHIFT) & 0x8000)
	{
		int nAnchor = GetSelectionMark();
		if (nAnchor < 0)
		{
			nAnchor = GetCaret();
		}

		if (nAnchor >= 0)
		{
			RemoveAllSelection();

			for (int i = 0; i <= nAnchor; i++)
			{
				SetSelection(i, TRUE);
			}

			ListView_SetSelectionMark(m_hWnd, nAnchor);

			SetCaret(0);
			EnsureVisible(0, FALSE);
		}
	}
	else
	{
		SetListPos(0);
	}
}

bool CQListCtrl::PostEventLoadedCheckDescription(int updatedRow)
{
	bool loadedClip = false;

	if (IsToolTipValid())
	{
		int toolTipClipId = m_pToolTip->GetClipId();
		int toolTipClipRow = m_pToolTip->GetClipRow();

		if (toolTipClipRow >= 0)
		{
			CLogger::Write(CStringUtil::Format(_T("PostEventLoadedCheckDescription refreshRow: %d tt_row: %d tt_id: %d"), updatedRow, toolTipClipRow, toolTipClipId));
		}

		//We tried to show the clip but we didn't have the id yet, it was loaded in a thread, now it's being updated
		//see if we need to show this rows description
		if (toolTipClipId <= 0 &&
			toolTipClipRow == updatedRow &&
			::IsWindow(m_toolTipHwnd))
		{
			ShowFullDescription(false, true);
			loadedClip = true;
		}
	}

	return loadedClip;
}

void CQListCtrl::SetToolTipImage(int nItem, CClipFormat& Clip)
{
	try
	{
		// the DIB if the clip has one, else the PNG
		for (const CLIPFORMAT cfType : { (CLIPFORMAT)CF_DIB, theApp.Services().ClipboardFormats().Png() })
		{
			Clip.m_cfType = cfType;
			if (GetClipData(nItem, Clip) && Clip.m_hgData)
			{
				m_pToolTip->SetGdiplusBitmap(std::unique_ptr<Gdiplus::Bitmap>(Clip.CreateGdiplusBitmap()));
				return;
			}
		}
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		// the description is still shown, without the image
		CErrorReport::Show(CStringUtil::Format(_T("Ditto cannot show the clip's image: the image data is malformed (%s)."), CString(error.what()).GetString()));
	}
}

bool CQListCtrl::ShowFullDescription(bool bFromAuto, bool fromNextPrev)
{
	if (this->GetSelectedCount() == 0)
	{
		return false;
	}

	int clipRow = this->GetCaret();
	int clipId = this->GetItemData(clipRow);

	CLogger::Write(CStringUtil::Format(_T("Show full description row: %d id: %d"), clipRow, clipId));

	if (IsToolTipShowingClip(clipId))
	{
		return false;
	}

	int nItem = GetCaret();
	CPoint pt{DescriptionPosition(nItem, bFromAuto)};

	CString csDescription;
	GetToolTipText(nItem, csDescription);

	PrepareToolTipWindow(fromNextPrev, pt);

	if (IsToolTipValid())
	{
		if (ShowClipInToolTip(nItem, clipId, clipRow, csDescription, pt) == false)
			return false;
	}

	return true;
}

bool CQListCtrl::IsToolTipShowingClip(int clipId)
{
	return IsToolTipValid() &&
		clipId > 0 &&
		m_pToolTip->GetClipId() == clipId &&
		::IsWindow(m_toolTipHwnd);
}

CPoint CQListCtrl::DescriptionPosition(int nItem, bool bFromAuto)
{
	CRect rc, crWindow;
	GetWindowRect(&crWindow);
	GetItemRect(nItem, rc, LVIR_BOUNDS);
	ClientToScreen(rc);

	CPoint pt;

	if (Settings().GetRememberDescPos())
	{
		Settings().GetDescWndPoint(pt);
	}
	else if (bFromAuto == false)
	{
		pt = CPoint(rc.left, rc.bottom);
	}
	else
	{
		pt = CPoint((crWindow.left + (crWindow.right - crWindow.left) / 2), rc.bottom);
	}

	return pt;
}

void CQListCtrl::PrepareToolTipWindow(bool fromNextPrev, CPoint& pt)
{
	if (IsToolTipValid() == false)
	{
		// a tool tip whose window is gone has already deleted itself (PostNcDestroy)
		CreateToolTip();
	}
	else
	{
		if (fromNextPrev)
		{
			CRect r;
			m_pToolTip->GetWindowRectEx(r);
			pt = r.TopLeft();
		}

		m_pToolTip->SetGdiplusBitmap(nullptr);
		m_pToolTip->SetRTFText("");
		m_pToolTip->SetToolTipText(_T(""));
		m_pToolTip->SetFolderPath(_T(""));
	}
}

bool CQListCtrl::ShowClipInToolTip(int nItem, int clipId, int clipRow, const CString& csDescription, CPoint pt)
{
	ResetToolTipContent(clipId, clipRow);

	CClipFormat Clip;

	if (LoadToolTipClipData(clipId) == false)
		return false;

	SetToolTipPlainText(nItem, Clip, csDescription);

	SetToolTipRtf(nItem, Clip);

	SetToolTipImage(nItem, Clip);

	m_pToolTip->Show(pt);
	return true;
}

void CQListCtrl::ResetToolTipContent(int clipId, int clipRow)
{
	m_pToolTip->SetTooltipActions(m_pToolTipActions);
	m_pToolTip->SetClipId(clipId);
	m_pToolTip->SetClipRow(clipRow);
	m_pToolTip->SetSearchText(m_searchText);
	LOGFONT lf;
	m_Font.GetLogFont(&lf);
	lf.lfHeight = m_windowDpi->UnScale(lf.lfHeight);
	m_pToolTip->SetLogFont(&lf, FALSE);

	m_pToolTip->SetClipData(_T(""));
	m_pToolTip->SetToolTipText(_T(""));
	m_pToolTip->SetRTFText("");
}

bool CQListCtrl::LoadToolTipClipData(int clipId)
{
	try
	{
		CppSQLite3Query q = theApp.Services().Database().execQueryEx(_T("SELECT lID, lDate, lastPasteDate, lDontAutoDelete, QuickPasteText, lShortCut, globalShortCut, stickyClipOrder, stickyClipGroupOrder, lParentID FROM Main WHERE lID = %d"), clipId);
		if (q.eof() == false)
		{
			CString clipData{ClipDataText(q)};

			int parentId = q.getIntField(_T("lParentID"));
			if (parentId > 0)
			{
				CString folder = CClipDatabase::FolderPath(theApp.Services().Database(), parentId);

				m_pToolTip->SetFolderPath(folder);
			}

			m_pToolTip->SetClipData(clipData);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the description of clip id %d failed: %s"), clipId, e.errorMessage()));
		return false;
	}

	return true;
}

void CQListCtrl::SetToolTipPlainText(int nItem, CClipFormat& Clip, const CString& csDescription)
{
	bool bSetPlainText = false;

	Clip.m_cfType = CF_UNICODETEXT;
	if (GetClipData(nItem, Clip) && Clip.m_hgData)
	{
		m_pToolTip->SetToolTipText(Clip.GetAsCString());
		bSetPlainText = true;

		Clip.Free();
		Clip.Clear();
	}

	if (bSetPlainText == false)
	{
		Clip.m_cfType = CF_TEXT;
		if (GetClipData(nItem, Clip) && Clip.m_hgData)
		{
			CString cs(Clip.GetAsCStringA());

			m_pToolTip->SetToolTipText(cs);
			bSetPlainText = true;

			Clip.Free();
			Clip.Clear();
		}
	}

	if (bSetPlainText == false)
	{
		m_pToolTip->SetToolTipText(csDescription);
	}
}

void CQListCtrl::SetToolTipRtf(int nItem, CClipFormat& Clip)
{
	// registered clipboard format ids are 16-bit (0xC000-0xFFFF)
	Clip.m_cfType = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CF_RTF));

	if (GetClipData(nItem, Clip) && Clip.m_hgData)
	{
		m_pToolTip->SetRTFText(Clip.GetAsCStringA());

		Clip.Free();
		Clip.Clear();
	}
}

CString CQListCtrl::ClipDataText(CppSQLite3Query& q)
{
	CString clipData{};
	COleDateTime time{(time_t)q.getInt64Field(_T("lDate"))};
	clipData += "Added: " + time.Format();

	COleDateTime modified{(time_t)q.getInt64Field(_T("lastPasteDate"))};
	clipData += _T(" | Last Used: ") + modified.Format();

	if (q.getIntField(_T("lDontAutoDelete")) > 0)
	{
		clipData += _T(" | Never Auto Delete");
	}

	CString csQuickPaste{q.getStringField(_T("QuickPasteText"))};
	if (csQuickPaste.IsEmpty() == FALSE)
	{
		clipData += _T(" | Quick Paste = ");
		clipData += csQuickPaste;
	}

	int shortCut{q.getIntField(_T("lShortCut"))};
	if (shortCut > 0)
	{
		clipData += _T(" | ");
		clipData += CHotKey::GetHotKeyDisplayStatic(shortCut);

		BOOL globalShortCut{q.getIntField(_T("globalShortCut"))};
		if (globalShortCut)
		{
			clipData += _T(" - Global Shortcut Key");
		}
	}

	if (theApp.Services().State().m_GroupID > 0)
	{
		int sticky{q.getIntField(_T("stickyClipGroupOrder"))};
		if (sticky != CClip::InvalidSticky)
		{
			clipData += _T(" | ");
			clipData += _T(" - Sticky In Group");
		}
	}
	else
	{
		int sticky{q.getIntField(_T("stickyClipOrder"))};
		if (sticky != CClip::InvalidSticky)
		{
			clipData += _T(" | ");
			clipData += _T(" - Sticky");
		}
	}

	return clipData;
}

void CQListCtrl::GetToolTipText(int nItem, CString& csText)
{
	CWnd* pParent = GetParent();
	if (pParent && (pParent->GetSafeHwnd() != NULL))
	{
		CQListToolTipText info;
		memset(&info, 0, sizeof(info));
		info.hdr.code = NmGetToolTipText;
		info.hdr.hwndFrom = GetSafeHwnd();
		info.hdr.idFrom = GetDlgCtrlID();
		info.lItem = nItem;
		//plus 100 for extra info - shortcut and such
		int maxCharacters = Settings().GetMaxToolTipCharacters();
		info.cchTextMax = min(maxCharacters, Settings().m_bDescTextSize) + 200;
		info.pszText = csText.GetBufferSetLength(info.cchTextMax);

		pParent->SendMessage(WM_NOTIFY, (WPARAM)info.hdr.idFrom, (LPARAM)&info);

		csText.ReleaseBuffer();
	}
}

BOOL CQListCtrl::GetClipData(int nItem, CClipFormat& Clip)
{
	return CClipDataReader(theApp.Services().Database()).GetClipData(static_cast<long>(GetItemData(nItem)), Clip);
}

DWORD CQListCtrl::GetItemData(int nItem)
{
	if ((GetStyle() & LVS_OWNERDATA))
	{
		CWnd* pParent = GetParent();
		if (pParent && (pParent->GetSafeHwnd() != NULL))
		{
			LV_DISPINFO info;
			memset(&info, 0, sizeof(info));
			info.hdr.code = LVN_GETDISPINFO;
			info.hdr.hwndFrom = GetSafeHwnd();
			info.hdr.idFrom = GetDlgCtrlID();

			info.item.iItem = nItem;
			info.item.lParam = -1;
			info.item.mask = LVIF_PARAM;

			pParent->SendMessage(WM_NOTIFY, (WPARAM)info.hdr.idFrom, (LPARAM)&info);

			return (DWORD)info.item.lParam;
		}
	}

	return (DWORD)CListCtrl::GetItemData(nItem);
}

CClipFormatQListCtrl* CQListCtrl::GetItem_CF_DIB_ClipFormat(int nItem)
{
	CClipFormatQListCtrl* format = NULL;

	CWnd* pParent = GetParent();
	if (pParent && (pParent->GetSafeHwnd() != NULL))
	{
		LV_DISPINFO info;
		memset(&info, 0, sizeof(info));
		info.hdr.code = LVN_GETDISPINFO;
		info.hdr.hwndFrom = GetSafeHwnd();
		info.hdr.idFrom = GetDlgCtrlID();

		info.item.iItem = nItem;
		info.item.lParam = NULL;
		info.item.mask = s_lvifCfDib;

		pParent->SendMessage(WM_NOTIFY, (WPARAM)info.hdr.idFrom, (LPARAM)&info);

		if (info.item.lParam != NULL)
		{
			format = (CClipFormatQListCtrl*)info.item.lParam;
		}
	}

	return format;
}

CClipFormatQListCtrl* CQListCtrl::GetItem_CF_RTF_ClipFormat(int nItem)
{
	CClipFormatQListCtrl* format = NULL;

	CWnd* pParent = GetParent();
	if (pParent && (pParent->GetSafeHwnd() != NULL))
	{
		LV_DISPINFO info;
		memset(&info, 0, sizeof(info));
		info.hdr.code = LVN_GETDISPINFO;
		info.hdr.hwndFrom = GetSafeHwnd();
		info.hdr.idFrom = GetDlgCtrlID();

		info.item.iItem = nItem;
		info.item.lParam = NULL;
		info.item.mask = s_lvifCfRichText;

		pParent->SendMessage(WM_NOTIFY, (WPARAM)info.hdr.idFrom, (LPARAM)&info);

		if (info.item.lParam != NULL)
		{
			format = (CClipFormatQListCtrl*)info.item.lParam;
		}
	}

	return format;
}

void CQListCtrl::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CListCtrl::OnHScroll(nSBCode, nPos, pScrollBar);
}

void CQListCtrl::DestroyAndCreateAccelerator(BOOL bCreate, CppSQLite3DB& db)
{
	m_Accels.RemoveAll();

	if (bCreate)
	{
		CMainTableFunctions::LoadAcceleratorKeys(m_Accels, db);

		LoadDittoCopyBufferHotkeys();
	}
}

void CQListCtrl::LoadDittoCopyBufferHotkeys()
{
	CCopyBufferItem Item;
	CAccel a;

	// the copy buffer command ids are negative; CAccel::Cmd keeps their bit pattern
	Settings().GetCopyBufferItem(0, Item);
	if (Item.m_lCopyHotKey > 0)
	{
		a.Cmd = s_copyBufferHotKey1Cmd;
		a.Key = Item.m_lCopyHotKey;
		m_Accels.AddAccel(a);
	}

	Settings().GetCopyBufferItem(1, Item);
	if (Item.m_lCopyHotKey > 0)
	{
		a.Cmd = s_copyBufferHotKey2Cmd;
		a.Key = Item.m_lCopyHotKey;
		m_Accels.AddAccel(a);
	}

	Settings().GetCopyBufferItem(2, Item);
	if (Item.m_lCopyHotKey > 0)
	{
		a.Cmd = s_copyBufferHotKey3Cmd;
		a.Key = Item.m_lCopyHotKey;
		m_Accels.AddAccel(a);
	}
}

void CQListCtrl::OnKillFocus(CWnd* pNewWnd)
{
	CListCtrl::OnKillFocus(pNewWnd);

	//if(FocusOnToolTip() == FALSE)
		//m_pToolTip->Hide();
}

HWND CQListCtrl::GetToolTipHWnd()
{
	if (IsToolTipValid())
		return m_pToolTip->GetSafeHwnd();

	return NULL;
}

BOOL CQListCtrl::SetItemCountEx(int iCount, DWORD dwFlags /* = 0 */)
{
	return CListCtrl::SetItemCountEx(iCount, dwFlags);
}

void CQListCtrl::OnSelectionChange(NMHDR* pNMHDR, LRESULT* /*pResult*/)
{
	NMLISTVIEW* pnmv = (NMLISTVIEW*)pNMHDR;

	if ((pnmv->uNewState == 3) ||
		(pnmv->uNewState == 1))
	{
		NotifySelectionChanged();
	}

	UpdateAllSelectedState();
}

void CQListCtrl::NotifySelectionChanged()
{
	// Notify parent to update modern scrollbar when selection changes (keyboard navigation)
	CWnd* pParent = GetParent();
	if (pParent && pParent->GetSafeHwnd())
	{
		pParent->PostMessage(NmUpdateScrollBar, FALSE, 0);
	}

	if (IsToolTipValid() &&
		::IsWindowVisible(m_pToolTip->m_hWnd))
	{
		this->ShowFullDescription(false, true);
	}
	if (Settings().m_bAllwaysShowDescription)
	{
		KillTimer(TimerShowProperties);
		SetTimer(TimerShowProperties, 300, NULL);
	}
	if (GetSelectedCount() > 0)
		theApp.Services().Windows().SetStatus(NULL, FALSE);
}

void CQListCtrl::UpdateAllSelectedState()
{
	if (GetSelectedCount() == static_cast<UINT>(this->GetItemCount()))
	{
		if (m_allSelected == false)
		{
			CLogger::Log(CStringUtil::Format(_T("List box Select All")));

			GetParent()->SendMessage(NmAllSelected, 0, 0);
			m_allSelected = true;
		}
	}
	else if (m_allSelected == true)
	{
		CLogger::Log(CStringUtil::Format(_T("List box REMOVED Select All")));
		m_allSelected = false;
	}
}

void CQListCtrl::OnTimer(UINT_PTR nIDEvent)
{
	//http://support.microsoft.com/kb/200054
	//OnTimer() Is Not Called Repeatedly for a List Control
	bool callBase = true;

	switch (nIDEvent)
	{
	case TimerShowProperties:
	{
		if (theApp.Services().State().m_bShowingQuickPaste)
			ShowFullDescription(true);
		KillTimer(TimerShowProperties);

		callBase = false;
	}
	break;

	case TimerHideScroll:
	{
		CPoint cursorPos;
		GetCursorPos(&cursorPos);

		CRect crWindow;
		this->GetWindowRect(&crWindow);



		//check and see if they moved out of the scroll area
		//If they did tell our parent so
		if (MouseInScrollBarArea(crWindow, cursorPos) == false)
		{
			StopHideScrollBarTimer();
		}

		callBase = false;
	}
	break;

	case TimerShowScroll:
	{
		CPoint cursorPos;
		GetCursorPos(&cursorPos);

		CRect crWindow;
		this->GetWindowRect(&crWindow);

		//Adjust for the v-scroll bar being off of the screen
		crWindow.right -= m_windowDpi->Scale(GetSystemMetrics(SM_CXVSCROLL));
		crWindow.bottom -= m_windowDpi->Scale(::GetSystemMetrics(SM_CXHSCROLL));

		//Check and see if we are still in the cursor area
		if (MouseInScrollBarArea(crWindow, cursorPos))
		{
			m_timerToHideScrollAreaSet = true;
			GetParent()->SendMessage(NmShowHideScrollBars, 1, 0);

			//Start looking to hide the scroll bars
			SetTimer(TimerHideScroll, 1000, NULL);
		}

		KillTimer(TimerShowScroll);

		callBase = false;
	}
	break;
	}

	if (callBase)
	{
		CListCtrl::OnTimer(nIDEvent);
	}
}

void CQListCtrl::SetLogFont(LOGFONT& font)
{
	m_Font.DeleteObject();
	m_boldFont.DeleteObject();

	m_Font.CreateFontIndirect(&font);
	font.lfWeight = 600;
	m_boldFont.CreateFontIndirect(&font);

	SetFont(&m_Font);
}

void CQListCtrl::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CListCtrl::OnVScroll(nSBCode, nPos, pScrollBar);
	
	// Notify parent to update modern scrollbar
	CWnd* pParent = GetParent();
	if (pParent && pParent->GetSafeHwnd())
	{
		pParent->PostMessage(NmUpdateScrollBar, TRUE, 0);
	}
}

BOOL CQListCtrl::OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult)
{
	NMLVCACHEHINT* pcachehint = NULL;

	if (message == WM_NOTIFY)
	{
		NMHDR* phdr = (NMHDR*)lParam;

		switch (phdr->code)
		{
		case LVN_ODCACHEHINT:
			pcachehint = (NMLVCACHEHINT*)phdr;

			GetParent()->SendMessage(NmFillRestOfList, pcachehint->iFrom, pcachehint->iTo);
			return FALSE;
		}
	}

	return CListCtrl::OnChildNotify(message, wParam, lParam, pLResult);
}

BOOL CQListCtrl::OnItemDeleted(long lID)
{
	BOOL bRet2 = m_RTFData.RemoveKey(lID);

	return (bRet2);
}

void CQListCtrl::OnMouseMove(UINT nFlags, CPoint point)
{
	if (Settings().m_showScrollBar == FALSE)
	{
		CRect crWindow;
		this->GetWindowRect(&crWindow);
		ScreenToClient(&crWindow);

		// Don't subtract scrollbar size - detect in the full window area
		// This prevents flickering when scrollbar appears/disappears

		if (MouseInScrollBarArea(crWindow, point))
		{
			// Show scrollbar immediately when mouse enters scrollbar area
			if (m_mouseOverScrollAreaStart == 0)
			{
				m_mouseOverScrollAreaStart = GetTickCount64();
				
				// For modern scrollbar, notify parent
				if (Settings().m_useModernScrollBar)
				{
					GetParent()->PostMessage(NmUpdateScrollBar, TRUE, 0);
				}
				else
				{
					// For native scrollbar, show immediately and start hide timer
					m_timerToHideScrollAreaSet = true;
					GetParent()->SendMessage(NmShowHideScrollBars, 1, 0);
					SetTimer(TimerHideScroll, 1000, NULL);
				}
			}
		}
		else
		{
			m_mouseOverScrollAreaStart = 0;
			if (m_timerToHideScrollAreaSet)
			{
				StopHideScrollBarTimer();
			}
			KillTimer(TimerShowScroll);
		}
	}

	CListCtrl::OnMouseMove(nFlags, point);
}

bool CQListCtrl::MouseInScrollBarArea(CRect crWindow, CPoint point)
{
	int scrollBarWidth = m_windowDpi->Scale(::GetSystemMetrics(SM_CXVSCROLL));
	int scrollBarHeight = m_windowDpi->Scale(::GetSystemMetrics(SM_CYHSCROLL));
	int extraMargin = m_windowDpi->Scale(6); // Small extra margin for easier detection

	CRect crRight(crWindow);
	CRect crBottom(crWindow);

	// Detect from the right edge of the window (includes scrollbar area when visible)
	crRight.left = crRight.right - scrollBarWidth - extraMargin;
	crBottom.top = crBottom.bottom - scrollBarHeight - extraMargin;

	/*CString cs;
	cs.Format(_T("point.x: %d, Width: %d, Height: %d\n"), point.x, crWindow.Width(), crWindow.Height());
	OutputDebugString(cs);*/

	if (crRight.PtInRect(point) || crBottom.PtInRect(point))
	{
		return true;
	}

	return false;
}

void CQListCtrl::StopHideScrollBarTimer()
{
	GetParent()->SendMessage(NmShowHideScrollBars, 0, 0);

	m_timerToHideScrollAreaSet = false;
	KillTimer(TimerHideScroll);
}

void CQListCtrl::SetSearchText(CString text)
{
	m_searchText = text;
}

void CQListCtrl::HidePopup(bool checkShowPersistant)
{
	if (IsToolTipValid())
	{
		if (checkShowPersistant == false ||
			m_pToolTip->GetShowPersistant() == false)
		{
			m_pToolTip->Hide();
		}
	}
}

BOOL CQListCtrl::IsToolTipWindowVisible()
{
	if (IsToolTipValid())
	{
		return ::IsWindowVisible(m_toolTipHwnd);
	}

	return FALSE;
}

void CQListCtrl::ToggleToolTipShowPersistant()
{
	if (IsToolTipValid())
	{
		m_pToolTip->ToggleShowPersistant();
	}
}

bool CQListCtrl::ToggleToolTipWordWrap()
{
	bool didWordWrap = false;
	if (IsToolTipValid())
	{
		didWordWrap = m_pToolTip->ToggleWordWrap();
	}

	return didWordWrap;
}


BOOL CQListCtrl::IsToolTipWindowFocus()
{
	if (IsToolTipValid())
	{
		return ::GetFocus() == m_toolTipHwnd ||
			::GetParent(::GetFocus()) == m_toolTipHwnd;
	}

	return FALSE;
}

bool CQListCtrl::IsToolTipShowPersistant()
{
	if (IsToolTipValid())
	{
		return m_pToolTip->GetShowPersistant();
	}

	return false;
}

void CQListCtrl::DoToolTipSearch()
{
	if (IsToolTipValid())
	{
		return m_pToolTip->DoSearch();
	}
}

void CQListCtrl::HideToolTip()
{
	if (IsToolTipValid())
	{
		m_pToolTip->Hide();
	}
}

void CQListCtrl::OnDpiChanged()
{
	SetDpiInfo(m_windowDpi);
}

void CQListCtrl::SetDpiInfo(CDPI* dpi)
{
	m_windowDpi = dpi;

	m_groupFolder.Reset();
	m_groupFolder.LoadStdImageDPI(m_windowDpi->GetDPI(), IDB_OPEN_FOLDER_16_16, IDB_OPEN_FOLDER_20_20, IDB_OPEN_FOLDER_24_24, IDB_OPEN_FOLDER_24_24, IDB_OPEN_FOLDER_32_32, _T("PNG"));

	m_dontDeleteImage.Reset();
	m_dontDeleteImage.LoadStdImageDPI(m_windowDpi->GetDPI(), IDB_YELLOW_STAR_16_16, IDB_YELLOW_STAR_20_20, IDB_YELLOW_STAR_24_24, IDB_YELLOW_STAR_24_24, IDB_YELLOW_STAR_32_32, _T("PNG"));

	m_inFolderImage.Reset();
	m_inFolderImage.LoadStdImageDPI(m_windowDpi->GetDPI(), IDB_IN_FOLDER_16_16, IDB_IN_FOLDER_20_20, IDB_IN_FOLDER_24_24, IDB_IN_FOLDER_24_24, IDB_IN_FOLDER_32_32, _T("PNG"));

	m_shortCutImage.Reset();
	m_shortCutImage.LoadStdImageDPI(m_windowDpi->GetDPI(), IDB_KEY_16_16, IDB_KEY_20_20, IDB_KEY_24_24, IDB_KEY_24_24, IDB_KEY_32_32, _T("PNG"));

	m_stickyImage.Reset();
	m_stickyImage.LoadStdImageDPI(m_windowDpi->GetDPI(), IDB_STICKY_16_16, IDB_STICKY_20_20, IDB_STICKY_24_24, IDB_STICKY_24_24, IDB_STICKY_32_32, _T("PNG"));

	CreateSmallFont();
}

bool CQListCtrl::IsToolTipValid() const
{
	// the tool tip deletes itself when its window goes away: only the stored handle may be read
	// until the window is known to be alive (and still the one m_pToolTip points to)
	return m_pToolTip != NULL &&
		::IsWindow(m_toolTipHwnd) &&
		CWnd::FromHandlePermanent(m_toolTipHwnd) == m_pToolTip;
}

void CQListCtrl::CreateToolTip()
{
	m_pToolTip = NULL;
	m_toolTipHwnd = NULL;

	// a self-deleting window: CWnd::CreateEx calls PostNcDestroy on failure too, so the window
	// owns the object from the Create call on
	CToolTipEx *pToolTip{ std::make_unique<CToolTipEx>().release() }; // ownership: the window (PostNcDestroy deletes it, also when Create fails)
	if (pToolTip->Create(this) == FALSE)
	{
		AfxThrowResourceException();
	}

	m_pToolTip = pToolTip;
	m_toolTipHwnd = m_pToolTip->GetSafeHwnd();
	m_pToolTip->SetNotifyWnd(GetParent());
}

void CQListCtrl::CreateSmallFont()
{
	// QPasteWnd calls this directly as well: free the previous font first
	m_SmallFont.DeleteObject();

	LOGFONT lf;

	lf.lfHeight = -MulDiv(Settings().GetFirstTenHotKeysFontSize(), m_windowDpi->GetDPI(), 72);
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_LIGHT;
	lf.lfItalic = FALSE;
	lf.lfUnderline = FALSE;
	lf.lfStrikeOut = FALSE;
	lf.lfCharSet = ANSI_CHARSET;
	lf.lfOutPrecision = OUT_STRING_PRECIS;
	lf.lfClipPrecision = CLIP_STROKE_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = VARIABLE_PITCH | FF_DONTCARE;
	lstrcpy(lf.lfFaceName, _T("Small Font"));

	m_SmallFont.CreateFontIndirect(&lf);
}

void CQListCtrl::OnMouseHWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
	if (zDelta < 0)
	{
		this->SendMessage(WM_HSCROLL, SB_LINERIGHT, NULL);
	}
	else
	{
		this->SendMessage(WM_HSCROLL, SB_LINELEFT, NULL);
	}

	// Notify parent to update modern scrollbar
	CWnd* pParent = GetParent();
	if (pParent && pParent->GetSafeHwnd())
	{
		pParent->PostMessage(NmUpdateScrollBar, TRUE, 0);
	}

	//CListCtrl::OnMouseHWheel(nFlags, zDelta, pt);
}
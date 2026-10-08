#if !defined(AFX_QLISTCTRL_H__30BEB04A_4B97_4943_BB73_C5128E66B4ED__INCLUDED_)
#define AFX_QLISTCTRL_H__30BEB04A_4B97_4943_BB73_C5128E66B4ED__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// QListCtrl.h : header file
//
#include <array>
#include <memory>
#include <vector>

#include "..\Shared\ArrayEx.h"
#include "ToolTipEx.h"
#include "FormattedTextDraw.h"
#include "sqlite/CppSQLite3.h"
#include "ClipFormatQListCtrl.h"
#include "Accels.h"
#include "GdiImageDrawer.h"
#include "DPI.h"
#include "HtmlTextDrawer.h"

class CQListToolTipText
{
public:
	NMHDR hdr;
	long lItem;
	LPTSTR pszText;
	int cchTextMax;
};


typedef CMap<long, long, CClipFormat, CClipFormat&> CMapIDtoCF;

class CGetSetOptions;

class CQListCtrl : public CListCtrl
{
private:
	/** @brief The application settings (theApp's services; this control is created by the framework).
	@return the settings. */
	CGetSetOptions& Settings() const;

public:
	/**
	 * @brief The window messages and notification codes that the list, the search box, the group tree
	 * and the tool tip send to the paste window (WM_USER based; the values never change).
	 */
	enum : UINT
	{
		/** @brief The search box got the Enter key. */
		NmSearchEnterPressed = WM_USER + 0x100,
		/** @brief The Delete key in the list or the search box. */
		NmDelete = WM_USER + 0x104,
		/** @brief The WM_NOTIFY code that asks the parent for a row's tool tip text. */
		NmGetToolTipText = WM_USER + 0x107,
		/** @brief Selects the clip with a database id (wParam). */
		NmSelectDbId = WM_USER + 0x108,
		/** @brief The group tree selected a group (wParam: the group id). */
		NmGroupTreeMessage = WM_USER + 0x110,
		/** @brief Starts the search of the search box text. */
		CbSearch = WM_USER + 0x112,
		/** @brief An up/down key in the search box (wParam, lParam: the key message's). */
		CbUpDown = WM_USER + 0x113,
		/** @brief The tool tip window became inactive. */
		NmInactiveToolTipWnd = WM_USER + 0x114,
		/** @brief Loads the rows from wParam to lParam (the list's cache hint). */
		NmFillRestOfList = WM_USER + 0x115,
		/** @brief The loader thread counted the rows (wParam: the count). */
		NmSetListCount = WM_USER + 0x116,
		/** @brief A clip was deleted (wParam: the clip id). */
		NmItemDeleted = WM_USER + 0x118,
		/** @brief Every row of the list is selected. */
		NmAllSelected = WM_USER + 0x119,
		/** @brief Refreshes a row (wParam: the clip id, lParam: the row). */
		NmRefreshRow = WM_USER + 0x120,
		/** @brief Shows (wParam 1) or hides (wParam 0) the scroll bars. */
		NmShowHideScrollBars = WM_USER + 0x122,
		/** @brief Cancels the search. */
		NmCancelSearch = WM_USER + 0x123,
		/** @brief Opens the options window. */
		NmPostOptionsWindow = WM_USER + 0x124,
		/** @brief Shows the properties of a group (wParam: the group id). */
		NmShowProperties = WM_USER + 0x125,
		/** @brief Creates a new group (wParam: the parent group id). */
		NmNewGroup = WM_USER + 0x126,
		/** @brief Deletes a group (wParam: the group id). */
		NmDeleteId = WM_USER + 0x127,
		/** @brief Moves the selection to a group (wParam: the group id). */
		NmMoveToGroup = WM_USER + 0x128,
		/** @brief The search box got the focus. */
		NmFocusOnSearch = WM_USER + 0x129,
		/** @brief Copies the selected clip. */
		NmCopyClip = WM_USER + 0x130,
		/** @brief Updates the paste window's scroll bar. */
		NmUpdateScrollBar = WM_USER + 0x131,
	};

	/** @brief The accelerator command of copy buffer 1's copy hot key (-100; CAccel::Cmd keeps its bit pattern). */
	static constexpr DWORD s_copyBufferHotKey1Cmd{ static_cast<DWORD>(-100) };
	/** @brief The accelerator command of copy buffer 2's copy hot key (-101; CAccel::Cmd keeps its bit pattern). */
	static constexpr DWORD s_copyBufferHotKey2Cmd{ static_cast<DWORD>(-101) };
	/** @brief The accelerator command of copy buffer 3's copy hot key (-102; CAccel::Cmd keeps its bit pattern). */
	static constexpr DWORD s_copyBufferHotKey3Cmd{ static_cast<DWORD>(-102) };

	/** @brief The LVITEM mask bit that asks the parent for a row's CF_DIB format (LVN_GETDISPINFO). */
	static constexpr UINT s_lvifCfDib{ 0x10000000 };
	/** @brief The LVITEM mask bit that asks the parent for a row's RTF format (LVN_GETDISPINFO; a bit of its own,
	 *         so a CF_DIB request does not also fill the RTF answer). */
	static constexpr UINT s_lvifCfRichText{ 0x20000000 };
	static_assert((s_lvifCfDib & s_lvifCfRichText) == 0, "the CF_DIB and RTF requests need bits of their own");

	// Construction
public:
	CQListCtrl();

	// Attributes
public:
	// Operations
public:
	// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CQListCtrl)
public:
	virtual INT_PTR OnToolHitTest(CPoint point, TOOLINFO* pTI) const;
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	virtual BOOL OnChildNotify(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pLResult);
	//}}AFX_VIRTUAL

	// Implementation
public:
	virtual ~CQListCtrl();

	BOOL m_bShowTextForFirstTenHotKeys;
	// returns the position 1-10 if the index is in the FirstTen block else -1
	int GetFirstTenNum(int index);

	void SetNumberOfLinesPerRow(int nLines, bool force);
	void GetSelectionIndexes(ARRAY& arr);
	void GetSelectionItemData(ARRAY& arr);
	void RefreshVisibleRows();
	void RefreshRow(int row);
	void RemoveAllSelection();
	BOOL SetSelection(int nRow, BOOL bSelect = TRUE);
	BOOL SetText(int nRow, int nCol, CString cs);
	BOOL SetFormattedText(int nRow, int nCol, LPCTSTR lpszFormat, ...);
	BOOL SetCaret(int nRow, BOOL bFocus = TRUE);
	long GetCaret();
	// moves the caret to the given index, selects it, and ensures it is visible.
	BOOL SetListPos(int index);
	bool PutSelectedItemOnDittoCopyBuffer(long lBuffer);

	virtual DROPEFFECT OnDragOver(COleDataObject* pDataObject, DWORD dwKeyState, CPoint point);

	DWORD GetItemData(int nItem);
	CClipFormatQListCtrl* GetItem_CF_DIB_ClipFormat(int nItem);
	CClipFormatQListCtrl* GetItem_CF_RTF_ClipFormat(int nItem);
	void GetToolTipText(int nItem, CString& csText);

	void SetShowTextForFirstTenHotKeys(BOOL bVal) { m_bShowTextForFirstTenHotKeys = bVal; }
	void SetShowIfClipWasPasted(BOOL val) { m_showIfClipWasPasted = val; }

	void DestroyAndCreateAccelerator(BOOL bCreate, CppSQLite3DB& db);

	bool PostEventLoadedCheckDescription(int updatedRow);
	bool ShowFullDescription(bool bFromAuto = false, bool fromNextPrev = false);
	BOOL SetItemCountEx(int iCount, DWORD dwFlags = 0);

	void HidePopup(bool checkShowPersistant);
	void ToggleToolTipShowPersistant();
	bool ToggleToolTipWordWrap();
	void SetTooltipActions(CAccels* pToolTipActions) { m_pToolTipActions = pToolTipActions; }
	bool IsToolTipShowPersistant();
	void DoToolTipSearch();
	void HideToolTip();

	void SetLogFont(LOGFONT& font);

	HWND GetToolTipHWnd();

	BOOL HandleKeyDown(WPARAM wParam, LPARAM lParam);

	BOOL OnItemDeleted(long lID);

	BOOL IsToolTipWindowVisible();
	BOOL IsToolTipWindowFocus();

	int GetRowHeight() { return m_rowHeight; }

	void SetSearchText(CString text);

	void SetDpiInfo(CDPI* dpi);

	void CreateSmallFont();

	void OnDpiChanged();

	void LoadCopyOrCutToClipboard();

protected:
	/**
	 * @brief Creates the description tool tip window and hands it to the window itself.
	 *
	 * Sets m_pToolTip and m_toolTipHwnd; both stay empty when the window cannot be created (a
	 * failed Create has already deleted the object through PostNcDestroy).
	 * @throws CResourceException when the window cannot be created.
	 */
	void CreateToolTip();
	BOOL GetClipData(int nItem, CClipFormat& Clip);
	// Puts the item's image (DIB, else PNG) into the tooltip; reports a malformed image.
	void SetToolTipImage(int nItem, CClipFormat& Clip);
	BOOL DrawBitMap(int nItem, CRect& crRect, CDC* pDC, const CString& csDescription);
	void LoadDittoCopyBufferHotkeys();
	bool MouseInScrollBarArea(CRect crWindow, CPoint point);
	BOOL DrawRtfText(int nItem, CRect& crRect, CDC* pDC);
	void StopHideScrollBarTimer();
	bool IsHexString(const CString& str);
	COLORREF HslToRgb(double h, double s, double l);

	void DrawCheckerboard(CDC* pDC, CRect rect);

	/** @brief The timer ids of the list (SetTimer / OnTimer). */
	enum : UINT
	{
		/** @brief Shows the selected clip's description in the tool tip after the selection settled. */
		TimerShowProperties = 1,
		/** @brief Hides the scroll bars after the mouse left their area. */
		TimerHideScroll = 2,
		/** @brief Shows the scroll bars when the mouse rests in their area. */
		TimerShowScroll = 3,
	};
	/** @brief The space below a row's text, in unscaled pixels. */
	static constexpr int s_rowBottomBorder{ 4 };
	/** @brief The space left of a row's text, in unscaled pixels. */
	static constexpr int s_rowLeftBorder{ 3 };

	/**
	 * @brief Whether the description tool tip exists and its window is alive.
	 * @return true when m_pToolTip is set, m_toolTipHwnd is a window and that window is m_pToolTip
	 *         (the object is not read: it is deleted when its window goes away).
	 */
	bool IsToolTipValid() const;

	// The tool tip texts handed to the tool tip control; they must outlive OnToolTipText
	CStringW m_toolTipTextW{};
	CStringA m_toolTipTextA{};
	/** @brief The font of the first-ten hot key numbers; empty until CreateSmallFont. */
	CFont m_SmallFont{};
	CAccels m_Accels;
	CMapIDtoCF m_RTFData;
	/** @brief The description tool tip (non-owning: the window deletes itself in PostNcDestroy). */
	CToolTipEx* m_pToolTip;
	HWND m_toolTipHwnd{};
	CFont m_Font;
	CFont m_boldFont;
	/** @brief The RTF thumbnail renderer (owned); created on first use. */
	std::unique_ptr<IFormattedTextDraw> m_pFormatter{};
	bool m_allSelected;
	int m_linesPerRow;
	ULONGLONG m_mouseOverScrollAreaStart{};
	bool m_timerToHideScrollAreaSet{};
	CGdiImageDrawer m_groupFolder;
	CGdiImageDrawer m_dontDeleteImage;
	CGdiImageDrawer m_inFolderImage;
	CGdiImageDrawer m_shortCutImage;
	CGdiImageDrawer m_stickyImage;
	int m_rowHeight;
	CString m_searchText;
	BOOL m_showIfClipWasPasted;
	CAccels* m_pToolTipActions;
	CRichEditCtrlEx m_rtfFormater;
	CDPI* m_windowDpi;
	/** @brief Draws the row texts with highlighted search matches; keeps its colour stack between rows. */
	HtmlTextDrawer m_htmlTextDrawer{};


	// Generated message map functions
protected:
	//{{AFX_MSG(CQListCtrl)
	afx_msg void OnKeydown(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnCustomdrawList(NMHDR* pNMHDR, LRESULT* pResult);
	void DrawCopiedColorCode(CString& csText, CRect& rcText, CDC* pDC);
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnSelectionChange(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	afx_msg BOOL OnToolTipText(UINT id, NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void MeasureItem(LPMEASUREITEMSTRUCT lpMeasureItemStruct);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);

private:
	// ShowFullDescription's clip data line from the clip's Main row: added and last used dates, never
	// auto delete, quick paste text, shortcut and sticky state
	static CString ClipDataText(CppSQLite3Query& q);

	/** @brief A colour found in a copied clip's text, with its opacity. */
	struct CopiedColor
	{
		/** @brief The colour. */
		COLORREF color{};
		/** @brief The opacity, 0 (transparent) to 255 (opaque). */
		int alpha{ 255 };
	};

	/** @brief The scan state that the plain RGB parsers share, in the order they run. */
	struct RgbScan
	{
		/** @brief The red value scanned last. */
		int r{};
		/** @brief The green value scanned last. */
		int g{};
		/** @brief The blue value scanned last. */
		int b{};
		/** @brief The characters consumed by the last scan that reached its %n. */
		int charsConsumed{};
	};

	/** @brief The first three numbers of a CSS colour function. */
	struct CssValues
	{
		/** @brief The first number (red, hue or lightness). */
		double first{};
		/** @brief The second number (green, saturation or chroma). */
		double second{};
		/** @brief The third number (blue, lightness or hue). */
		double third{};
	};

	/** @brief The background and text colours of a list row. */
	struct RowColors
	{
		/** @brief The background colour. */
		COLORREF background{};
		/** @brief The text colour. */
		COLORREF text{};
	};

	/** @brief A parser of one colour notation: (text in lower case, colour found) -> found. */
	using ColorParser = bool (CQListCtrl::*)(const CString&, CopiedColor&);

	/**
	 * @brief Draws one list row (OnCustomdrawList's item stage).
	 * @param pLVCD the custom draw data of the row.
	 */
	void DrawListItem(NMLVCUSTOMDRAW* pLVCD);
	/**
	 * @brief Reads a row's text and splits off its symbols (the part before the first '|').
	 * @param nItem the row.
	 * @param csText receives the text without the symbols.
	 * @param strSymbols receives the symbols; empty when the text has none.
	 */
	void ReadItemText(int nItem, CString& csText, CString& strSymbols);
	/**
	 * @brief The colours of a row by its selection, the list's focus and the row's parity.
	 * @param nItem the row.
	 * @param state the row's LVIS state.
	 * @param bListHasFocus whether the list has the focus.
	 * @return the background and text colours.
	 */
	RowColors ChooseRowColors(int nItem, UINT state, BOOL bListHasFocus);
	/**
	 * @brief Whether the row shows the "clip was pasted" marker.
	 * @param strSymbols the row's symbols.
	 * @return true when the option is on and the clip was pasted from Ditto.
	 */
	bool IsPastedClip(const CString& strSymbols) const;
	/**
	 * @brief Draws the "clip was pasted" marker at the left edge of the row.
	 * @param pDC the device context.
	 * @param rcItem the row rectangle.
	 */
	void DrawPastedMarker(CDC* pDC, const CRect& rcItem);
	/**
	 * @brief Whether the row's first ten hot key number is shown.
	 * @param firstTenNum the row's first ten number (-1: none).
	 * @return true when the option is on and the row is one of the first ten.
	 */
	bool ShowsFirstTenHotKey(int firstTenNum) const;
	/**
	 * @brief Whether the "in group" icon is drawn (not when the list shows a group's clips).
	 * @param strSymbols the row's symbols.
	 * @return false inside a group for a clip that is in a group, else true.
	 */
	static bool ShouldDrawInGroupIcon(const CString& strSymbols);
	/**
	 * @brief Draws the symbol icons of a row (group, never auto delete, shortcut, in group, sticky).
	 * @param pDC the device context.
	 * @param rcText the text rectangle; its left moves past each icon.
	 * @param strSymbols the row's symbols.
	 * @param drawInGroupIcon whether the "in group" icon is drawn.
	 */
	void DrawSymbolIcons(CDC* pDC, CRect& rcText, const CString& strSymbols, bool drawInGroupIcon);
	/**
	 * @brief Draws one symbol icon and moves the text past it.
	 * @param pDC the device context.
	 * @param rcText the text rectangle; its left moves past the icon.
	 * @param image the icon.
	 */
	void DrawSymbolIcon(CDC* pDC, CRect& rcText, CGdiImageDrawer& image);
	/**
	 * @brief Draws a row's text: the RTF thumbnail, else the text with the search matches highlighted.
	 * @param nItem the row.
	 * @param csText the text (the highlight tags are inserted into it).
	 * @param rcText the text rectangle.
	 * @param pDC the device context.
	 */
	void DrawItemText(int nItem, CString& csText, CRect& rcText, CDC* pDC);
	/**
	 * @brief Inserts the search highlight tags around the search matches.
	 * @param csText the text.
	 * @return true when there is a search text and it was found.
	 */
	bool HighlightSearchMatches(CString& csText);
	/**
	 * @brief Draws a row's first ten hot key number and the line beside it.
	 * @param pDC the device context.
	 * @param rcItem the row rectangle.
	 * @param firstTenNum the row's first ten number.
	 */
	void DrawFirstTenHotKey(CDC* pDC, const CRect& rcItem, int firstTenNum);

	/**
	 * @brief Finds a colour in a clip's text, trying each notation in turn.
	 * @param parseText the cleaned text in lower case.
	 * @param color receives the colour found.
	 * @return true when a notation matched.
	 */
	bool ParseCopiedColor(const CString& parseText, CopiedColor& color);
	/**
	 * @brief Parses a W3C colour name.
	 * @param parseText the cleaned text in lower case.
	 * @param color receives the colour found.
	 * @return true when the text is a colour name.
	 */
	bool ParseNamedColor(const CString& parseText, CopiedColor& color);
	/**
	 * @brief Parses a hex colour with a # or 0x prefix (3, 4, 6 or 8 digits).
	 * @param parseText the cleaned text in lower case.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	bool ParsePrefixedHexColor(const CString& parseText, CopiedColor& color);
	/**
	 * @brief Removes the # or 0x prefix of a hex colour.
	 * @param parseText the text.
	 * @return the text without its prefix.
	 */
	static CString StripHexPrefix(const CString& parseText);
	/**
	 * @brief Expands shorthand hex digits (3 or 4 digits: each digit doubled).
	 * @param hexString the digits.
	 * @return the expanded digits; other lengths are returned unchanged.
	 */
	static CString ExpandShorthandHex(const CString& hexString);
	/**
	 * @brief Scans 6 (RRGGBB) or 8 (RRGGBBAA) hex digits.
	 * @param hexString the digits.
	 * @param color receives the colour.
	 * @return true when the digits were scanned.
	 */
	static bool ScanHexColor(const CString& hexString, CopiedColor& color);
	/**
	 * @brief Whether the text is a call of a CSS colour function such as rgb(...).
	 * @param parseText the text.
	 * @param name the function name prefix.
	 * @return true when the text starts with the name and ends with ')'.
	 */
	static bool IsCssFunction(const CString& parseText, const CString& name);
	/**
	 * @brief Splits CSS function arguments at spaces, commas and slashes.
	 * @param content the text between the parentheses.
	 * @return the arguments.
	 */
	static std::vector<CString> SplitCssArguments(CString content);
	/**
	 * @brief Parses the first three CSS arguments.
	 * @param tokens the arguments.
	 * @param values receives the numbers.
	 * @return true when there are three arguments and all are numbers.
	 */
	static bool ParseCssValues(const std::vector<CString>& tokens, CssValues& values);
	/**
	 * @brief The opacity from the fourth CSS argument (a number 0-1 or a percentage).
	 * @param tokens the arguments.
	 * @return the opacity 0-255; 255 when there is no valid fourth argument.
	 */
	static int CssAlpha(const std::vector<CString>& tokens);
	/**
	 * @brief Whether a number is a colour channel value 0-255.
	 * @param value the number.
	 * @return true when it is in range.
	 */
	static bool IsByteValue(double value);
	/**
	 * @brief Whether a number is a percentage 0-100.
	 * @param value the number.
	 * @return true when it is in range.
	 */
	static bool IsPercentValue(double value);
	/**
	 * @brief Whether a channel value is 0-255.
	 * @param value the value.
	 * @return true when it is in range.
	 */
	static bool IsRgbByte(int value);
	/**
	 * @brief Parses rgb(...) or rgba(...).
	 * @param parseText the cleaned text in lower case.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	bool ParseCssRgbColor(const CString& parseText, CopiedColor& color);
	/**
	 * @brief Parses hsl(...) or hsla(...).
	 * @param parseText the cleaned text in lower case.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	bool ParseCssHslColor(const CString& parseText, CopiedColor& color);
	/**
	 * @brief Parses oklch(...).
	 * @param parseText the cleaned text in lower case.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	bool ParseCssOklchColor(const CString& parseText, CopiedColor& color);
	/**
	 * @brief Parses the non-W3C notations: (r, g, b), "r, g, b", "r g b" and RRGGBB.
	 * @param parseText the cleaned text in lower case.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	bool ParsePlainRgbColor(const CString& parseText, CopiedColor& color);
	/**
	 * @brief Parses a parenthesized RGB: "(255, 128, 0)" or "(255 128 0)".
	 * @param parseText the cleaned text in lower case.
	 * @param scan the shared scan state.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	static bool ParseParenthesizedRgb(const CString& parseText, RgbScan& scan, CopiedColor& color);
	/**
	 * @brief Parses a comma-separated RGB: "255, 0, 0".
	 * @param parseText the cleaned text in lower case.
	 * @param scan the shared scan state.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	static bool ParseCommaSeparatedRgb(const CString& parseText, RgbScan& scan, CopiedColor& color);
	/**
	 * @brief Parses a space-separated RGB: "255 128 0".
	 * @param parseText the cleaned text in lower case.
	 * @param scan the shared scan state.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	static bool ParseSpaceSeparatedRgb(const CString& parseText, RgbScan& scan, CopiedColor& color);
	/**
	 * @brief Parses 6 hex digits without a prefix: "FF00CC".
	 * @param parseText the cleaned text in lower case.
	 * @param scan the shared scan state.
	 * @param color receives the colour found.
	 * @return true when the text is such a colour.
	 */
	bool ParseSixDigitHex(const CString& parseText, RgbScan& scan, CopiedColor& color);
	/**
	 * @brief Takes the scanned RGB values when all are 0-255.
	 * @param scan the scanned values.
	 * @param color receives the opaque colour.
	 * @return true when the values are in range.
	 */
	static bool AcceptRgb(const RgbScan& scan, CopiedColor& color);
	/**
	 * @brief Draws the colour box at the left of the text (a checkerboard behind a translucent colour).
	 * @param pDC the device context.
	 * @param rcText the text rectangle; its left moves past the box.
	 * @param color the colour.
	 */
	void DrawColorBox(CDC* pDC, CRect& rcText, const CopiedColor& color);

	/**
	 * @brief Runs a matching accelerator (copy buffer, open clip or move to group).
	 * @param pMsg the message.
	 * @return true when an accelerator matched.
	 */
	bool RunAccelerator(MSG* pMsg);
	/**
	 * @brief Runs the command of a matched accelerator.
	 * @param a the accelerator.
	 */
	void RunAcceleratorCommand(const CAccel& a);
	/**
	 * @brief Lets the list scroll on the mouse wheel and asks the parent to update its scroll bar.
	 * @param pMsg the message.
	 * @return the list's PreTranslateMessage result.
	 */
	BOOL PreTranslateMouseWheel(MSG* pMsg);
	/** @brief Selects every row (Ctrl+A). */
	void SelectAllItems();
	/** @brief Home: selects up to the anchor with Shift, else moves to the first row. */
	void HandleHomeKey();

	/**
	 * @brief Whether the description window already shows the clip.
	 * @param clipId the clip.
	 * @return true when the window is valid, open and shows the clip.
	 */
	bool IsToolTipShowingClip(int clipId);
	/**
	 * @brief Where the description window opens.
	 * @param nItem the row of the clip.
	 * @param bFromAuto whether the window opens on its own (then centred under the row).
	 * @return the remembered position, else below the row.
	 */
	CPoint DescriptionPosition(int nItem, bool bFromAuto);
	/**
	 * @brief Creates the description window, or clears the open one for a new clip.
	 * @param fromNextPrev whether the user moved to the next or previous clip (the window stays).
	 * @param pt the window position; the open window's position when fromNextPrev.
	 */
	void PrepareToolTipWindow(bool fromNextPrev, CPoint& pt);
	/**
	 * @brief Fills the description window with the clip and shows it.
	 * @param nItem the row of the clip.
	 * @param clipId the clip.
	 * @param clipRow the row of the clip.
	 * @param csDescription the row's description, shown when the clip has no text.
	 * @param pt the window position.
	 * @return false when the clip's data cannot be loaded.
	 */
	bool ShowClipInToolTip(int nItem, int clipId, int clipRow, const CString& csDescription, CPoint pt);
	/**
	 * @brief Sets the clip, actions, search text and font of the description window and clears its texts.
	 * @param clipId the clip.
	 * @param clipRow the row of the clip.
	 */
	void ResetToolTipContent(int clipId, int clipRow);
	/**
	 * @brief Loads the clip's data line and folder into the description window.
	 * @param clipId the clip.
	 * @return false (after showing the error) when the database query fails.
	 */
	bool LoadToolTipClipData(int clipId);
	/**
	 * @brief Puts the clip's Unicode or ANSI text (else the description) into the description window.
	 * @param nItem the row of the clip.
	 * @param Clip the format buffer.
	 * @param csDescription the row's description.
	 */
	void SetToolTipPlainText(int nItem, CClipFormat& Clip, const CString& csDescription);
	/**
	 * @brief Puts the clip's RTF into the description window.
	 * @param nItem the row of the clip.
	 * @param Clip the format buffer.
	 */
	void SetToolTipRtf(int nItem, CClipFormat& Clip);

	/** @brief Updates the parent's scroll bar, the description and the status for a new selection. */
	void NotifySelectionChanged();
	/** @brief Notifies the parent when all rows become selected, and tracks when that ends. */
	void UpdateAllSelectedState();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_QLISTCTRL_H__30BEB04A_4B97_4943_BB73_C5128E66B4ED__INCLUDED_)

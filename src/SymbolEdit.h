/************************************************************************/
/* Created by MARIUS BANCILA
/*            www.mariusbancila.ro
/* Copyright (C) 2008 MARIUS BANCILA. All rights reserved.
/*
/* Permission is given by the author to freely redistribute and
/* include this code in any program as long as this credit is
/* given where due.
/*
/* CODE IS PROVIDED UNDER THIS LICENSE ON AN "AS IS" BASIS,
/* WITHOUT WARRANTY OF ANY KIND, EITHER EXPRESSED OR IMPLIED,
/* INCLUDING, WITHOUT LIMITATION, WARRANTIES THAT THE CODE
/* IS FREE OF DEFECTS, MERCHANTABLE, FIT FOR A PARTICULAR PURPOSE
/* OR NON-INFRINGING. THE ENTIRE RISK AS TO THE QUALITY AND
/* PERFORMANCE OF THE CODE IS WITH YOU. SHOULD ANY
/* CODE PROVE DEFECTIVE IN ANY RESPECT, YOU (NOT THE INITIAL
/* DEVELOPER OR ANY OTHER CONTRIBUTOR) ASSUME THE COST OF ANY
/* NECESSARY SERVICING, REPAIR OR CORRECTION. THIS DISCLAIMER OF
/* WARRANTY CONSTITUTES AN ESSENTIAL PART OF THIS LICENSE. NO USE
/* OF ANY CODE IS AUTHORIZED HEREUNDER EXCEPT UNDER
/* THIS DISCLAIMER.
/*
/************************************************************************/



#pragma once

#include <memory>
#include <type_traits>

#include "GdiImageDrawer.h"
#include "Accels.h"
#include "DPI.h"

class CGetSetOptions;

// CSymbolEdit

class CSymbolEdit : public CEdit
{
	DECLARE_DYNAMIC(CSymbolEdit)

	/** @brief Destroys an icon handle: the deleter of m_hSymbolIcon. */
	struct IconDestroyer
	{
		/**
		 * @brief Destroys the icon.
		 * @param hIcon the icon handle.
		 */
		void operator()(HICON hIcon) const { ::DestroyIcon(hIcon); }
	};

	CFont m_fontPrompt;
	/** @brief The symbol icon (owned, also when handed in by SetSymbolIcon(HICON)); empty when there is none. */
	std::unique_ptr<std::remove_pointer_t<HICON>, IconDestroyer> m_hSymbolIcon{};
	CString m_strPromptText;
	COLORREF m_colorPromptText;

	CBrush m_brush;
	COLORREF m_lastBrushColor{CLR_INVALID};

	void DestroyIcon();

	/** @brief Handles Ctrl + Z, X, C, V and A before the edit control gets them.
	 *  @param pMsg The WM_KEYDOWN message.
	 *  @return true if the key was handled. */
	bool HandleControlKey(MSG* pMsg);

	/** @brief Ctrl + C: copies the selection, or asks the parent to copy the clip when nothing is selected.
	 *  @param pMsg The WM_KEYDOWN message. */
	void CopySelectionOrClip(const MSG* pMsg);

	/** @brief Sends a message with the key's wParam and lParam to the parent window.
	 *  @param message Message to send.
	 *  @param pMsg The key message.
	 *  @return true if there is a parent and the message was sent. */
	bool SendKeyToParent(UINT message, const MSG* pMsg);

	/** @brief Tells if the key state opens the search history menu with the down key.
	 *  @return true if Ctrl (or Ctrl + Shift) is down. */
	static bool IsHistoryMenuKeyState();

	/** @brief Tells if the key moves the selection in the clip list.
	 *  @param key Virtual key code.
	 *  @return true for down, up, page up and page down. */
	static bool IsListNavigationKey(WPARAM key);

	/** @brief Handles return, down, list navigation and delete keys.
	 *  @param pMsg The WM_KEYDOWN message.
	 *  @return true if the key was handled. */
	bool HandleKeyDown(MSG* pMsg);

	/** @brief Return key: starts the search in the parent and adds the text to the search history. */
	void HandleReturnKey();

	/** @brief Delete key: with the caret at the end of the text, asks the parent to delete the selected clip.
	 *  @param pMsg The WM_KEYDOWN message.
	 *  @return true if the message was sent to the parent. */
	bool HandleDeleteKey(const MSG* pMsg);

	/** @brief Draws the symbol icon and makes room for it.
	 *  @param dc Paint DC.
	 *  @param rect Client rectangle; made smaller by the icon.
	 *  @param margins Edit control margins (GetMargins). */
	void DrawSymbolIcon(CDC& dc, CRect& rect, DWORD margins);

	/** @brief Fills the text area and draws the text when focused or not empty.
	 *  @param dc Paint DC.
	 *  @param rect Area to fill.
	 *  @param textRect Area of the text.
	 *  @param text Window text. */
	void DrawTextArea(CDC& dc, const CRect& rect, const CRect& textRect, const CString& text);

	/**
	 * @brief The application's settings.
	 * @return theApp.Services().Settings().
	 */
	CGetSetOptions& Settings() const;

	/** @brief Draws the prompt text in an empty edit control.
	 *  @param dc Paint DC.
	 *  @param textRect Area of the text (widened for the prompt). */
	void DrawPromptText(CDC& dc, CRect textRect);

	/** @brief Draws the search history button (focused or not empty) and the clear button (not empty)
	 *         and sets their hit rectangles (empty when not shown).
	 *  @param dc Paint DC.
	 *  @param rect Client rectangle.
	 *  @param text Window text. */
	void DrawButtons(CDC& dc, const CRect& rect, const CString& text);

public:
	CSymbolEdit();
	virtual ~CSymbolEdit();

	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void AddToSearchHistory();

	bool ShowSearchHistoryMenu();

	void SetSymbolIcon(HICON hIcon, BOOL redraw = TRUE);
	void SetSymbolIcon(UINT id, BOOL redraw = TRUE);

	void SetPromptText(CString text, BOOL redraw = TRUE);
	void SetPromptText(LPCTSTR szText, BOOL redraw = TRUE);

	void SetPromptTextColor(COLORREF color, BOOL redraw = TRUE);

	void SetPromptFont(CFont& font, BOOL redraw = TRUE);
	void SetPromptFont(const LOGFONT* lpLogFont, BOOL redraw = TRUE);

	bool ApplyLastSearch();

	void SetLastSearchAccel(CAccel a) { m_lastSearchShortCut = a; }

	CString SavePastSearches();
	void LoadPastSearches(CString values);

	/**
	 * @brief Sets the window's DPI and loads the buttons' images for it; until then the edit paints itself.
	 * @param dpi The DPI of the window (not owned; must outlive this control).
	 * @throws std::invalid_argument when dpi is null.
	 */
	void SetDpiInfo(CDPI *dpi);

	/**
	 * @brief Reloads the buttons for the current DPI (SetDpiInfo must have been called).
	 * @throws std::invalid_argument when SetDpiInfo was not called yet.
	 */
	void OnDpiChanged();

	//void SetWindowTextEx(LPCSTR)

protected:
	
	//CGdiImageDrawer m_searchButton;
	CGdiImageDrawer m_closeButton;
	CRect m_closeButtonRect;
	bool m_mouseDownOnClose;
	bool m_mouseHoveringOverClose;

	CGdiImageDrawer m_searchesButton;
	CRect m_searchesButtonRect;
	bool m_mouseDownOnSearches;
	bool m_mouseHoveringOverSearches;

	CAccel m_lastSearchShortCut;

	CStringArray m_searches;

	/** @brief The command ids of the search history menu. */
	enum : UINT
	{
		/** @brief The command id of the first past search; the next ones follow it. */
		IdRangeStart = 3000,
		/** @brief The command id of "Clear List". */
		IdClearList = 3010,
	};
	/** @brief The most past searches the history keeps and shows. */
	static constexpr int s_listMaxCount{10};
	/** @brief The most characters of a search the history keeps. */
	static constexpr int s_maxSavedSearchLength{50};

	void RecalcLayout();
	virtual void PreSubclassWindow();

	CDPI *m_windowDpi;

	int m_centerTextDiff;
	CString m_lastTextOnPaint;

	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnSetFont(WPARAM wParam, LPARAM lParam);
	//afx_msg LRESULT OnMenuExit(WPARAM wParam, LPARAM lParam);
	afx_msg HBRUSH CtlColor(CDC* pDC, UINT n);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnKillFocus(CWnd* pNewWnd);
	afx_msg BOOL OnSetCursor(CWnd* pWnd, UINT nHitTest, UINT message);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnSelectSearchString(UINT idIn);

	DECLARE_MESSAGE_MAP()

public:
	afx_msg void OnPaint();
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS* lpncsp);
	afx_msg void OnNcPaint();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};



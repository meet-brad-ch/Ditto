#pragma once

#include <array>
#include <memory>

#include "RichEditCtrlEx.h"
#include "WndEx.h"
#include "DittoWindow.h"
#include "GdipButton.h"
#include "ImageViewer.h"
#include "GroupStatic.h"
#include "Accels.h"
#include "SnapWindow.h"

class CGetSetOptions;

class CToolTipEx : public CWnd
{
// Construction
public:
	CToolTipEx();

private:
	/** @brief The application settings (theApp's services; this window is created by the framework).
	@return the settings. */
	CGetSetOptions& Settings() const;

// Attributes
public:

// Operations
public:
	BOOL OnMsg(MSG* pMsg);
	BOOL Create(CWnd* pParentWnd);
	BOOL Show(CPoint point);
	BOOL Hide();
	void SetToolTipText(const CString &csText);
	void SetRTFText(const CStringA &rtf);
	/**
	 * @brief Shows an image (replacing the previous one) and takes ownership of it.
	 * @param gdiplusBitmap the image; empty to show none.
	 */
	void SetGdiplusBitmap(std::unique_ptr<Gdiplus::Bitmap> gdiplusBitmap);
	void SetNotifyWnd(CWnd *pNotify)		{ m_pNotifyWnd = pNotify;	}
	void HideWindowInXMilliSeconds(long lms);
	CRect GetBoundsRect();

	void SetClipId(int clipId) { m_clipId = clipId; }
	int GetClipId() { return m_clipId; }

	void SetClipRow(int clipRow) { m_clipRow = clipRow; }
	int GetClipRow() { return m_clipRow; }

	void SetSearchText(CString text) { m_searchText = text; }

	void SetClipData(CString data) { m_clipData = data; m_originalClipData = data; }
	void SetFolderPath(CString path) { m_folderPath = path; }

	bool GetShowPersistant() { return m_showPersistant; }
	void ToggleShowPersistant() { OnFirstAlwaysontop(); }
	bool ToggleWordWrap();
	void SetTooltipActions(CAccels *pToolTipActions) { m_pToolTipActions = pToolTipActions; }

	void GetWindowRectEx(LPRECT lpRect);

	void UpdateMenuShortCut(CMenu *subMenu, int id, DWORD action);

	void DoSearch();
	void MoveControls();

	BOOL SetLogFont(LPLOGFONT lpLogFont, BOOL bRedraw /*=TRUE*/);


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CToolTipEx)
	protected:
	virtual void PostNcDestroy();
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CToolTipEx();
	
protected:
	DWORD m_dwTextStyle;
	CRect m_rectMargin;
	CString m_csText;
	CFont m_Font;
	int m_fontHeight{};
	CStringA m_csRTF;
	CRichEditCtrlEx m_RichEdit;
	CWnd *m_pNotifyWnd;
	CGdipButton m_optionsButton;
	int m_clipId;
	CString m_searchText;
	CScrollBar m_vScroll;
	CScrollBar m_hScroll;
	CDittoWindow m_DittoWindow;
	CImageViewer m_imageViewer;
	CGroupStatic m_clipDataStatic;
	CGroupStatic m_folderPathStatic;
	CString m_clipData;
	CString m_originalClipData;
	CFont m_clipDataFont;
	bool m_saveWindowLockout{};
	int m_clipRow;
	bool m_showPersistant;
	CAccels *m_pToolTipActions;
	bool m_bMaxSetTimer;
	int m_lDelayMaxSeconds;
	SnapWindow m_snap;
	CString m_folderPath;
	bool m_showingText;
	bool m_showingRTF;
	bool m_showingImage{};

protected:
	CString GetFieldFromString(CString ref, int nIndex, TCHAR ch);	
	BOOL IsCursorInToolTip();
	void HighlightSearchText();	
	void ApplyWordWrap();
	void SaveWindowSize();

	// Generated message map functions
protected:
	//{{AFX_MSG(CToolTipEx)	
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg LRESULT OnNcHitTest(CPoint point);
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnNcMouseMove(UINT nHitTest, CPoint point);
	afx_msg void OnNcLButtonUp(UINT nHitTest, CPoint point); 
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point); 
	afx_msg void OnNcCalcSize(BOOL bCalcValidRects, NCCALCSIZE_PARAMS FAR* lpncsp); 
	afx_msg void OnNcPaint();
	afx_msg void OnOptions();
	afx_msg void OnWindowPosChanging(WINDOWPOS* lpwndpos);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnRememberwindowposition();
	afx_msg void OnSizewindowtocontent();
	afx_msg void OnScaleimagestofitwindow();
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnSetFocus(CWnd* pOldWnd);
	afx_msg void OnPaint();	
	afx_msg void OnFirstHidedescriptionwindowonm();
	afx_msg void OnFirstWraptext();
	afx_msg void OnNcLButtonDblClk(UINT nHitTest, CPoint point);
	afx_msg void OnFirstAlwaysontop();
	void OnEnMsgfilterRichedit21(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg LRESULT OnDpiChanged(WPARAM wParam, LPARAM lParam);
	afx_msg void OnMoving(UINT fwSide, LPRECT pRect);
	afx_msg void OnEnterSizeMove();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	afx_msg LRESULT OnRefreshFooter(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFirstViewtext();
	afx_msg void OnFirstViewrtf();
	afx_msg void OnFirstViewImage();
	afx_msg void OnUpdateFirstViewtext(CCmdUI* pCmdUI);
	afx_msg void OnUpdateFirstViewrtf(CCmdUI* pCmdUI);

private:
	/** @brief The number of text lines and the longest line, for sizing the window to its text. */
	struct TextLines
	{
		/** @brief The number of lines (counted up to 100). */
		int count{};
		/** @brief The longest line. */
		CString longest{};
	};

	/** @brief The clicks that hide the window unless it is shown persistently. */
	static constexpr std::array<UINT, 10> s_hidingClickMessages{
		WM_LBUTTONDBLCLK, WM_RBUTTONDBLCLK, WM_MBUTTONDOWN, WM_MBUTTONDBLCLK,
		WM_NCLBUTTONDOWN, WM_NCLBUTTONDBLCLK, WM_NCRBUTTONDOWN, WM_NCRBUTTONDBLCLK,
		WM_NCMBUTTONDOWN, WM_NCMBUTTONDBLCLK };

	/** @brief The keys that OnMsg leaves to the list (the window stays open). */
	static constexpr std::array<WPARAM, 7> s_listKeys{
		VK_CONTROL, VK_SHIFT, VK_UP, VK_DOWN, VK_NEXT, VK_PRIOR, VK_DELETE };

	/**
	 * @brief The window rectangle from the saved description window size.
	 * @param point the top left corner.
	 * @return the rectangle, kept on the screen.
	 */
	CRect RectFromSavedSize(CPoint point);
	/**
	 * @brief The window rectangle sized to the text or image, at the point and within its monitor.
	 * @param point the top left corner.
	 * @return the rectangle.
	 */
	CRect RectSizedToContent(CPoint point);
	/** @brief Shows the image viewer or the rich edit, and records which content is shown. */
	void ShowContentWindow();
	/**
	 * @brief Counts the text lines and finds the longest.
	 * @return the line count and the longest line.
	 */
	TextLines MeasureTextLines();

	/**
	 * @brief Ctrl+C: copies the rich edit's selection.
	 * @param pMsg the key down message.
	 * @return true when the key was Ctrl+C.
	 */
	bool HandleCopyKey(MSG* pMsg);
	/**
	 * @brief Shows the options menu on a right click in the text or the image.
	 * @return true when the menu was shown.
	 */
	bool HandleContentRButtonDown();
	/** @brief Gives the focus back to the parent after a left click outside the text and the options button. */
	void FocusParentAfterLButtonUp();
	/**
	 * @brief Lets the description window's actions see the message.
	 * @param pMsg the message.
	 */
	void CheckToolTipActions(MSG* pMsg);

	/** @brief Hides the window on a mouse click outside it (when the option is on and the window is not persistent). */
	void HideOnMouseClick();
	/** @brief Hides the window unless it is shown persistently. */
	void HideUnlessPersistent();
	/**
	 * @brief Whether a message is a click that hides the window.
	 * @param message the message id.
	 * @return true for the clicks in s_hidingClickMessages.
	 */
	static bool IsHidingClick(UINT message);
	/**
	 * @brief Whether a key is left to the list.
	 * @param vk the virtual key.
	 * @return true for the keys in s_listKeys.
	 */
	static bool IsListKey(WPARAM vk);
	/**
	 * @brief Handles a key while the window is visible: Tab focuses the text, other keys hide it.
	 * @param vk the virtual key.
	 * @return TRUE when the key was handled (Tab), else FALSE.
	 */
	BOOL OnMsgKeyDown(WPARAM vk);
	/**
	 * @brief Forwards a mouse wheel message to the image viewer or the rich edit.
	 * @param pMsg the message.
	 */
	void ForwardMouseWheel(MSG* pMsg);

	/** @brief The timer ids of the window (SetTimer / OnTimer). */
	enum : UINT
	{
		/** @brief Hides the window after its delay. */
		TimerHideWindow = 1,
		/** @brief Saves the window size once resizing settled. */
		TimerSaveSize = 2,
		/** @brief Finishes a caption click once the mouse button is up. */
		TimerButtonUp = 3,
		/** @brief Restores the minimized window when the mouse is still over its caption. */
		TimerAutoMax = 4,
	};

	/** @brief TimerButtonUp: finishes a caption click once the mouse button is up. */
	void OnButtonUpTimer();
	/** @brief TimerAutoMax: restores the minimized window when the mouse is still over its caption. */
	void OnAutoMaxTimer();

	/**
	 * @brief Checks the menu items of the description options.
	 * @param cmSubMenu the options menu.
	 */
	void CheckOptionMenuItems(CMenu* cmSubMenu);
	/**
	 * @brief Checks the shown view (text, RTF, image) and disables the views without content.
	 * @param cmSubMenu the options menu.
	 */
	void CheckViewMenuItems(CMenu* cmSubMenu);
};

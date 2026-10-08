#pragma once

#include <afxtempl.h>
#include "DPI.h"

typedef struct tagNMTABCHANGE{
    NMHDR hdr;
    long lOldTab;
    long lNewTab;
	long lOldItemData;
	long lNewItemData;
} NMTABCHANGE, FAR *LPNMTABCHANGE;

/////////////////////////////////////////////////////////////////////////////
// CTabCtrlEx window

class CTabCtrlEx : public CWnd
{
public:
	/** @brief The sheet styles: the low word of Create's dwStyle. */
	enum : int
	{
		/** @brief No sheet style. */
		ScsSheet = 0x0000,
		/** @brief Tab style. */
		ScsTab = 0x0001,
		/** @brief The tabs are at the top. */
		ScsTop = 0x0002,
		/** @brief The tabs are at the bottom. */
		ScsBottom = 0x0004,
		/** @brief The selected tab's text is bold. */
		ScsBold = 0x0008,
		/** @brief The sheet has a border (set from WS_BORDER). */
		ScsBorder = 0x0010,
	};

	/** @brief The WM_NOTIFY code sent to the parent when the active tab changes (NMTABCHANGE). */
	enum : UINT
	{
		/** @brief The active tab changed. */
		SnSetActiveTab = WM_APP + 1122,
	};

// Construction
public:
	CTabCtrlEx();

public:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTabCtrlEx)
	public:
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CTabCtrlEx();

	// Create the sheet control
	virtual BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

	// Add a new window to the sheet. Returns the index of the new tab, -1 on error
	bool AddItem(const CString& csTabTitle, CWnd* pTabWnd);

	// Insert a new window in the sheet. Returns the index of the new tab, -1 on error
	bool InsertItem(int nTab, const CString& csTabTitle, CWnd* pTabWnd);
	
	//Replaces the window in nTab position -- Calls HideWindow for the old and ShowWindow for the new
	bool ReplaceItem(int nTab, const CString &csTabTitle, CWnd* pTabWnd);

	// Delete a tab in the sheet
	bool DeleteItem(int nTab);

	// Delete all tabs in the sheet
	bool DeleteAllItems();
	
	// Get/Set the tab title
	CString GetTabTitle(int nTab);
	bool SetTabTitle(int nTab, const CString& csTabTitle);

	// Get the active sheet
	int GetActiveTab();

	// Get the number of tabs
	int GetTabCount();

	// Set the active tab and make the tab title visible
	void SetActiveTab(int nTab, bool bNotify = true);

	bool SetTabItemData(int nTab, long lItemData);
	long GetTabItemData(int nTab);

	// Make a tab title visible
	void MakeTabVisible(int nTab);
	
	// Set the tab height
	void SetTabHeight(int nTabHeight);

	// Relay keyboard events for the sheet to process,
	// returns TRUE if the message was processed, DO NOT process this message
	// return FALSE if the message was not processed
	virtual BOOL PreTranslateMessage(MSG* pMsg);

	void SetFocusToNewlySelectedTab(bool bVal)	{ m_bSetFocusToNewlySelectedTab = bVal;	}
	bool GetFocusToNewlySelectedTab()			{ return m_bSetFocusToNewlySelectedTab;	}

	void SetTabColors(COLORREF Selected, COLORREF NonSelected)	{ m_SelectedColor = Selected; m_NonSelectedColor = NonSelected;}

	void OnDpiChanged(CWnd* pParent, int dpi);

protected:
	class CTab
	{
	public:
		CTab()
		{
			lItemData = -1;
		}
		CString csTitle;
		CWnd* pWnd{};
		long lWidth{};
		long lItemData;
	};

	typedef enum {ArrowLeft, ArrowRight} ButtonStyle;
	typedef enum {BtnDown, BtnUp/*, BtnHover*/} ButtonState;

	/** @brief Win32 values of the control: its timer id and the key it switches tabs with. */
	enum : UINT
	{
		/** @brief The timer that scrolls the tabs while a spinner button is held down. */
		IdScrollTimer = 0x1010,
		/** @brief The virtual key code of the tilde key (VK_OEM_3), which switches the tabs. */
		VkTilde = 0xC0,
	};
	/** @brief The space on each side of a tab title, in pixels. */
	static constexpr int s_textPad{7};
	/** @brief The window class name registered for the sheet control. */
	static constexpr const TCHAR* s_className{_T("_TabCtrlClass_")};
	/** @brief The window class style of the sheet control. */
	static constexpr UINT s_classStyle{CS_DBLCLKS};
	/** @brief The space left of the spinner buttons, in pixels. */
	static constexpr int s_spinPad{3};
	/** @brief How far one spinner step shifts the tabs, in pixels. */
	static constexpr int s_shiftUnits{10};
	/** @brief The light edge colour of a spinner button. */
	static constexpr COLORREF s_colorWhite{RGB(255, 255, 255)};
	/** @brief The dark edge colour of a spinner button. */
	static constexpr COLORREF s_colorDarkGray{RGB(64, 64, 64)};


protected:
	short m_nStyle;
	CArray <CTab, CTab&> m_Tabs;
	int m_nActiveTab;
	int m_nTabHeight;
	/** @brief Fills the tab bar and the spinner background (the non-selected tab colour). */
	CBrush m_brNonSelectedTab;
	CPen m_penGray;
	CPen m_penBlack;
	/** @brief The font of the inactive tabs; empty until SetTabHeight. */
	CFont m_fntText{};
	/** @brief The font of the active tab; empty until SetTabHeight. */
	CFont m_fntBoldText{};
	ButtonState m_btnState[2];
	bool m_bBtnEnabled[2];
	int m_nLeftShifted;
	int m_nPrevWidth;
	bool m_bSetFocusToNewlySelectedTab;
	COLORREF m_SelectedColor;
	COLORREF m_NonSelectedColor;
	CDPI m_dpi;

	
protected:
	int GetTextWidth(const CString& csText);
	int GetDisplayWidth();
	int GetTabsWidth();
	int GetSpinnerWidth();
	
	void GetFullRect(CRect& rcTab);
	void GetTabListRect(CRect& rcTab);
	void GetTabRect(int nTab, CRect& rcTab);
	void GetSpinnerRect(CRect& rcButton);
	void GetButtonRect(int nBtn, CRect& rcBtn);

	void EnableSpinners();
	void DrawTabs(CDC *pDC);
	void DrawBar(CDC* pDC);
	void DrawTab(int nTab, CDC *pDC, CRect& rcTab);
	void DrawTabEx(int nTab, CDC *pDC, CRect& rcTab);
	void DrawSpinner(CDC *pDC);
	void DrawButton(CDC *pDC, CRect& rcBtn, ButtonState btnState, ButtonStyle btnStyle, bool bEnable);
	void ScrollTab(CPoint point);
	
	void ActivateTab(int nTab, bool bNotify, bool bOnSize = false);
	void ResizeTabWindow(int nOldTab, int nNewTab, bool bNotify, bool bOnSize);
	void SwitchTabs(bool bNext = true);

	/** @brief Tells if a tab index is in range.
	 *  @param nTab Tab index.
	 *  @return true if 0 <= nTab < GetTabCount(). */
	bool IsValidTab(int nTab);

	/** @brief Tells if a window pointer is set and its window exists.
	 *  @param pWnd Window pointer (may be NULL).
	 *  @return true if pWnd is set and ::IsWindow says its handle is a window. */
	static bool IsLiveWindow(CWnd* pWnd);

	/** @brief Hides the window of the previous tab.
	 *  @param nOldTab Valid index of the previous tab.
	 *  @return The item data of the previous tab. */
	long HideTabWindow(int nOldTab);

	/** @brief Moves, shows and (if set) focuses the window of the new tab.
	 *  @param nNewTab Valid index of the new tab.
	 *  @param bOnSize true when called from OnSize (then the focus is not set).
	 *  @return The item data of the new tab. */
	long ShowTabWindow(int nNewTab, bool bOnSize);

	/** @brief Sends SnSetActiveTab to the parent window.
	 *  @param nOldTab Index of the previous tab.
	 *  @param nNewTab Index of the new tab.
	 *  @param lOldItemData Item data of the previous tab, -1 if none.
	 *  @param lNewItemData Item data of the new tab, -1 if none. */
	void NotifyTabChange(int nOldTab, int nNewTab, long lOldItemData, long lNewItemData);

	// Generated message map functions
protected:
	//{{AFX_MSG(CTabCtrlEx)
	afx_msg void OnPaint();
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnShowWindow(BOOL bShow, UINT nStatus);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};
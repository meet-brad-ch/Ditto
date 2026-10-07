#include "afxwin.h"
#if !defined(AFX_COPYPROPERTIES_H__129FE1CD_D305_487A_B88C_BB01CD9C1BB7__INCLUDED_)
#define AFX_COPYPROPERTIES_H__129FE1CD_D305_487A_B88C_BB01CD9C1BB7__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// CopyProperties.h : header file
//

#include "GroupCombo.h"
#include "RichEditCtrlEx.h"
#include "DialogResizer.h"
#include <array>

/////////////////////////////////////////////////////////////////////////////
// CCopyProperties dialog

class CCopyProperties : public CDialog
{
// Construction
public:
	CCopyProperties(long lCopyID, CWnd* pParent = NULL, CClip *pMemoryClip = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CCopyProperties)
	enum { IDD = IDD_COPY_PROPERTIES };
	CEdit	m_QuickPasteText;
	CEdit m_description;
	CGroupCombo	m_GroupCombo;
	CHotKeyCtrl	m_HotKey;
	CHotKeyCtrl	m_MoveToGrouHotKey;
	CListBox	m_lCopyData;
	CString	m_eDate;
	CString m_lastPasteDate;
	BOOL	m_bNeverAutoDelete;
	BOOL m_hotKeyGlobal{};
	BOOL m_moveToGroupHotKeyGlobal{};
	//}}AFX_DATA


// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CCopyProperties)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

public:
	bool m_bChangedText;
	long m_lGroupChangedTo;
	void SetHideOnKillFocus(bool bVal)	{ m_bHideOnKillFocus = bVal; }
	void SetHandleKillFocus(bool bVal)	{ m_bHandleKillFocus = bVal; }
	void SetToTopMost(bool bVal)		{ m_bSetToTopMost = bVal; }

// Implementation
protected:

	long m_lCopyID;
	ARRAY m_DeletedData;
	bool m_bDeletedData;
	bool m_bHideOnKillFocus;
	CDialogResizer m_Resize;
	bool m_bInGroup{};
	bool m_bHandleKillFocus;
	bool m_bSetToTopMost;
	CClip *m_pMemoryClip;
	CBrush m_brush;
	CClip m_clip;
	bool m_mouseDownOnCaption{};

	void LoadDataIntoCClip(CClip &Clip);
	void LoadDataFromCClip(CClip &Clip);
	BOOL CheckGlobalHotKey(CClip &clip);
	BOOL CheckMoveToGroupGlobalHotKey(CClip &clip);

	// Generated message map functions
	//{{AFX_MSG(CCopyProperties)
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	afx_msg void OnDeleteCopyData();
	virtual void OnCancel();
	afx_msg void OnActivate(UINT nState, CWnd* pWndOther, BOOL bMinimized);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
public:
	afx_msg HBRUSH OnCtlColor(CDC* pDC, CWnd* pWnd, UINT nCtlColor);
	afx_msg void OnLbnSelchangeCopyData();
	afx_msg void OnNcLButtonDown(UINT nHitTest, CPoint point);

private:
	// OnOK's work for a clip not yet saved: writes the dialog into it and removes the deleted formats
	void SaveToMemoryClip();
	// OnOK's work for a saved clip: writes the dialog into its Main row, registers its hot keys and
	// deletes the removed formats; false when the user cancels after a hot key error
	bool SaveToStoredClip();

	/** @brief The keys whose hot key control needs the extended-key flag (arrows, page keys, ...). */
	static constexpr std::array<BYTE, 12> s_extendedHotKeys{
		VK_LEFT, VK_UP, VK_RIGHT, VK_DOWN, // arrow keys
		VK_PRIOR, VK_NEXT, // page up and page down
		VK_END, VK_HOME, VK_INSERT, VK_DELETE,
		VK_DIVIDE, // numpad slash
		VK_NUMLOCK,
	};

	/**
	 * @brief Whether a key needs the extended-key flag in the hot key control.
	 * @param key The virtual key.
	 * @return True for the keys in s_extendedHotKeys.
	 */
	static bool IsExtendedHotKey(BYTE key);

	/**
	 * @brief LoadDataFromCClip's hot key step: shows the clip's hot key and move-to-group hot key.
	 * @param Clip The clip.
	 */
	void LoadHotKeys(CClip &Clip);

	/**
	 * @brief LoadDataFromCClip's format step: lists the clip's formats with their sizes, selecting the last.
	 * @param Clip The clip.
	 */
	void LoadFormatList(CClip &Clip);

	/**
	 * @brief Selects the last row of the format list, when it has one.
	 */
	void SelectLastFormat();

	/**
	 * @brief LoadDataFromCClip's step for a clip that is not a group: hides the move-to-group hot key
	 * controls and moves the controls below them up.
	 */
	void HideMoveToGroupHotKey();
};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_COPYPROPERTIES_H__129FE1CD_D305_487A_B88C_BB01CD9C1BB7__INCLUDED_)

#if !defined(AFX_SEARCHEDITBOX_H__E1864DC5_E60C_419F_A419_19F8222E42EC__INCLUDED_)
#define AFX_SEARCHEDITBOX_H__E1864DC5_E60C_419F_A419_19F8222E42EC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// SearchEditBox.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CSearchEditBox window

class CSearchEditBox : public CEdit
{
// Construction
public:
	CSearchEditBox();

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CSearchEditBox)
	public:
	virtual BOOL PreTranslateMessage(MSG* pMsg);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CSearchEditBox();

	// Generated message map functions
protected:
	//{{AFX_MSG(CSearchEditBox)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()

private:
	/** @brief Handles the return, list navigation, cut, copy and delete keys.
	 *  @param pMsg The WM_KEYDOWN message.
	 *  @return true if the key was handled. */
	bool HandleKeyDown(const MSG* pMsg);

	/** @brief Return key: asks the parent to search. */
	void HandleReturnKey();

	/** @brief Tells if the key moves the selection in the clip list.
	 *  @param key Virtual key code.
	 *  @return true for down, up and F3. */
	static bool IsListNavigationKey(WPARAM key);

	/** @brief Tells if the key is Ctrl + C, Ctrl + X or delete.
	 *  @param key Virtual key code.
	 *  @return true for those keys. */
	static bool IsCutCopyDeleteKey(WPARAM key);

	/** @brief Sends the key to the parent window as CQListCtrl::CbUpDown.
	 *  @param pMsg The key message.
	 *  @return true if there is a parent and the message was sent. */
	bool SendKeyToParent(const MSG* pMsg);
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_SEARCHEDITBOX_H__E1864DC5_E60C_419F_A419_19F8222E42EC__INCLUDED_)

#if !defined(AFX_CLIPBOARDVIEWER_H__67418FB6_6048_48FA_86D4_F412CACC41B1__INCLUDED_)
#define AFX_CLIPBOARDVIEWER_H__67418FB6_6048_48FA_86D4_F412CACC41B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#define TIMER_ENSURE_VIEWER_IN_CHAIN	6
#define TIMER_DRAW_CLIPBOARD			7
#define TIMER_PING						8

class CClipboardViewer : public CWnd
{
// Construction
public:
	CClipboardViewer(CCopyThread* pHandler);
	virtual ~CClipboardViewer();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CClipboardViewer)
	//}}AFX_VIRTUAL

// Implementation
public:
	void Create();

	bool	m_bPinging;
	bool	m_bConnect;
	bool	m_bIsConnected;
	bool	m_connectOnStartup;
	CString m_activeWindow;

	// m_pHandler->OnClipboardChange is called when the clipboard changes.
	CCopyThread*	m_pHandler;

	void Connect();    // starts listening for clipboard changes
	void Disconnect(bool bSendPing = true); // stops listening for clipboard changes

	void SendPing();

	bool GetConnect()				{ return m_bConnect; }
	void SetConnect(bool bConnect);
	void SetEnsureConnectedTimer();
	bool ValidActiveWnd();

	bool GetIgnoreClipboardChange();

	ULONGLONG m_dwLastCopy;

	// Generated message map functions
protected:
	//{{AFX_MSG(CClipboardViewer)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	//}}AFX_MSG
	afx_msg LRESULT OnSetConnect(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT CClipboardViewer::OnClipboardChange(WPARAM wParam, LPARAM lPara);
	DECLARE_MESSAGE_MAP()

private:
	void ProcessClipboardChange();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLIPBOARDVIEWER_H__67418FB6_6048_48FA_86D4_F412CACC41B1__INCLUDED_)

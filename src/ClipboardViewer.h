#if !defined(AFX_CLIPBOARDVIEWER_H__67418FB6_6048_48FA_86D4_F412CACC41B1__INCLUDED_)
#define AFX_CLIPBOARDVIEWER_H__67418FB6_6048_48FA_86D4_F412CACC41B1__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGetSetOptions;
class CRegisteredClipboardFormats;

class CClipboardViewer : public CWnd
{
	/** @brief The timer ids of the clipboard viewer (SetTimer / KillTimer / OnTimer). */
	enum : UINT_PTR
	{
		/** @brief Checks now and then that the viewer is still connected to the clipboard. */
		TimerEnsureViewerInChain = 6,
		/** @brief Handles a clipboard change after the configured delay. */
		TimerDrawClipboard = 7,
		/** @brief Waits for the answer to the ping. */
		TimerPing = 8,
	};

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

	bool m_bPinging;
	bool m_bConnect;
	bool m_bIsConnected;
	bool m_connectOnStartup;
	CString m_activeWindow;

	// m_pHandler->OnClipboardChange is called when the clipboard changes.
	CCopyThread* m_pHandler;

	void Connect();                         // starts listening for clipboard changes
	void Disconnect(bool bSendPing = true); // stops listening for clipboard changes

	void SendPing();

	bool GetConnect() { return m_bConnect; }
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
	afx_msg LRESULT OnClipboardChange(WPARAM wParam, LPARAM lPara);
	DECLARE_MESSAGE_MAP()

private:
	/**
	 * @brief The application's settings.
	 * @return theApp.Services().Settings().
	 */
	CGetSetOptions& Settings() const;

	/**
	 * @brief The clipboard formats Ditto registers by name.
	 * @return theApp.Services().ClipboardFormats().
	 */
	const CRegisteredClipboardFormats& Formats() const;

	void ProcessClipboardChange();
	/**
	 * @brief ValidActiveWnd's step: sets m_activeWindow to the lower-case process name of the
	 * clipboard owner, or of the foreground window when the owner has none.
	 */
	void UpdateActiveWindowName();
	/**
	 * @brief Looks for the first entry of an app name list that matches m_activeWindow.
	 * @param apps the lower-case app name list (wildcards allowed, separated by the settings' GetCopyAppSeparator()).
	 * @param line receives the matching (trimmed) entry.
	 * @return true if an entry matches.
	 */
	bool FindAppMatch(const CString& apps, CString& line);
	/**
	 * @brief OnTimer's TimerDrawClipboard step: hands the clipboard change on unless it came too fast.
	 * @param nIDEvent the timer id (killed here).
	 */
	void OnDrawClipboardTimer(UINT_PTR nIDEvent);
	/**
	 * @brief OnTimer's TimerPing step: reconnects when the ping was not answered.
	 */
	void OnPingTimer();
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CLIPBOARDVIEWER_H__67418FB6_6048_48FA_86D4_F412CACC41B1__INCLUDED_)

#pragma once

#include "QuickPaste.h"
#include "ToolTipEx.h"
#include "MainFrmThread.h"
#include "ClipboardSaveRestore.h"
#include "PowerManager.h"
#include "DittoPopupWindow.h"
#include "NTray.h"
#include "EditFrameWnd.h"

#include <array>
#include <memory>

class CHotKey;
class CGetSetOptions;
class CAppServices;

class CMainFrame: public CFrameWnd
{
public:
    CMainFrame();
protected:
    DECLARE_DYNAMIC(CMainFrame)

    // Attributes
public:

    // Operations
public:

    BOOL ResetKillDBTimer();

    // Overrides
    // ClassWizard generated virtual function overrides
    //{{AFX_VIRTUAL(CMainFrame)
public:
    virtual BOOL PreCreateWindow(CREATESTRUCT &cs);
    //	virtual BOOL Create(LPCTSTR lpszClassName, LPCTSTR lpszWindowName, DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID, CCreateContext* pContext = NULL);
    //}}AFX_VIRTUAL

    // Implementation
public:
    virtual ~CMainFrame();
    #ifdef _DEBUG
        virtual void AssertValid()const;
        virtual void Dump(CDumpContext &dc)const;
    #endif 

    CQuickPaste m_quickPaste;
	CTrayNotifyIcon m_trayIcon;
    ULONG m_ulCopyGap{};
    CString m_csKeyboardPaste;
    CAlphaBlend m_Transparency;
    BYTE m_keyStateModifiers;
    ULONGLONG m_startKeyStateTime{};
    bool m_bMovedSelectionMoveKeyState;
    short m_keyModifiersTimerCount;
    HWND m_tempFocusWnd{};
    CMainFrmThread m_thread;
	// The modeless dialogs while open; each is destroyed when its WM_*_CLOSED message arrives
	std::unique_ptr<CDialog> m_pGlobalClips{};
	std::unique_ptr<CDialog> m_pDeleteClips{};
	std::unique_ptr<CPropertySheet> m_pOptions{};
	int m_doubleClickGroupId;
	ULONGLONG m_doubleClickGroupStartTime{};
	CPowerManager m_PowerManager;
	int m_startupScreenWidth{};
	int m_startupScreenHeight{};
    CRichEditCtrlEx m_richEditTextConverter;

    void DoDittoCopyBufferPaste(int nCopyBuffer);
    void DoFirstTenPositionsPaste(int nPos);
	void PasteOrShowGroup(int dbId, BOOL updateClipTime, BOOL activeTarget, BOOL sendPaste, bool pastedFromGroup);

	void StartKeyModifierTimer();

	bool PasteQuickPasteEntry(CString csQuickPaste);
    void ShowErrorMessage(CString csTitle, CString csMessage);
    bool CloseAllOpenDialogs();
	void DoTextOnlyPaste();
	void RefreshShowInTaskBar();

    void ShowEditWnd(CClipIDs &Ids);
    // The open edit frame (it deletes itself in PostNcDestroy); null when closed
    CEditFrameWnd* m_pEditFrameWnd;


    // Generated message map functions
protected:
    //{{AFX_MSG(CMainFrame)
    afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
    afx_msg void OnFirstOption();
    afx_msg void OnFirstExit();
    afx_msg void OnTimer(UINT_PTR nIDEvent);
    afx_msg void OnFirstShowquickpaste();
    afx_msg void OnFirstToggleConnectCV();
    afx_msg void OnUpdateFirstToggleConnectCV(CCmdUI *pCmdUI);
    //}}AFX_MSG
    afx_msg LRESULT OnHotKey(WPARAM wParam, LPARAM lParam);
	void ShowQPasteWithActiveWindowCheck();
    afx_msg LRESULT OnShowTrayIcon(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnClipboardCopied(WPARAM wParam, LPARAM lParam);
	/**
	 * @brief CDittoMessage::ShowOwnedErrorMsg handler: shows an error balloon posted by CErrorReport.
	 * @param wParam A CString* allocated by the sender; this handler takes ownership and frees it.
	 * @param lParam Unused.
	 * @return TRUE.
	 */
	afx_msg LRESULT OnOwnedErrorMsg(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnEditWndClose(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnSetConnected(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOpenCloseWindow(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnGlobalClipsClosed(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnDeleteClipDataClosed(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnOptionsClosed(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnShowOptions(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnSaveClipboardMessage(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnReAddTaskBarIcon(WPARAM wParam, LPARAM lParam);
DECLARE_MESSAGE_MAP()public:
    virtual BOOL PreTranslateMessage(MSG *pMsg);
    /**
     * @brief Clears the services' main frame and handle (CAppWindows), then deletes the frame
     *        (CFrameWnd::PostNcDestroy), so no service keeps a pointer to the deleted frame.
     */
    void PostNcDestroy() override;
    afx_msg void OnClose();
    afx_msg void OnFirstImport();
    afx_msg void OnDestroy();
    afx_msg void OnFirstNewclip();
	afx_msg void OnFirstGlobalhotkeys();
	afx_msg void OnFirstDeleteclipdata();
	afx_msg void OnFirstSavecurrentclipboard();
	afx_msg LRESULT OnReOpenDatabase(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnShowMsgWindow(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnShowDittoGroup(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFirstFixupstickycliporder();
	afx_msg LRESULT OnResolutionChange(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnPlainTextPaste(WPARAM wParam, LPARAM lParam);
	afx_msg void OnWinIniChange(LPCTSTR lpszSection);
	afx_msg void OnFirstShowstartupmessage();
	afx_msg void OnUpdateFirstShowstartupmessage(CCmdUI *pCmdUI);
	afx_msg void OnFirstBackupdatabase();
	afx_msg void OnFirstRestoredatabase();
	afx_msg LRESULT OnRestoreDb(WPARAM wParam, LPARAM lParam);
	afx_msg LRESULT OnBackupDb(WPARAM wParam, LPARAM lParam);
	afx_msg void OnFirstDeleteallnonusedclips();
    afx_msg LRESULT OnPasteClip(WPARAM wParam, LPARAM lParam);
    afx_msg LRESULT OnEditClip(WPARAM wParam, LPARAM lParam);

	/** @brief The window message the tray icon sends to the main frame. */
	enum : UINT
	{
		/** @brief A tray icon notification (lParam: the mouse message). */
		WmTrayNotify = WM_USER + 100,
	};

private:
	// PasteOrShowGroup's clip branch: ends a pending group double press and pastes the clip;
	// an argument of -1 keeps the option's current value
	void PasteSingleClip(int dbId, BOOL updateClipTime, BOOL activeTarget, BOOL sendPaste, bool pastedFromGroup);

	/** @brief The timer ids of the main frame (SetTimer / KillTimer / OnTimer). */
	enum : UINT_PTR
	{
		/** @brief Closing the window on a timer (disabled; the id stays reserved). */
		CloseWindowTimer = 1,
		/** @brief Hides the tray icon. */
		HideIconTimer = 2,
		/** @brief Deletes old entries. */
		RemoveOldEntriesTimer = 3,
		/** @brief Removes old temporary files. */
		RemoveOldTempFilesTimer = 6,
		/** @brief Waits for the modifier keys to be released. */
		KeyStateModifiersTimer = 8,
		/** @brief Tracks the active window. */
		ActiveWindowTimer = 9,
		/** @brief Reads the database file. */
		ReadRandomDbFileTimer = 12,
		/** @brief Ends the wait for a group hot key's second press. */
		GroupDoubleClickTimer = 13,
		/** @brief The screen resolution changed. */
		ScreenResolutionChangedTimer = 15,
		/** @brief Shows the paste window after a delay. */
		DelayedShowDittoTimer = 16,
		/** @brief Reloads the Windows theme. */
		SetWindowsThemeTimer = 17,
		/** @brief Closes the no-database window. */
		CloseNoDbWindowTimer = 18,
	};

	/** @brief One entry of OnTimer's dispatch table. */
	struct TimerHandler
	{
		/** @brief The timer id (one of the *Timer ids). */
		UINT_PTR timerId{};
		/** @brief The member function that handles the timer. */
		void (CMainFrame::*handle)() = nullptr;
	};

	/** @brief The copy, paste and cut hot keys of one copy buffer (non-owning; the CHotKeys registry owns them). */
	struct CopyBufferHotKeys
	{
		/** @brief The hot key that copies into the buffer, or nullptr. */
		CHotKey* copy{};
		/** @brief The hot key that pastes the buffer, or nullptr. */
		CHotKey* paste{};
		/** @brief The hot key that cuts into the buffer, or nullptr. */
		CHotKey* cut{};
	};

	/** @brief OnTimer's dispatch table: the handler of each timer id (CloseWindowTimer has none). */
	static const std::array<TimerHandler, 11> s_timerHandlers;

	/**
	 * @brief The application's services (the frame is created by the application, not by an owner
	 *        holding the services).
	 * @return theApp.Services().
	 */
	CAppServices& Services() const;

	/**
	 * @brief The application's settings.
	 * @return Services().Settings().
	 */
	CGetSetOptions& Settings() const;

	/**
	 * @brief Tells whether a WM_HOTKEY id belongs to a hot key.
	 * @param hotKey The hot key, or nullptr.
	 * @param wParam The WM_HOTKEY id.
	 * @return True when hotKey is set and its atom is wParam.
	 */
	static bool IsHotKey(const CHotKey* hotKey, WPARAM wParam);

	/**
	 * @brief Tells whether a WM_HOTKEY id is one of the three show-Ditto hot keys.
	 * @param wParam The WM_HOTKEY id.
	 * @return True for a show-Ditto hot key.
	 */
	bool IsShowDittoHotKey(WPARAM wParam) const;

	/** @brief Handles a show-Ditto hot key: moves the selection, hides or shows the window. */
	void OnShowDittoHotKey();

	/**
	 * @brief Handles a first-ten position hot key (paste position 1 to 10).
	 * @param wParam The WM_HOTKEY id.
	 * @return True when wParam was a position hot key (and was handled).
	 */
	bool DoFirstTenHotKey(WPARAM wParam);

	/**
	 * @brief Handles a copy buffer hot key (copy, paste or cut of buffer 1 to 5).
	 * @param wParam The WM_HOTKEY id.
	 * @return True when wParam was a copy buffer hot key (and was handled).
	 */
	bool DoCopyBufferHotKey(WPARAM wParam);

	/**
	 * @brief Handles the copy, paste or cut hot key of one copy buffer.
	 * @param hotKeys The buffer's hot keys.
	 * @param buffer The buffer index, 0 to 4.
	 * @param wParam The WM_HOTKEY id.
	 * @return True when wParam was one of the buffer's hot keys (and was handled).
	 */
	bool DoCopyBufferHotKey(const CopyBufferHotKeys& hotKeys, int buffer, WPARAM wParam);

	/** @brief Handles the copy-and-save-clipboard hot key: sends a copy, waits, then saves the clipboard. */
	void DoCopyAndSaveClipboard();

	/**
	 * @brief Handles a global clip hot key (paste a clip, or save the selection to a group).
	 * @param wParam The WM_HOTKEY id.
	 */
	void DoGlobalClipHotKey(WPARAM wParam);

	/** @brief HideIconTimer: hides the tray icon unless the option shows it. */
	void OnHideIconTimer();
	/** @brief RemoveOldEntriesTimer: starts deleting old entries on the worker thread. */
	void OnRemoveOldEntriesTimer();
	/** @brief RemoveOldTempFilesTimer: starts removing old temporary files on the worker thread. */
	void OnRemoveOldTempFilesTimer();
	/** @brief KeyStateModifiersTimer: pastes or resets once the modifier keys are released. */
	void OnKeyStateModifiersTimer();
	/** @brief ActiveWindowTimer: tracks the active window while the paste window shows. */
	void OnActiveWindowTimer();
	/** @brief ReadRandomDbFileTimer: starts reading the database file on the worker thread. */
	void OnReadRandomDbFileTimer();
	/** @brief GroupDoubleClickTimer: handles a single press of a group hot key (opens Ditto on the group). */
	void OnGroupDoubleClickTimer();
	/** @brief ScreenResolutionChangedTimer: lets the paste window follow the new resolution. */
	void OnScreenResolutionChangedTimer();
	/** @brief DelayedShowDittoTimer: shows the paste window. */
	void OnDelayedShowDittoTimer();
	/** @brief SetWindowsThemeTimer: reloads the Windows theme and reopens a visible paste window. */
	void OnSetWindowsThemeTimer();
	/** @brief CloseNoDbWindowTimer: closes the no-database window. */
	void OnCloseNoDbWindowTimer();
};

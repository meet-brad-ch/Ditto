#pragma once
#include <afxwin.h>
#include "NTray.h"
#include "HotKeys.h"
#include <memory>

class COptionsSheet;

class CNoDbFrameWnd : public CFrameWnd
{
public:
	DECLARE_MESSAGE_MAP()
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);

public:
	CNoDbFrameWnd();
	/** @brief Defined in the .cpp file, where COptionsSheet is complete. */
	~CNoDbFrameWnd() override;

	CTrayNotifyIcon m_trayIcon;
	std::unique_ptr<COptionsSheet> m_pOptions{}; // the modeless options sheet while it is open
	// the hot keys below are owned by g_HotKeys; these pointers do not own them
	CHotKey* m_pDittoHotKey; // activate ditto's qpaste window
	CHotKey* m_pDittoHotKey2; // activate ditto's qpaste window
	CHotKey* m_pDittoHotKey3; // activate ditto's qpaste window

	afx_msg void OnFirstOptions();
	afx_msg void OnFirstExitNoDb();
	afx_msg LRESULT OnTrayNotification(WPARAM wParam, LPARAM lParam);
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	void ShowNoDbMessage();
	void TryOpenDatabase();
	LRESULT OnOptionsClosed(WPARAM wParam, LPARAM lParam);
	afx_msg void OnHotKey(UINT nHotKeyId, UINT nKey1, UINT nKey2);

private:
	/** @brief The window message the tray icon sends to the frame. */
	enum : UINT
	{
		/** @brief A tray icon notification (lParam: the mouse message). */
		WmTrayNotify = WM_USER + 100,
	};

	/** @brief The timer ids of the frame (SetTimer / OnTimer). */
	enum : UINT
	{
		/** @brief Retries opening the database. */
		TimerOpenDb = 1,
		/** @brief Shows the "no database" message. */
		TimerErrorMsg = 2,
	};
};


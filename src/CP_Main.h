#pragma once

#ifndef __AFXWIN_H__
#error include 'stdafx.h' before including this file for PCH
#endif

#include "../resource.h"       // main symbols
#include "Clip.h"
#include "DatabaseUtilities.h"
#include "Misc.h"
#include "Options.h"
#include "..\Shared\ArrayEx.h"
#include "MainFrm.h"
#include "ProcessPaste.h"
#include "MultiLanguage.h"
#include "CopyThread.h"
#include "ClipboardSaveRestore.h"
#include "DittoCopyBuffer.h"
#include "sqlite\CppSQLite3.h"
#include "DittoDb.h"
#include "DittoAddins.h"
#include "externalwindowtracker.h"
#include "HotKeys.h"
#include "UAC_Thread.h"
#include "ICU_String.h"
#include "ClipEditThread.h"
#include "AppState.h"
#include "AppWindows.h"
#include "ClipboardMonitor.h"
#include "ClipCommands.h"
#include "ClipDataReader.h"
#include "GroupNavigator.h"
#include "RegisteredClipboardFormats.h"
#include "AppServices.h"
#include <memory>


extern class CCP_MainApp theApp;

class DittoCommandLineInfo;

class CCP_MainApp : public CWinApp
{
public:
	CCP_MainApp();
	~CCP_MainApp();

	/**
	 * @brief The application's services (the composition root). For MFC windows, dialogs and
	 *        threads the framework creates; other classes get what they need from their owner.
	 * @return The services; they exist from the constructor until the destructor.
	 */
	CAppServices& Services();

	/**
	 * @brief Called by the main frame after its window was created: registers the hot keys, starts
	 *        the edit watcher and the copy thread, loads the add-ins.
	 * @return False when Ditto cannot run (reported).
	 */
	bool AfterMainCreate();

	/** @brief Called by the main frame before its window closes: unregisters the hot keys, stops the copy thread. */
	void BeforeMainClose();

	/**
	 * @brief Creates the main frame window (also after the no-database window found a database).
	 * @throws std::runtime_error When the window could not be created.
	 */
	void CreateMainWnd();

	/** @brief Destroys the no-database window, when it exists. */
	void CloseNoDbWindow();

private:
	/// The services; declared first so that they are created before and destroyed after every other member.
	std::unique_ptr<CAppServices> m_services{};

	/// The single-instance mutex.
	HANDLE m_hMutex{};
	/// The mutex of the elevated paste helper (only in the helper process).
	HANDLE m_adminPasteMutex{};
	/// TRUE or FALSE from /Connect or /Disconnect; -1 when the command line gives neither.
	int m_connectOnStartup{-1};
	/// The window shown when there is no database (not owned: it deletes itself, CFrameWnd::PostNcDestroy).
	CFrameWnd* m_pNoDbMainFrame{};
	/// The GDI+ token of GdiplusStartup.
	ULONG_PTR m_gdiplusToken{};

	/** @brief Creates the hot keys of the clips that have a global shortcut (or move-to-group shortcut). */
	void LoadGlobalClips();

public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()
	virtual BOOL OnIdle(LONG lCount);

protected:
	void ShowCommandLineError(CString csTitle, CString csMessage);
	// Puts an exported clip file given on the command line on the clipboard; reports any error
	void ImportFileFromCommandLine(const CString& fileName);

private:
	/**
	 * @brief Starts the application; InitInstance wraps it as the start-up error boundary.
	 * @return TRUE to run the message loop, FALSE to exit the process.
	 * @throws std::exception When a start-up step fails.
	 */
	BOOL InitInstanceBody();

	/** @brief A message this instance sends to the running Ditto for a command line switch. */
	struct RunningInstanceRequest
	{
		/** @brief True when the command line asks for this message. */
		bool requested{};
		/** @brief The message. */
		UINT message{};
		/** @brief The message's wParam. */
		WPARAM wParam{};
		/** @brief The message's lParam. */
		LPARAM lParam{};
	};

	/**
	 * @brief InitInstanceBody's command line step (restart, import, connect, or a request to the running Ditto).
	 * @param cmdInfo The parsed command line.
	 * @return False when this instance must exit (InitInstance returns FALSE).
	 */
	bool HandleCommandLine(const DittoCommandLineInfo& cmdInfo);

	/**
	 * @brief Handles /Connect or /Disconnect: passes it to the running Ditto, else starts this instance (dis)connected.
	 * @param cmdInfo The parsed command line.
	 * @return False when the running Ditto handled it (this instance exits).
	 */
	bool HandleConnectSwitch(const DittoCommandLineInfo& cmdInfo);

	/**
	 * @brief Sends the first requested command line request (open/close, exit, plain text paste, paste or edit a clip) to the running Ditto.
	 * @param cmdInfo The parsed command line.
	 * @return True when the command line held such a request (this instance exits).
	 */
	bool ForwardToRunningInstance(const DittoCommandLineInfo& cmdInfo);

	/**
	 * @brief Registers with the restart manager and creates the single instance mutex.
	 * @return False when Ditto already runs (the running one is asked to show its tray icon).
	 */
	bool CreateSingleInstanceMutex();
};
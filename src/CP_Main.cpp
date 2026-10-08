#include "stdafx.h"
#include "CP_Main.h"
#include "MainFrm.h"
#include "ClipboardFormatError.h"
#include "ErrorReport.h"
#include "Misc.h"
#include ".\cp_main.h"
#include <io.h>
#include "Clip_ImportExport.h"
#include "OptionsSheet.h"
#include "DittoCopyBuffer.h"
#include "SendKeys.h"
#include "MainTableFunctions.h"
#include "NoDbFrameWnd.h"
#include <array>
#include <clocale>
#include <memory>
#include <stdexcept>

class DittoCommandLineInfo : public CCommandLineInfo
{
public:
	DittoCommandLineInfo()
	{
		m_bDisconnect = FALSE;
		m_bConnect = FALSE;
		m_uacPID = 0;
		m_bOpenWindow = FALSE;
		m_bCloseWindow = FALSE;
		m_exit = FALSE;
		m_plainTextPaste = FALSE;
		m_pasteClip = FALSE;
		m_clipID = -1;
		m_editClip = FALSE;
		m_restartFromRestartManager = FALSE;
	}

	virtual void ParseParam(const TCHAR* pszParam, BOOL bFlag, BOOL bLast)
	{
		if (bFlag)
		{
			ParseFlag(pszParam);
		}

		CCommandLineInfo::ParseParam(pszParam, bFlag, bLast);
	}

	BOOL m_bDisconnect;
	BOOL m_bConnect;
	BOOL m_pasteClip;
	int m_uacPID;
	int m_clipID;
	BOOL m_bCloseWindow;
	BOOL m_exit;
	BOOL m_bOpenWindow;
	BOOL m_plainTextPaste;
	BOOL m_editClip;
	BOOL m_restartFromRestartManager;

private:
	/** @brief A switch that sets a flag when it is given exactly (case-insensitive). */
	struct ExactSwitch
	{
		/** @brief The switch text. */
		const TCHAR* name{};
		/** @brief The flag the switch sets to TRUE. */
		BOOL* flag{};
	};

	/**
	 * @brief Handles one command line switch (the text after / or -).
	 * @param pszParam The switch.
	 */
	void ParseFlag(const TCHAR* pszParam)
	{
		if (SetExactSwitch(pszParam))
		{
			return;
		}

		if (wcsncmp(pszParam, _T("uacpaste"), 8) == 0)
		{
			ReadNumberAfterColon(pszParam, m_uacPID);
		}
		else if (_wcsnicmp(pszParam, _T("paste"), 5) == 0)
		{
			if (ReadNumberAfterColon(pszParam, m_clipID))
			{
				m_pasteClip = TRUE;
			}
		}
		else if (_wcsnicmp(pszParam, _T("edit"), 4) == 0)
		{
			if (ReadNumberAfterColon(pszParam, m_clipID))
			{
				m_editClip = TRUE;
			}
		}
		else if (_wcsnicmp(pszParam, _T("RestartByRestartManager"), 23) == 0)
		{
			m_restartFromRestartManager = true;
		}
	}

	/**
	 * @brief Sets the flag of a switch given exactly: Connect, Disconnect, open, close, exit or PlainTextPaste.
	 * @param pszParam The switch.
	 * @return True when the switch was one of them.
	 */
	bool SetExactSwitch(const TCHAR* pszParam)
	{
		const std::array<ExactSwitch, 6> exactSwitches{ {
			{ _T("Connect"), &m_bConnect },
			{ _T("Disconnect"), &m_bDisconnect },
			{ _T("open"), &m_bOpenWindow },
			{ _T("close"), &m_bCloseWindow },
			{ _T("exit"), &m_exit },
			{ _T("PlainTextPaste"), &m_plainTextPaste },
		} };

		for (const ExactSwitch& exactSwitch : exactSwitches)
		{
			if (_tcsicmp(pszParam, exactSwitch.name) == 0)
			{
				*exactSwitch.flag = TRUE;
				return true;
			}
		}

		return false;
	}

	/**
	 * @brief Reads the number after the last ':' of a switch (uacpaste:pid, paste:id, edit:id).
	 * @param pszParam The switch.
	 * @param number Receives the number when the switch has a ':'.
	 * @return True when the switch has a ':'.
	 */
	static bool ReadNumberAfterColon(const TCHAR* pszParam, int& number)
	{
		CString pidCommand(pszParam);
		long sep = pidCommand.ReverseFind(':');
		if (sep > -1)
		{
			CString id = pidCommand.Right(pidCommand.GetLength() - sep - 1);
			number = _ttoi(id);
			return true;
		}

		return false;
	}
};

CCP_MainApp theApp;

BEGIN_MESSAGE_MAP(CCP_MainApp, CWinApp)
	//{{AFX_MSG_MAP(CCP_MainApp)
	// NOTE - the ClassWizard will add and remove mapping macros here.
	//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

CCP_MainApp::CCP_MainApp() :
	m_services(std::make_unique<CAppServices>())
{
	::AllowSetForegroundWindow(ASFW_ANY);

	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

CCP_MainApp::~CCP_MainApp()
{
}

CAppServices& CCP_MainApp::Services()
{
	return *m_services;
}

void CCP_MainApp::ImportFileFromCommandLine(const CString& fileName)
{
	try
	{
		Services().Settings().m_bEnableDebugLogging = Services().Settings().GetEnableDebugLogging();

		CppSQLite3DB db;
		db.open(fileName);

		CClip_ImportExport clip(Services().ClipContext());
		if (clip.ImportFromSqliteDB(db, false, true))
		{
			ShowCommandLineError("Ditto", Services().Language().GetString("Importing_Good", "Clip placed on clipboard"));
		}
		else
		{
			ShowCommandLineError("Ditto", Services().Language().GetString("Error_Importing", "Error importing exported clip"));
		}
	}
	catch (CppSQLite3Exception& e)
	{
		ASSERT(FALSE);

		CString csError;
		csError.Format(_T("%s - Exception - %d - %s"), Services().Language().GetString("Error_Parsing", "Error parsing exported clip").GetString(), e.errorCode(), e.errorMessage());
		ShowCommandLineError("Ditto", csError);
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CString csError;
		csError.Format(_T("%s - %s"), Services().Language().GetString("Error_Parsing", "Error parsing exported clip").GetString(), CString(error.what()).GetString());
		ShowCommandLineError("Ditto", csError);
	}
}

BOOL CCP_MainApp::InitInstance()
{
	// application-start boundary: the main window does not exist yet, so a failure is shown in a message box
	try
	{
		return InitInstanceBody();
	}
	catch (const std::exception& e)
	{
		AfxMessageBox(CStringUtil::Format(_T("Ditto could not start: %s"), CString(e.what()).GetString()), MB_OK | MB_ICONERROR);
		return FALSE;
	}
}

BOOL CCP_MainApp::InitInstanceBody()
{
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	AfxEnableControlContainer();
	AfxOleInit();
	CRichEditCtrlEx::InitRichEditEx();

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	LoadLibrary(TEXT("MSFTEDIT.DLL"));

	setlocale(LC_TIME, ".OCP"); // defines the date/time formatting

	DittoCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	Services().Settings().LoadSettings();

	Services().ActiveWindow().TrackActiveWnd(false);

	if (cmdInfo.m_uacPID > 0)
	{
		CLogger::Log(CStringUtil::Format(_T("Startup up ditto as admin to paste to admin windows, parent process id: %d"), cmdInfo.m_uacPID));

		CString mutex;
		mutex.Format(_T("DittoAdminPaste_%d"), cmdInfo.m_uacPID);
		m_adminPasteMutex = CreateMutex(NULL, FALSE, mutex);

		Services().ActiveWindow().RunUacHelper(cmdInfo.m_uacPID);

		return FALSE;
	}

	if (!HandleCommandLine(cmdInfo))
	{
		return FALSE;
	}

	auto runningVersion = CAppVersion::GetRunningVersion(Services().Settings().GetExeFileName());
	CString cs = CAppVersion::GetVersionString(runningVersion);
	cs.Insert(0, _T("InitInstance  -  Running Version - "));
	CLogger::Log(cs);

	if (!CreateSingleInstanceMutex())
	{
		return TRUE;
	}

	CString csFile = Services().Settings().GetLanguageFile();
	const CString languageDir = Services().Settings().GetPath(CGetSetOptions::PathLanguage);
	if (Services().Language().LoadLanguageFile(languageDir, csFile) == false)
	{
		CString csLanguageError;
		csLanguageError.Format(_T("Error loading language file - %s - \n\n%s"), csFile.GetString(), Services().Language().m_csLastError.GetString());
		CLogger::Log(csLanguageError);

		Services().Language().LoadLanguageFile(languageDir, _T("English.xml"));
	}

	Services().IcuString().Load();

	DatabaseLocator locator(Services().Settings(), Services().Language(), Services().Database(), Services().State());
	int nRet = locator.CheckDBExists(Services().Settings().GetDBPath());
	if (nRet == FALSE)
	{
		CreateNoDbWnd();
	}
	else
	{
		CreateMainWnd();
	}

	return TRUE;
}

void CCP_MainApp::CreateNoDbWnd()
{
	m_pNoDbMainFrame = std::make_unique<CNoDbFrameWnd>().release(); // ownership: the frame window itself (CFrameWnd::PostNcDestroy deletes it, also when LoadFrame fails)
	m_pMainWnd = m_pNoDbMainFrame;

	if (!m_pNoDbMainFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL))
	{
		// the failed frame destroyed itself (CFrameWnd::PostNcDestroy)
		m_pNoDbMainFrame = NULL;
		m_pMainWnd = NULL;
		throw std::runtime_error("the window shown without a database could not be created");
	}

	m_pNoDbMainFrame->ShowWindow(SW_SHOW);
	m_pNoDbMainFrame->UpdateWindow();
}

bool CCP_MainApp::HandleCommandLine(const DittoCommandLineInfo& cmdInfo)
{
	if (cmdInfo.m_restartFromRestartManager)
	{
		CLogger::Log(CStringUtil::Format(_T("Ditto was restarted from restart manager")));
	}
	else if (cmdInfo.m_strFileName.IsEmpty() == FALSE)
	{
		ImportFileFromCommandLine(cmdInfo.m_strFileName);
		return false;
	}
	else if (cmdInfo.m_bConnect || cmdInfo.m_bDisconnect)
	{
		return HandleConnectSwitch(cmdInfo);
	}
	else if (ForwardToRunningInstance(cmdInfo))
	{
		return false;
	}

	return true;
}

bool CCP_MainApp::HandleConnectSwitch(const DittoCommandLineInfo& cmdInfo)
{
	//First get the saved hwnd and send it a message
	//If ditto is running then this will return 1, meaning the running ditto process
	//handled this message
	//If it didn't handle the message(ditto is not running) then startup this processes of ditto
	//disconnected from the clipboard
	LRESULT ret = 0;
	HWND hWnd = (HWND)(LONG_PTR)Services().Settings().GetMainHWND();
	if (hWnd)
	{
		ret = ::SendMessage(hWnd, CDittoMessage::SetConnected, cmdInfo.m_bConnect, cmdInfo.m_bDisconnect);
	}

	//passed off to the running instance of ditto, exit this instance
	if (ret == 1)
	{
		return false;
	}

	if (cmdInfo.m_bConnect)
	{
		m_connectOnStartup = TRUE;
	}
	else if (cmdInfo.m_bDisconnect)
	{
		m_connectOnStartup = FALSE;
	}

	return true;
}

bool CCP_MainApp::ForwardToRunningInstance(const DittoCommandLineInfo& cmdInfo)
{
	// in this order: the first requested one is sent
	const std::array<RunningInstanceRequest, 5> requests{ {
		{ cmdInfo.m_bOpenWindow || cmdInfo.m_bCloseWindow, CDittoMessage::OpenCloseWindow, static_cast<WPARAM>(cmdInfo.m_bOpenWindow), static_cast<LPARAM>(cmdInfo.m_bCloseWindow) },
		{ cmdInfo.m_exit != FALSE, WM_CLOSE, 0, 0 },
		{ cmdInfo.m_plainTextPaste != FALSE, CDittoMessage::PlainTextPaste, 0, 0 },
		{ cmdInfo.m_pasteClip != FALSE, CDittoMessage::PasteClip, static_cast<WPARAM>(cmdInfo.m_clipID), 0 },
		{ cmdInfo.m_editClip != FALSE, CDittoMessage::EditClip, static_cast<WPARAM>(cmdInfo.m_clipID), 0 },
	} };

	for (const RunningInstanceRequest& request : requests)
	{
		if (request.requested)
		{
			//send the request to the running ditto (if it runs); this instance exits either way
			HWND hWnd = (HWND)(LONG_PTR)Services().Settings().GetMainHWND();
			if (hWnd)
			{
				::SendMessage(hWnd, request.message, request.wParam, request.lParam);
			}

			return true;
		}
	}

	return false;
}

bool CCP_MainApp::CreateSingleInstanceMutex()
{
	CString csMutex("Ditto Is Now Running");
	if (Services().Settings().GetIsPortableDitto() || Services().Settings().GetIsWindowsApp() || Services().Settings().GetIsChocolateyApp())
	{
		csMutex += " ";
		csMutex += Services().Settings().GetExeFileName();
	}

	CWinApp::RegisterWithRestartManager(false, csMutex);

	//create mutex doesn't like slashes, remove them, it always returns NULL with them in
	csMutex.Replace(_T("\\"), _T("_"));

	m_hMutex = CreateMutex(NULL, TRUE, csMutex);
	DWORD dwError = GetLastError();
	if (m_hMutex == NULL ||
		dwError == ERROR_ALREADY_EXISTS)
	{
		CLogger::Log(CStringUtil::Format(_T("Ditto is already running, closing, mutex: %s"), csMutex.GetString()));
		HWND hWnd = (HWND)(LONG_PTR)Services().Settings().GetMainHWND();
		if (hWnd)
			::SendMessage(hWnd, CDittoMessage::ShowTrayIcon, TRUE, TRUE);

		return false;
	}

	CLogger::Log(CStringUtil::Format(_T("Starting up ditto with mutex: %s"), csMutex.GetString()));

	return true;
}

void CCP_MainApp::CreateMainWnd()
{
	CMainFrame* pFrame{ std::make_unique<CMainFrame>().release() }; // ownership: the frame window itself (CFrameWnd::PostNcDestroy deletes it, also when LoadFrame fails)
	m_pMainWnd = pFrame;
	Services().Windows().SetMainFrame(pFrame);

	if (!pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL))
	{
		// the failed frame destroyed itself (CFrameWnd::PostNcDestroy); AfterMainCreate reported why
		m_pMainWnd = NULL;
		Services().Windows().SetMainFrame(NULL);
		Services().Windows().SetMainHwnd(NULL);
		throw std::runtime_error("the main window could not be created");
	}

	//removed to keep ditto from taking focus on startup
	//pFrame->ShowWindow(SW_SHOW);
	//pFrame->UpdateWindow();
}

void CCP_MainApp::CloseNoDbWindow()
{
	if (m_pNoDbMainFrame != NULL)
	{
		// the frame deletes itself once its window is destroyed (CFrameWnd::PostNcDestroy)
		m_pNoDbMainFrame->DestroyWindow();
		m_pNoDbMainFrame = NULL;
	}
}

bool CCP_MainApp::AfterMainCreate()
{
	const HWND mainHwnd{ Services().Windows().MainFrame()->m_hWnd };
	Services().Windows().SetMainHwnd(mainHwnd);
	ASSERT(::IsWindow(mainHwnd));
	Services().Settings().SetMainHWND((long)(LONG_PTR)mainHwnd);

	Services().HotKeys().Init(mainHwnd);

	// create hotkeys here.  the registry owns them and destroys them on exit
	Services().HotKeys().CreateNamed();

	Services().EditThread().StartWatchingFolderForChanges();

	LoadGlobalClips();

	Services().HotKeys().RegisterAll();
	if (!Services().Clipboard().Start(m_connectOnStartup))
	{
		return false;
	}

#ifdef UNICODE
	Services().Addins().LoadAll();
#endif

	Services().State().m_bAppRunning = true;
	return true;
}

void CCP_MainApp::LoadGlobalClips()
{
	try
	{
		{
			CppSQLite3Query q = Services().Database().execQuery(_T("SELECT lID, lShortCut, mText FROM Main WHERE lShortCut > 0 AND globalShortCut = 1"));

			while (q.eof() == false)
			{
				int id = q.getIntField(_T("lID"));
				int shortcut = q.getIntField(_T("lShortCut"));
				CString desc = q.getStringField(_T("mText"));

				// the registry owns the key and destroys it
				CHotKey& globalHotKey{ Services().HotKeys().Create(CStringUtil::Format(_T("GlobalClip: %d"), id), shortcut, true, CHotKey::PASTE_OPEN_CLIP, desc) };
				globalHotKey.m_clipId = id;

				q.nextRow();
			}
		}

		{
			CppSQLite3Query q2 = Services().Database().execQuery(_T("SELECT lID, MoveToGroupShortCut, mText FROM Main WHERE MoveToGroupShortCut > 0 AND GlobalMoveToGroupShortCut = 1"));

			while (q2.eof() == false)
			{
				int id = q2.getIntField(_T("lID"));
				int shortcut = q2.getIntField(_T("MoveToGroupShortCut"));
				CString desc = q2.getStringField(_T("mText"));

				// the registry owns the key and destroys it
				CHotKey& globalHotKey{ Services().HotKeys().Create(CStringUtil::Format(_T("MoveToGroup: %d"), id), shortcut, true, CHotKey::MOVE_TO_GROUP, desc) };
				globalHotKey.m_clipId = id;

				q2.nextRow();
			}
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the global clip hot keys from the clip database failed: %s"), e.errorMessage()));
		return;
	}
}

void CCP_MainApp::BeforeMainClose()
{
	CAppState& state{ Services().State() };
	ASSERT(state.m_bAppRunning && !state.m_bAppExiting);
	state.m_bAppRunning = false;
	state.m_bAppExiting = true;
	Services().HotKeys().UnregisterAll();
	Services().Clipboard().Stop();
}

/////////////////////////////////////////////////////////////////////////////
// CCP_MainApp message handlers

int CCP_MainApp::ExitInstance()
{
	CLogger::Log(_T("ExitInstance"));

	CTempFileCleaner::DeleteDittoTempFiles(Services().Settings(), FALSE);

	Services().Database().close();

	Services().ActiveWindow().StopUacThread();

	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	return CWinApp::ExitInstance();
}

// return TRUE if there is more idle processing to do
BOOL CCP_MainApp::OnIdle(LONG lCount)
{
	// let winapp handle its idle processing
	if (CWinApp::OnIdle(lCount))
		return TRUE;

	return FALSE;
}

void CCP_MainApp::ShowCommandLineError(CString csTitle, CString csMessage)
{
	CLogger::Log(CStringUtil::Format(_T("ShowCommandLineError %s - %s"), csTitle.GetString(), csMessage.GetString()));

	// handed off before Create: MFC's CWnd::CreateEx calls PostNcDestroy on every failure path,
	// so a failed Create has already deleted the window object
	CToolTipEx* pErrorWnd{ std::make_unique<CToolTipEx>().release() }; // ownership: the window itself (CToolTipEx::PostNcDestroy deletes it)
	if (!pErrorWnd->Create(NULL))
	{
		AfxMessageBox(csTitle + "\n\n" + csMessage, MB_OK | MB_ICONERROR);
		return;
	}
	pErrorWnd->SetToolTipText(csTitle + "\n\n" + csMessage);

	CPoint pt;
	CRect rcScreen = CMonitorGeometry::DefaultMonitorRect();
	pt = rcScreen.BottomRight();

	CRect cr = pErrorWnd->GetBoundsRect();

	pt.x -= max(cr.Width() + 50, 150);
	pt.y -= max(cr.Height() + 50, 150);

	pErrorWnd->Show(pt);

	CAppWindows::PumpMessages(pErrorWnd->m_hWnd);

	Sleep(4000);

	pErrorWnd->DestroyWindow();
}

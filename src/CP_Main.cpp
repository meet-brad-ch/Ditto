#include "stdafx.h"
#include "FileDialogPath.h"
#include "CP_Main.h"
#include "MainFrm.h"
#include "ClipboardFormatError.h"
#include "ErrorReport.h"
#include "Misc.h"
#include ".\cp_main.h"
#include <io.h>
#include "Path.h"
#include "Clip_ImportExport.h"
#include "OptionsSheet.h"
#include "DittoCopyBuffer.h"
#include "SendKeys.h"
#include "MainTableFunctions.h"
#include "ShowTaskBarIcon.h"
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
  		if(bFlag)
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

		if(wcsncmp(pszParam, _T("uacpaste"), 8) == 0)
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
	m_db([](const CString& text) { CLogger::Log(text); })
{
	m_copyReason = CopyReasonEnum::COPY_TO_UNKOWN;
	m_copyReasonStartTime = 0;
	m_activeGroupId = -1;
	m_activeGroupStartTime = 0;
	m_bAppRunning = false;
	m_bAppExiting = false;
	m_connectOnStartup = -1;
	m_MainhWnd = NULL;
	m_pMainFrame = NULL;
	::AllowSetForegroundWindow(ASFW_ANY);

	m_bShowingQuickPaste = false;

	m_GroupDefaultID = 0;
	m_GroupID = -1;
	m_GroupParentID = 0;
	m_GroupText = "History";
	m_FocusID = -1;

	ClearOldGroupState();

	m_bAsynchronousRefreshView = true;
	m_oldtStartUp = COleDateTime::GetCurrentTime();

	// Registered clipboard formats are in the range 0xC000..0xFFFF, so they fit in a CLIPFORMAT.
	m_RTFFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormat(_T("Rich Text Format")));
	m_HTML_Format = static_cast<CLIPFORMAT>(::RegisterClipboardFormat(_T("HTML Format")));
	m_PingFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormat(_T("Ditto Ping Format")));
	m_cfIgnoreClipboard = static_cast<CLIPFORMAT>(::RegisterClipboardFormat(_T("Clipboard Viewer Ignore")));
	m_cfDelaySavingData = static_cast<CLIPFORMAT>(::RegisterClipboardFormat(_T("Ditto Delay Saving Data")));
	m_DittoFileData = static_cast<CLIPFORMAT>(::RegisterClipboardFormat(_T("Ditto File Data")));
	m_PNG_Format = GetFormatID(_T("PNG"));

	//https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
	m_excludeClipboardContentFromMonitorProcessing = static_cast<CLIPFORMAT>(RegisterClipboardFormat(L"ExcludeClipboardContentFromMonitorProcessing"));

	//https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
	m_canIncludeInClipboardHistory = static_cast<CLIPFORMAT>(RegisterClipboardFormat(L"CanIncludeInClipboardHistory"));

	m_pNoDbMainFrame = NULL;
	m_databaseOnNetworkShare = false;

	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_RESTART;
}

CCP_MainApp::~CCP_MainApp()
{
	
}

void CCP_MainApp::ImportFileFromCommandLine(const CString& fileName)
{
	try
	{
		CGetSetOptions::m_bEnableDebugLogging = CGetSetOptions::GetEnableDebugLogging();

		CppSQLite3DB db;
		db.open(fileName);

		CClip_ImportExport clip;
		if(clip.ImportFromSqliteDB(db, false, true))
		{
			ShowCommandLineError("Ditto", theApp.m_Language.GetString("Importing_Good", "Clip placed on clipboard"));
		}
		else
		{
			ShowCommandLineError("Ditto", theApp.m_Language.GetString("Error_Importing", "Error importing exported clip"));
		}
	}
	catch (CppSQLite3Exception& e)
	{
		ASSERT(FALSE);

		CString csError;
		csError.Format(_T("%s - Exception - %d - %s"), theApp.m_Language.GetString("Error_Parsing", "Error parsing exported clip").GetString(), e.errorCode(), e.errorMessage());
		ShowCommandLineError("Ditto", csError);
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CString csError;
		csError.Format(_T("%s - %s"), theApp.m_Language.GetString("Error_Parsing", "Error parsing exported clip").GetString(), CString(error.what()).GetString());
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
		AfxMessageBox(StrF(_T("Ditto could not start: %s"), CString(e.what()).GetString()), MB_OK | MB_ICONERROR);
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
	AfxInitRichEditEx();	

	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&m_gdiplusToken, &gdiplusStartupInput, NULL);

	LoadLibrary(TEXT("MSFTEDIT.DLL"));

	setlocale(LC_TIME, ".OCP"); // defines the date/time formatting

	//MessageBox(NULL, _T("ditto starting"), _T("d"), MB_OK);

	DittoCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	CGetSetOptions::LoadSettings();

	theApp.m_activeWnd.TrackActiveWnd(false);

	if(cmdInfo.m_uacPID > 0)
	{
		CLogger::Log(StrF(_T("Startup up ditto as admin to paste to admin windows, parent process id: %d"), cmdInfo.m_uacPID));

		CString mutex;
		mutex.Format(_T("DittoAdminPaste_%d"), cmdInfo.m_uacPID);
		m_adminPasteMutex = CreateMutex(NULL, FALSE, mutex);

		m_pUacPasteThread = std::make_unique<CUAC_Thread>(cmdInfo.m_uacPID);
		m_pUacPasteThread->Start();
		m_pUacPasteThread->WaitForThreadToExit(INT_MAX);

		return FALSE;
	}

	if (!HandleCommandLine(cmdInfo))
	{
		return FALSE;
	}

	auto runningVersion = GetRunningVersion();
	CString cs = GetVersionString(runningVersion);
	cs.Insert(0, _T("InitInstance  -  Running Version - "));
	CLogger::Log(cs);

	if (!CreateSingleInstanceMutex())
	{
		return TRUE;
	}

	CString csFile = CGetSetOptions::GetLanguageFile();
	if(m_Language.LoadLanguageFile(csFile) == false)
	{
		CString csLanguageError;
		csLanguageError.Format(_T("Error loading language file - %s - \n\n%s"), csFile.GetString(), m_Language.m_csLastError.GetString());
		CLogger::Log(csLanguageError);

		m_Language.LoadLanguageFile(_T("English.xml"));
	}

	m_icuString.Load();
	
	int nRet = CheckDBExists(CGetSetOptions::GetDBPath());
	if(nRet == FALSE)
	{
		m_pNoDbMainFrame = std::make_unique<CNoDbFrameWnd>().release(); // ownership: the frame window itself (CFrameWnd::PostNcDestroy deletes it)
		m_pMainWnd = m_pNoDbMainFrame;
				
		m_pNoDbMainFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL);
		m_pNoDbMainFrame->ShowWindow(SW_SHOW);
		m_pNoDbMainFrame->UpdateWindow();
	}
	else
	{
		//Sleep(1000);
		CreateMainWnd();
	}

	return TRUE;
}

bool CCP_MainApp::HandleCommandLine(const DittoCommandLineInfo& cmdInfo)
{
	if (cmdInfo.m_restartFromRestartManager)
	{
		CLogger::Log(StrF(_T("Ditto was restarted from restart manager")));
	}
	else if(cmdInfo.m_strFileName.IsEmpty() == FALSE)
	{
		ImportFileFromCommandLine(cmdInfo.m_strFileName);
		return false;
	}
	else if(cmdInfo.m_bConnect || cmdInfo.m_bDisconnect)
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
	HWND hWnd = (HWND)(LONG_PTR)CGetSetOptions::GetMainHWND();
	if(hWnd)
	{
		ret = ::SendMessage(hWnd, CDittoMessage::SetConnected, cmdInfo.m_bConnect, cmdInfo.m_bDisconnect);
	}

	//passed off to the running instance of ditto, exit this instance
	if(ret == 1)
	{
		return false;
	}

	if(cmdInfo.m_bConnect)
	{
		m_connectOnStartup = TRUE;
	}
	else if(cmdInfo.m_bDisconnect)
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
			HWND hWnd = (HWND)(LONG_PTR)CGetSetOptions::GetMainHWND();
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
	if(CGetSetOptions::GetIsPortableDitto() || CGetSetOptions::GetIsWindowsApp() || CGetSetOptions::GetIsChocolateyApp())
	{
		csMutex += " ";
		csMutex += CGetSetOptions::GetExeFileName();
	}

	CWinApp::RegisterWithRestartManager(false, csMutex);

	//create mutex doesn't like slashes, remove them, it always returns NULL with them in
	csMutex.Replace(_T("\\"), _T("_"));

	m_hMutex = CreateMutex(NULL, TRUE, csMutex);
	DWORD dwError = GetLastError();
	if(m_hMutex == NULL ||
		dwError == ERROR_ALREADY_EXISTS)
	{
		CLogger::Log(StrF(_T("Ditto is already running, closing, mutex: %s"), csMutex.GetString()));
		HWND hWnd = (HWND)(LONG_PTR)CGetSetOptions::GetMainHWND();
		if(hWnd)
			::SendMessage(hWnd, CDittoMessage::ShowTrayIcon, TRUE, TRUE);

		return false;
	}

	CLogger::Log(StrF(_T("Starting up ditto with mutex: %s"), csMutex.GetString()));

	return true;
}

void CCP_MainApp::CreateMainWnd()
{
	CMainFrame* pFrame{std::make_unique<CMainFrame>().release()}; // ownership: the frame window itself (CFrameWnd::PostNcDestroy deletes it, also when LoadFrame fails)
	m_pMainWnd = m_pMainFrame = pFrame;

	if (!pFrame->LoadFrame(IDR_MAINFRAME, WS_OVERLAPPEDWINDOW | FWS_ADDTOTITLE, NULL, NULL))
	{
		// the failed frame destroyed itself (CFrameWnd::PostNcDestroy); AfterMainCreate reported why
		m_pMainWnd = m_pMainFrame = NULL;
		m_MainhWnd = NULL;
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
	m_MainhWnd = m_pMainFrame->m_hWnd;
	ASSERT( ::IsWindow(m_MainhWnd) );
	CGetSetOptions::SetMainHWND((long)(LONG_PTR)m_MainhWnd);

	g_HotKeys.Init(m_MainhWnd);

	// create hotkeys here.  g_HotKeys owns them and destroys them on exit
	m_pDittoHotKey = &g_HotKeys.Create(CString("DittoHotKey"), 704); //704 is ctrl-tilda
	m_pDittoHotKey2 = &g_HotKeys.Create(CString("DittoHotKey2"));
	m_pDittoHotKey3 = &g_HotKeys.Create(CString("DittoHotKey3"));

	m_pPosOne = &g_HotKeys.Create("Position1", 0, true);
	m_pPosTwo = &g_HotKeys.Create("Position2", 0, true);
	m_pPosThree = &g_HotKeys.Create("Position3", 0, true);
	m_pPosFour = &g_HotKeys.Create("Position4", 0, true);
	m_pPosFive = &g_HotKeys.Create("Position5", 0, true);
	m_pPosSix = &g_HotKeys.Create("Position6", 0, true);
	m_pPosSeven = &g_HotKeys.Create("Position7", 0, true);
	m_pPosEight = &g_HotKeys.Create("Position8", 0, true);
	m_pPosNine = &g_HotKeys.Create("Position9", 0, true);
	m_pPosTen = &g_HotKeys.Create("Position10", 0, true);

	m_pCopyBuffer1 = &g_HotKeys.Create("CopyBufferCopyHotKey_0", 0, true);
	m_pPasteBuffer1 = &g_HotKeys.Create("CopyBufferPasteHotKey_0", 0, true);
	m_pCutBuffer1 = &g_HotKeys.Create("CopyBufferCutHotKey_0", 0, true);

	m_pCopyBuffer2 = &g_HotKeys.Create("CopyBufferCopyHotKey_1", 0, true);
	m_pPasteBuffer2 = &g_HotKeys.Create("CopyBufferPasteHotKey_1", 0, true);
	m_pCutBuffer2 = &g_HotKeys.Create("CopyBufferCutHotKey_1", 0, true);

	m_pCopyBuffer3 = &g_HotKeys.Create("CopyBufferCopyHotKey_2", 0, true);
	m_pPasteBuffer3 = &g_HotKeys.Create("CopyBufferPasteHotKey_2", 0, true);
	m_pCutBuffer3 = &g_HotKeys.Create("CopyBufferCutHotKey_2", 0, true);

	m_pCopyBuffer4 = &g_HotKeys.Create("CopyBufferCopyHotKey_3", 0, true);
	m_pPasteBuffer4 = &g_HotKeys.Create("CopyBufferPasteHotKey_3", 0, true);
	m_pCutBuffer4 = &g_HotKeys.Create("CopyBufferCutHotKey_3", 0, true);

	m_pCopyBuffer5 = &g_HotKeys.Create("CopyBufferCopyHotKey_4", 0, true);
	m_pPasteBuffer5 = &g_HotKeys.Create("CopyBufferPasteHotKey_4", 0, true);
	m_pCutBuffer5 = &g_HotKeys.Create("CopyBufferCutHotKey_4", 0, true);

	m_pTextOnlyPaste = &g_HotKeys.Create("TextOnlyPaste", 0, true);

	m_pSaveClipboard = &g_HotKeys.Create("SaveClipboard", 0, false);

	m_pCopyAndSaveClipboard = &g_HotKeys.Create("CopyAndSaveClipboard", 0, false);

	m_editThread.StartWatchingFolderForChanges();

	LoadGlobalClips();

	g_HotKeys.RegisterAll();
	if (!StartCopyThread())
	{
		return false;
	}

#ifdef UNICODE
	m_Addins.LoadAll();
#endif
	
	m_bAppRunning = true;
	return true;
}

void CCP_MainApp::LoadGlobalClips()
{
	try
	{
		{
			CppSQLite3Query q = m_db.execQuery(_T("SELECT lID, lShortCut, mText FROM Main WHERE lShortCut > 0 AND globalShortCut = 1"));

			while(q.eof() == false)
			{
				int id = q.getIntField(_T("lID"));
				int shortcut = q.getIntField(_T("lShortCut"));
				CString desc = q.getStringField(_T("mText"));

				// g_HotKeys owns the key and destroys it
				CHotKey& globalHotKey{g_HotKeys.Create(StrF(_T("GlobalClip: %d"), id), shortcut, true, CHotKey::PASTE_OPEN_CLIP, desc)};
				globalHotKey.m_clipId = id;

				q.nextRow();
			}
		}

		{
			CppSQLite3Query q2 = m_db.execQuery(_T("SELECT lID, MoveToGroupShortCut, mText FROM Main WHERE MoveToGroupShortCut > 0 AND GlobalMoveToGroupShortCut = 1"));

			while(q2.eof() == false)
			{
				int id = q2.getIntField(_T("lID"));
				int shortcut = q2.getIntField(_T("MoveToGroupShortCut"));
				CString desc = q2.getStringField(_T("mText"));

				// g_HotKeys owns the key and destroys it
				CHotKey& globalHotKey{g_HotKeys.Create(StrF(_T("MoveToGroup: %d"), id), shortcut, true, CHotKey::MOVE_TO_GROUP, desc)};
				globalHotKey.m_clipId = id;

				q2.nextRow();
			}
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Loading the global clip hot keys from the clip database failed: %s"), e.errorMessage()));
		return;
	}
}

void CCP_MainApp::BeforeMainClose()
{
	ASSERT( m_bAppRunning && !m_bAppExiting );
	m_bAppRunning = false;
	m_bAppExiting = true;
	g_HotKeys.UnregisterAll();
	StopCopyThread();
}

bool CCP_MainApp::StartCopyThread()
{
	ASSERT( m_MainhWnd );
	std::unique_ptr<CClipTypes> pTypes{LoadTypesFromDB()};
	if (!pTypes)
	{
		// LoadTypesFromDB reported why; without the clip types the copy thread cannot work
		return false;
	}
	// initialize to:
	// - m_MainhWnd = send CDittoMessage::ClipboardCopied messages to m_MainhWnd
	// - true = use Asynchronous communication (PostMessage)
	// - true = enable copying on clipboard changes
	// - pTypes = the supported types to use
	m_CopyThread.Init(CCopyConfig(m_MainhWnd, true, true, std::move(pTypes)));
	
	if(m_connectOnStartup == FALSE || CGetSetOptions::GetConnectedToClipboard() == FALSE)
	{
		m_CopyThread.m_connectOnStartup = false;
		CLogger::Log(StrF(_T("Starting Ditto up disconnected from the clipboard, commandLine: %d, saved value: %d"), m_connectOnStartup, CGetSetOptions::GetConnectedToClipboard()));
		SetConnectCV(false);
	}
	else if(m_connectOnStartup == TRUE)
	{
		SetConnectCV(true);
		CLogger::Log(_T("Starting Ditto up connected from the clipboard, passed in true from command line to start connected"));
	}

	if (!m_CopyThread.CreateThread(CREATE_SUSPENDED))
	{
		CErrorReport::Show(StrF(_T("Starting the clipboard copy thread failed (error %u)."), ::GetLastError()));
		return false;
	}
	m_CopyThread.ResumeThread();
	return true;
}

void CCP_MainApp::StopCopyThread()
{
	EnableCbCopy(false);
	m_CopyThread.Quit();
}

// returns the current Clipboard Viewer Connect state (though it might not yet
//  be actually connected -- check IsClipboardViewerConnected())
bool CCP_MainApp::ToggleConnectCV()
{
	bool bConnect = !GetConnectCV();
	SetConnectCV(bConnect);
	return bConnect;
}

// Sets a menu entry according to the current Clipboard Viewer Connection status
// - the menu text indicates the available command (opposite the current state)
// - a check mark appears in the rare cases that the menu text actually represents
//   the current state, e.g. if we are supposed to be connected, but we somehow
//   lose that connection, "Disconnect from Clipboard" will have a check next to it.
void CCP_MainApp::UpdateMenuConnectCV(CMenu* pMenu, UINT nMenuID)
{
	if(pMenu == NULL)
		return;

	bool bConnect = theApp.GetConnectCV();
	CString cs;

	if(bConnect)
	{
		cs = theApp.m_Language.GetString("Disconnect_Clipboard", "Disconnect from Clipboard.");
		pMenu->ModifyMenu(nMenuID, MF_BYCOMMAND, nMenuID, cs);
	}
	else
	{
		cs = theApp.m_Language.GetString("Connect_Clipboard", "Connect to Clipboard.");
		pMenu->ModifyMenu(nMenuID, MF_BYCOMMAND, nMenuID, cs);
	}
}

// Allocates a new CClipTypes; null when the database read failed (reported)
std::unique_ptr<CClipTypes> CCP_MainApp::LoadTypesFromDB()
{
	std::unique_ptr<CClipTypes> pTypes{std::make_unique<CClipTypes>()};

	try
	{
		CppSQLite3Query q = theApp.m_db.execQuery(_T("SELECT TypeText FROM Types"));
		while(q.eof() == false)
		{
			pTypes->Add(GetFormatID(q.getStringField(_T("TypeText"))));

			q.nextRow();
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Loading the clipboard types to save from the clip database failed: %s"), e.errorMessage()));
		return nullptr;
	}

	if(pTypes->GetSize() <= 0)
	{
		pTypes->Add(CF_TEXT);
		pTypes->Add(GetFormatID(CF_RTF));
		pTypes->Add(CF_UNICODETEXT);
		pTypes->Add(CF_HDROP);
		pTypes->Add(CF_DIB);
		pTypes->Add(GetFormatID(_T("HTML Format")));
		pTypes->Add(GetFormatID(_T("PNG")));
	}

	return pTypes;
}

void CCP_MainApp::ReloadTypes()
{
	std::unique_ptr<CClipTypes> pTypes{LoadTypesFromDB()};

	if(pTypes)
	{
		m_CopyThread.SetSupportedTypes(std::move(pTypes));
	}
}

void CCP_MainApp::RefreshView(CopyReasonEnum::CopyReason copyReason)
{
	CQPasteWnd *pWnd = QPasteWnd();
	if(pWnd)
	{
		if(m_bAsynchronousRefreshView)
		{
			pWnd->PostMessage(CDittoMessage::RefreshView, copyReason, 0);
		}
		else
		{
			pWnd->SendMessage(CDittoMessage::RefreshView, copyReason, 0);
		}
	}
}

void CCP_MainApp::RefreshClipInUI(int clipId, int updateFlags)
{
	CQPasteWnd *pWnd = QPasteWnd();
	if(pWnd)
	{
		if(m_bAsynchronousRefreshView)
		{
			pWnd->PostMessage(CDittoMessage::ReloadClipInUi, clipId, updateFlags);		
		}
		else
		{
			pWnd->SendMessage(CDittoMessage::ReloadClipInUi, clipId, updateFlags);
		}
	}
}

void CCP_MainApp::OnPasteCompleted()
{
}

void CCP_MainApp::OnCopyCompleted(long lLastID, int count, CopyReasonEnum::CopyReason copyReason)
{
	if(count <= 0)
	{
		return;
	}

	// update copy statistics
	CGetSetOptions::SetTripCopyCount(-count);
	CGetSetOptions::SetTotalCopyCount(-count);

	if(m_CopyBuffer.Active())
	{
		m_CopyBuffer.EndCopy(lLastID);
	}

	RefreshView(copyReason);
}

void CCP_MainApp::SaveCurrentGroupState()
{
	m_oldGroupID = m_GroupID;
	m_oldGroupParentID = m_GroupParentID;
	m_oldGroupText = m_GroupText;
}

void CCP_MainApp::ClearOldGroupState()
{
	m_oldGroupID = -2;
	m_oldGroupParentID = -2;
	m_oldGroupText = _T("");
}

BOOL CCP_MainApp::TryEnterOldGroupState()
{
	BOOL enteredGroup = FALSE;

	if(m_oldGroupID > -2)
	{
		m_GroupID = m_oldGroupID;
		m_GroupParentID = m_oldGroupParentID;
		m_GroupText = m_oldGroupText;

		ClearOldGroupState();

		theApp.RefreshView();
		if(QPasteWnd())
			QPasteWnd()->UpdateStatus(true);

		enteredGroup = TRUE;
	}

	return enteredGroup;
}

BOOL CCP_MainApp::EnterGroupID(long lID, BOOL clearOldGroupState/* = TRUE*/, BOOL saveCurrentGroupState/* = FALSE*/)
{
	BOOL bResult = FALSE;

	if(m_GroupID == lID)
		return TRUE;

	ULONGLONG startTick = GetTickCount64();

	if(clearOldGroupState)
	{
		ClearOldGroupState();
	}

	if(saveCurrentGroupState)
	{
		SaveCurrentGroupState();
	}

	// if we are switching to the parent, focus on the previous group
	if(m_GroupParentID == lID && m_GroupID > 0)
		m_FocusID = m_GroupID;

	if (!OpenGroup(lID, bResult))
	{
		return FALSE;
	}

	FinishEnterGroup(bResult, startTick);

	return bResult;
}

bool CCP_MainApp::OpenGroup(long lID, BOOL& bResult)
{
	switch(lID)
	{
	case -1:
		m_FocusID = -1;
		m_GroupID = -1;
		m_GroupParentID = -1;
		m_GroupText = "History";
		bResult = TRUE;
		break;
	default: // Normal Group
		try
		{
			bResult = EnterStoredGroup(lID);
		}
		catch (CppSQLite3Exception& e)
		{
			CErrorReport::Show(StrF(_T("Opening group id %ld failed: %s"), lID, e.errorMessage()));
			return false;
		}
		break;
	}

	return true;
}

void CCP_MainApp::FinishEnterGroup(BOOL bResult, ULONGLONG startTick)
{
	if(bResult)
	{
		theApp.RefreshView();
		if(QPasteWnd())
			QPasteWnd()->UpdateStatus(true);
	}

	ULONGLONG endTick = GetTickCount64();
	if((endTick-startTick) > 150)
		CLogger::Log(StrF(_T("Paste Timing EnterParentId: %llu"), endTick-startTick));
}

BOOL CCP_MainApp::EnterStoredGroup(long lID)
{
	BOOL bResult{FALSE};
	CppSQLite3Query q{theApp.m_db.execQueryEx(_T("SELECT lParentID, mText, bIsGroup FROM Main WHERE lID = %d"), lID)};
	if(q.eof() == false)
	{
		if(q.getIntField(_T("bIsGroup")) > 0)
		{
			m_GroupID = lID;
			m_GroupParentID = q.getIntField(_T("lParentID"));
			m_GroupText = q.getStringField(_T("mText"));
			bResult = TRUE;
		}
	}

	return bResult;
}

// returns a usable group id (not negative)
long CCP_MainApp::GetValidGroupID()
{
	return m_GroupID;
}

// sets a valid id
void CCP_MainApp::SetGroupDefaultID(long lID)
{
	if(m_GroupDefaultID == lID)
	{
		return;
	}

	if(lID <= 0)
	{
		m_GroupDefaultID = 0;
	}
	else
	{
		m_GroupDefaultID = lID;
	}

	if(QPasteWnd())
	{
		QPasteWnd()->UpdateStatus();
	}
}

void CCP_MainApp::SetStatus(const TCHAR* status, bool bRepaintImmediately)
{
	m_Status = status;
	if(QPasteWnd())
	{
		QPasteWnd()->UpdateStatus(bRepaintImmediately);
	}
}

void CCP_MainApp::ShowPersistent(bool bVal)
{
	CGetSetOptions::SetShowPersistent(bVal);

	// give some visual indication
	if(m_bShowingQuickPaste)
	{
		ASSERT(QPasteWnd());
		QPasteWnd()->SetCaptionColorActive(CGetSetOptions::m_bShowPersistent, theApp.GetConnectCV());
		QPasteWnd()->RefreshNc();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CCP_MainApp message handlers

int CCP_MainApp::ExitInstance() 
{
	CLogger::Log(_T("ExitInstance"));

	DeleteDittoTempFiles(FALSE);

	m_db.close();

	if(m_pUacPasteThread)
	{
		if(m_pUacPasteThread->ThreadWasStarted() == false)
		{
			m_pUacPasteThread->FireExit();
		}
		m_pUacPasteThread.reset();
	}

	Gdiplus::GdiplusShutdown(m_gdiplusToken);

	return CWinApp::ExitInstance();
}

// return TRUE if there is more idle processing to do
BOOL CCP_MainApp::OnIdle(LONG lCount)
{
	// let winapp handle its idle processing
	if(CWinApp::OnIdle(lCount))
		return TRUE;

	return FALSE;
}

void CCP_MainApp::SetConnectCV(bool bConnect)
{ 
	m_CopyThread.SetConnectCV(bConnect); 
	CGetSetOptions::SetConnectedToClipboard(bConnect == true);

	if(bConnect)
	{
		m_pMainFrame->m_trayIcon.SetIcon(IDR_MAINFRAME);
		m_pMainFrame->m_trayIcon.SetTooltipText(_T("Ditto"));
	}
	else
	{
		m_pMainFrame->m_trayIcon.SetIcon(IDI_DITTO_NOCOPYCB);
		CString cs;
		cs = _T("Ditto ");
		cs += theApp.m_Language.GetString("disconnected", "[Disconnected]");
		m_pMainFrame->m_trayIcon.SetTooltipText(cs);
	}

	if(QPasteWnd())
	{
		QPasteWnd()->SetCaptionColorActive(CGetSetOptions::m_bShowPersistent, theApp.GetConnectCV());
		QPasteWnd()->RefreshNc();
	}
}

void CCP_MainApp::OnDeleteID(long lID)
{
	if(QPasteWnd())
	{
		QPasteWnd()->PostMessage(CQListCtrl::NmItemDeleted, lID, 0);
	}
}

bool CCP_MainApp::ImportClips(HWND hWnd)
{
	OPENFILENAME	FileName;
	TCHAR			szFileName[400];
	TCHAR			szDir[400];

	memset(&FileName, 0, sizeof(FileName));
	memset(szFileName, 0, sizeof(szFileName));
	memset(&szDir, 0, sizeof(szDir));

	CString csInitialDir = CGetSetOptions::GetLastImportDir();
	_tcscpy(szDir, csInitialDir);

	FileName.lStructSize = sizeof(FileName);
	FileName.lpstrTitle = _T("Import Clips");
	FileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	FileName.nMaxFile = _countof(szFileName);
	FileName.lpstrFile = szFileName;
	FileName.lpstrInitialDir = szDir;
	FileName.lpstrFilter = _T("Exported Ditto Clips (.dto)\0*.dto\0\0");
	FileName.lpstrDefExt = _T("dto");

	if(GetOpenFileName(&FileName) == 0)
	{
		return false;
	}

	using namespace nsPath;
	CPath path(CFileDialogPath::From(FileName));
	CString csPath(path.GetPath());
	CGetSetOptions::SetLastImportDir(csPath);
	
	try
	{
		CppSQLite3DB db;
		db.open(CFileDialogPath::From(FileName));

		CClip_ImportExport clip;
		if(clip.ImportFromSqliteDB(db, true, false))
		{
			CShowTaskBarIcon show;

			CString cs;
			
			cs.Format(_T("%s %d "), theApp.m_Language.GetString("Import_Successfully", "Successfully imported").GetString(), clip.m_importCount);
			if(clip.m_importCount == 1)
				cs += theApp.m_Language.GetString("Clip", "clip");
			else
				cs += theApp.m_Language.GetString("Clips", "clips");

			MessageBox(hWnd, cs, _T("Ditto"), MB_OK);
		}
		else
		{
			CShowTaskBarIcon show;
			MessageBox(hWnd, theApp.m_Language.GetString("Error_Importing", "Error importing exported clip"), _T("Ditto"), MB_OK);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		ASSERT(FALSE);

		CString csError;
		csError.Format(_T("%s - Exception - %d - %s"), theApp.m_Language.GetString("Error_Parsing", "Error parsing exported clip").GetString(), e.errorCode(), e.errorMessage());
		MessageBox(hWnd, csError, _T("Ditto"), MB_OK);
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CString csError;
		csError.Format(_T("%s - %s"), theApp.m_Language.GetString("Error_Parsing", "Error parsing exported clip").GetString(), CString(error.what()).GetString());
		MessageBox(hWnd, csError, _T("Ditto"), MB_OK);
	}

	return true;
}

void CCP_MainApp::ShowCommandLineError(CString csTitle, CString csMessage)
{
	CLogger::Log(StrF(_T("ShowCommandLineError %s - %s"), csTitle.GetString(), csMessage.GetString()));

	// handed off before Create: MFC's CWnd::CreateEx calls PostNcDestroy on every failure path,
	// so a failed Create has already deleted the window object
	CToolTipEx* pErrorWnd{std::make_unique<CToolTipEx>().release()}; // ownership: the window itself (CToolTipEx::PostNcDestroy deletes it)
	if (!pErrorWnd->Create(NULL))
	{
		AfxMessageBox(csTitle + "\n\n" + csMessage, MB_OK | MB_ICONERROR);
		return;
	}
	pErrorWnd->SetToolTipText(csTitle + "\n\n" + csMessage);

	CPoint pt;
	CRect rcScreen = DefaultMonitorRect();
	pt = rcScreen.BottomRight();

	CRect cr = pErrorWnd->GetBoundsRect();

	pt.x -= max(cr.Width()+50, 150);
	pt.y -= max(cr.Height()+50, 150);

	pErrorWnd->Show(pt);

	PumpMessageEx(pErrorWnd->m_hWnd);
	
	Sleep(4000);

	pErrorWnd->DestroyWindow();
}

BOOL CCP_MainApp::GetClipData(long parentId, CClipFormat &Clip)
{
	BOOL bRet = FALSE;

	try
	{
		CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT ooData FROM Data WHERE lParentID = %d AND strClipboardFormat = '%s'"), parentId, GetFormatName(Clip.m_cfType).GetString());
		if(q.eof() == false)
		{
			int nDataLen = 0;
			const unsigned char *cData = q.getBlobField(_T("ooData"), nDataLen);
			if(cData != NULL)
			{
				Clip.m_hgData = NewGlobal(nDataLen);

				::CopyToGlobalHP(Clip.m_hgData, (LPVOID)cData, nDataLen);

				bRet = TRUE;
			}
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Loading the data of clip id %ld from the clip database failed: %s"), parentId, e.errorMessage()));
		return FALSE;
	}

	return bRet;
}

bool CCP_MainApp::EditItems(CClipIDs &Ids, bool /*bShowError*/, bool forceTextEdit)
{
	bool ret = false;	
	
	int lastFileCheckId = 1;

	for (int i = 0; i < min(Ids.GetCount(), 20); i++)
	{
		if (EditItem(Ids[i], forceTextEdit, lastFileCheckId))
		{
			ret = true;
		}
	}

	return ret;
}

bool CCP_MainApp::EditItem(int id, bool forceTextEdit, int& lastFileCheckId)
{
	CClip clip;
	if (id >= 0 && clip.LoadFormats(id) == false)
	{
		CLogger::Log(StrF(_T("Failed to load formats for clipId: %d"), id));
		return false;
	}

	ClipEditTarget target{};
	if (!ChooseClipEditTarget(clip, id, forceTextEdit, target))
	{
		return false;
	}

	if (EditInInternalEditor(target, id))
	{
		return false;
	}

	CString savePath = MakeEditFilePath(id, target.extension, lastFileCheckId);

	m_editThread.WatchFile(savePath);

	if (target.imageFile)
	{
		clip.WriteImageToFileOrReport(savePath, _T("edit"));
	}
	else
	{
		clip.WriteTextToFile(savePath, target.unicodeFile, target.asciFile, target.rtfFile, (id == -1));
	}

	return LaunchClipEditor(target.exePath, savePath, id);
}

bool CCP_MainApp::ChooseClipEditTarget(CClip& clip, int id, bool forceTextEdit, ClipEditTarget& target)
{
	if (forceTextEdit == false && clip.ContainsClipFormat(theApp.m_RTFFormat))
	{
		target.extension = _T("rtf");
		target.rtfFile = true;
		target.exePath = CGetSetOptions::GetRTFEditorPath();
	}
	else if (clip.ContainsClipFormat(CF_UNICODETEXT))
	{
		target.extension = _T("txt");
		target.unicodeFile = true;
		target.exePath = CGetSetOptions::GetTextEditorPath();
	}
	else if (clip.ContainsClipFormat(CF_TEXT))
	{
		target.extension = _T("txt");
		target.asciFile = true;
		target.exePath = CGetSetOptions::GetTextEditorPath();
	}
	else if (id == -1)
	{
		target.extension = _T("txt");
		target.unicodeFile = true;
		target.exePath = CGetSetOptions::GetTextEditorPath();
	}
	else if (clip.ContainsClipFormat(theApp.m_PNG_Format))
	{
		target.imageFile = true;
		target.extension = _T("png");
		target.exePath = CGetSetOptions::GetImageEditorPath();
	}
	else if (clip.ContainsClipFormat(CF_DIB))
	{
		target.imageFile = true;
		target.extension = _T("bmp");
		target.exePath = CGetSetOptions::GetImageEditorPath();
	}
	else
	{
		return false;
	}

	return true;
}

bool CCP_MainApp::EditInInternalEditor(const ClipEditTarget& target, int id)
{
	if((target.unicodeFile || target.asciFile || target.rtfFile) && target.exePath == _T(""))
	{
		CLogger::Log(StrF(_T("Clip id %d is a text or rtf file without a specific editor set, using internal editor"), id));

		CClipIDs editIds;
		editIds.Add(id);
		m_pMainFrame->ShowEditWnd(editIds);
		return true;
	}

	return false;
}

CString CCP_MainApp::MakeEditFilePath(int id, const CString& extension, int& lastFileCheckId)
{
	CString startingFilePath = StrF(_T("%sEditClip_%d.%s"), CGetSetOptions::GetPath(CGetSetOptions::PathEditClips).GetString(), id, extension.GetString());

	if (id == -1)
	{
		startingFilePath = StrF(_T("%sNewClip_1.%s"), CGetSetOptions::GetPath(CGetSetOptions::PathEditClips).GetString(), extension.GetString());
	}

	CString savePath = startingFilePath;

	//for new files make a unique file name
	if (id < 0 &&
		FileExists(startingFilePath))
	{
		savePath = _T("");

		for (int y = lastFileCheckId; y < 1000000; y++)
		{
			CString testFilePath = StrF(_T("%sNewClip_%d.%s"), CGetSetOptions::GetPath(CGetSetOptions::PathEditClips).GetString(), y, extension.GetString());

			if (FileExists(testFilePath) == FALSE)
			{
				savePath = testFilePath;
				lastFileCheckId = y + 1;
				break;
			}
		}
	}

	return savePath;
}

bool CCP_MainApp::LaunchClipEditor(const CString& exePath, const CString& savePath, int id)
{
	SHELLEXECUTEINFO sei = { sizeof(sei) };
	sei.fMask = SEE_MASK_NOCLOSEPROCESS;
	sei.lpVerb = _T("open");

	if (exePath != _T(""))
	{
		sei.lpFile = exePath;
		sei.lpParameters = savePath;

		CLogger::Log(StrF(_T("Launching editor path: %s, file: %s"), exePath.GetString(), savePath.GetString()));
	}
	else
	{
		sei.lpFile = savePath;

		CLogger::Log(StrF(_T("Launching editor without specific exe path, file: %s"), savePath.GetString()));
	}

	sei.nShow = SW_NORMAL;

	if (ShellExecuteEx(&sei) == FALSE)
	{
		CLogger::Log(StrF(_T("ShellExecuteEx failed, not editing clipid: %d"), id));
		return false;
	}

	/*DWORD PID = GetProcessId(sei.hProcess);

	HANDLE hProcess = sei.hProcess;
	if (m_editThread.IsRunning() == false)
	{
		m_editThread.SubscribeToFileChanges();
		m_editThread.Start();
	}

	m_editThread.WatchFileForChange(savePath, id, hProcess);*/

	return true;
}

void CCP_MainApp::PumpMessageEx(HWND hWnd)
{
	MSG KeyboardMsg;
	while (::PeekMessage(&KeyboardMsg, hWnd, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&KeyboardMsg);
		::DispatchMessage(&KeyboardMsg);
	}
}

HWND CCP_MainApp::QPastehWnd() 
{ 
	if(m_pMainFrame != NULL)
	{
		if(m_pMainFrame->m_quickPaste.m_pwndPaste != NULL)
		{
			return m_pMainFrame->m_quickPaste.m_pwndPaste->GetSafeHwnd();
		}
	}

	return NULL;
}

CQPasteWnd* CCP_MainApp::QPasteWnd() 
{ 
	if(m_pMainFrame != NULL)
	{
		return m_pMainFrame->m_quickPaste.m_pwndPaste.get(); 
	}

	return NULL;
}

bool CCP_MainApp::UACPaste()
{
	if(!m_pUacPasteThread)
	{
		m_pUacPasteThread = std::make_unique<CUAC_Thread>(GetCurrentProcessId());
	}

	return m_pUacPasteThread->UACPaste();
}

bool CCP_MainApp::UACCopy()
{
	if(!m_pUacPasteThread)
	{
		m_pUacPasteThread = std::make_unique<CUAC_Thread>(GetCurrentProcessId());
	}

	return m_pUacPasteThread->UACCopy();
}

bool CCP_MainApp::UACCut()
{
	if(!m_pUacPasteThread)
	{
		m_pUacPasteThread = std::make_unique<CUAC_Thread>(GetCurrentProcessId());
	}

	return m_pUacPasteThread->UACCut();
}

bool CCP_MainApp::UACThreadRunning()
{
	if(m_pUacPasteThread)
	{
		return m_pUacPasteThread->IsRunning();
	}

	return false;
}

void CCP_MainApp::RefreshShowInTaskBar()
{
	if(m_pMainFrame != NULL)
	{
		m_pMainFrame->RefreshShowInTaskBar();
	}
}

void CCP_MainApp::SetActiveGroupId(int groupId)
{
	m_activeGroupId = groupId;
	m_activeGroupStartTime = GetTickCount64();
}

int CCP_MainApp::GetActiveGroupId()
{
	int ret = -1;
	ULONGLONG maxDiff = CGetSetOptions::GetSaveToGroupTimeoutMS();
	ULONGLONG diff = GetTickCount64() - m_activeGroupStartTime;

	if(m_activeGroupId > -1 &&
		diff < maxDiff)
	{
		ret = m_activeGroupId;
	}

	m_activeGroupId = -1;
	m_activeGroupStartTime = 0;

	return ret;
}

void CCP_MainApp::SetCopyReason(CopyReasonEnum::CopyReason copyReason)
{
	m_copyReason = copyReason;
	m_copyReasonStartTime = GetTickCount64();
}

CopyReasonEnum::CopyReason CCP_MainApp::GetCopyReason()
{
	CopyReasonEnum::CopyReason ret = CopyReasonEnum::COPY_TO_UNKOWN;
	ULONGLONG maxDiff = CGetSetOptions::GetCopyReasonTimeoutMS();
	ULONGLONG diff = GetTickCount64() - m_copyReasonStartTime;

	if(m_copyReason != CopyReasonEnum::COPY_TO_UNKOWN &&
		diff < maxDiff)
	{
		ret = m_copyReason;
	}

	m_copyReason = CopyReasonEnum::COPY_TO_UNKOWN;
	m_copyReasonStartTime = 0;

	return ret;
}

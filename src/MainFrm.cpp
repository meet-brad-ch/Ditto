// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "CP_Main.h"
#include "MainFrm.h"
#include "afxole.h"
#include "Misc.h"
#include "CopyProperties.h"
#include ".\mainfrm.h"
#include "Path.h"
#include "DittoCopyBuffer.h"
#include "HotKeys.h"
#include "GlobalClips.h"
#include "OptionsSheet.h"
#include "DeleteClipData.h"
#include "DatabaseUtilities.h"
#include "ErrorReport.h"
#include "ClipboardFormatError.h"

#include <memory>

IMPLEMENT_DYNAMIC(CMainFrame, CFrameWnd)

BEGIN_MESSAGE_MAP(CMainFrame, CFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_FIRST_OPTION, OnFirstOption)
	ON_COMMAND(ID_FIRST_EXIT, OnFirstExit)
	ON_WM_TIMER()
	ON_COMMAND(ID_FIRST_SHOWQUICKPASTE, OnFirstShowquickpaste)
	ON_COMMAND(ID_FIRST_TOGGLECONNECTCV, OnFirstToggleConnectCV)
	ON_UPDATE_COMMAND_UI(ID_FIRST_TOGGLECONNECTCV, OnUpdateFirstToggleConnectCV)
	//}}AFX_MSG_MAP
	ON_MESSAGE(WM_HOTKEY, OnHotKey)
	ON_MESSAGE(CDittoMessage::ShowTrayIcon, OnShowTrayIcon)
	ON_MESSAGE(CDittoMessage::ClipboardCopied, OnClipboardCopied)
	ON_WM_CLOSE()
	ON_MESSAGE(CDittoMessage::ShowOwnedErrorMsg, OnOwnedErrorMsg)
	ON_COMMAND(ID_FIRST_IMPORT, OnFirstImport)
	ON_MESSAGE(CDittoMessage::EditWndClosing, OnEditWndClose)
	ON_WM_DESTROY()
	ON_COMMAND(ID_FIRST_NEWCLIP, OnFirstNewclip)
	ON_MESSAGE(CDittoMessage::SetConnected, OnSetConnected)
	ON_MESSAGE(CDittoMessage::OpenCloseWindow, OnOpenCloseWindow)
	ON_COMMAND(ID_FIRST_GLOBALHOTKEYS, &CMainFrame::OnFirstGlobalhotkeys)
	ON_MESSAGE(CDittoMessage::GlobalClipsClosed, OnGlobalClipsClosed)
	ON_MESSAGE(CDittoMessage::OptionsClosed, OnOptionsClosed)
	ON_MESSAGE(CDittoMessage::ShowOptions, OnShowOptions)
	ON_COMMAND(ID_FIRST_DELETECLIPDATA, &CMainFrame::OnFirstDeleteclipdata)
	ON_MESSAGE(CDittoMessage::DeleteClipsClosed, OnDeleteClipDataClosed)
	ON_COMMAND(ID_FIRST_SAVECURRENTCLIPBOARD, &CMainFrame::OnFirstSavecurrentclipboard)
	ON_MESSAGE(CDittoMessage::SaveClipboard, &CMainFrame::OnSaveClipboardMessage)
	ON_MESSAGE(CDittoMessage::ReaddTaskbarIcon, OnReAddTaskBarIcon)
	ON_MESSAGE(CDittoMessage::ReopenDatabase, &CMainFrame::OnReOpenDatabase)
	ON_MESSAGE(CDittoMessage::ShowMsgWindow, &CMainFrame::OnShowMsgWindow)
	ON_MESSAGE(CDittoMessage::ShowDittoGroup, &CMainFrame::OnShowDittoGroup)
	ON_COMMAND(ID_FIRST_FIXUPSTICKYCLIPORDER, &CMainFrame::OnFirstFixupstickycliporder)
	ON_MESSAGE(WM_DISPLAYCHANGE, &CMainFrame::OnResolutionChange)
	ON_MESSAGE(WmTrayNotify, &CMainFrame::OnTrayNotification)
	ON_MESSAGE(CDittoMessage::PlainTextPaste, &CMainFrame::OnPlainTextPaste)
	ON_WM_WININICHANGE()
	ON_COMMAND(ID_FIRST_SHOWSTARTUPMESSAGE, &CMainFrame::OnFirstShowstartupmessage)
	ON_UPDATE_COMMAND_UI(ID_FIRST_SHOWSTARTUPMESSAGE, &CMainFrame::OnUpdateFirstShowstartupmessage)
	ON_COMMAND(ID_FIRST_BACKUPDATABASE, &CMainFrame::OnFirstBackupdatabase)
	ON_COMMAND(ID_FIRST_RESTOREDATABASE, &CMainFrame::OnFirstRestoredatabase)
	ON_MESSAGE(CDittoMessage::BackupDb, OnBackupDb)
	ON_MESSAGE(CDittoMessage::RestoreDb, OnRestoreDb)
	ON_COMMAND(ID_FIRST_DELETEALLNONUSEDCLIPS, &CMainFrame::OnFirstDeleteallnonusedclips)
	ON_MESSAGE(CDittoMessage::PasteClip, OnPasteClip)
	ON_MESSAGE(CDittoMessage::EditClip, OnEditClip)

	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

	static UINT indicators[] = 
{
	ID_SEPARATOR,  // status line indicator
	ID_INDICATOR_CAPS, ID_INDICATOR_NUM, ID_INDICATOR_SCRL, 
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	m_pEditFrameWnd = NULL;
    m_keyStateModifiers = 0;
    m_startKeyStateTime = 0;
    m_bMovedSelectionMoveKeyState = false;
    m_keyModifiersTimerCount = 0;
	m_doubleClickGroupId = -1;
	m_doubleClickGroupStartTime = 0;
}

CMainFrame::~CMainFrame()
{
    CGetSetOptions::SetMainHWND(0);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
    if(CFrameWnd::OnCreate(lpCreateStruct) ==  - 1)
    {
        return  - 1;
    }

	m_PowerManager.Start(m_hWnd);

    ////Center the main window so message boxes are in the center
    CRect rcScreen = DefaultMonitorRect();
    CPoint cpCenter = rcScreen.CenterPoint();
    MoveWindow(cpCenter.x, cpCenter.x,  1,  1);

	m_startupScreenWidth = GetScreenWidth();
	m_startupScreenHeight = GetScreenHeight();

    //Then set the main window to transparent so it's never shown
    //if it is shown then only the task tray icon
    //m_Transparency.SetTransparent(m_hWnd, 0, true);

    SetWindowText(_T(""));

    CLogger::Log(_T("Setting polling timer to track focus"));
    SetTimer(ActiveWindowTimer,CGetSetOptions::FocusWndTimerTimeout(), 0);

	SetTimer(ReadRandomDbFileTimer, CGetSetOptions::ReadRandomFileInterval() * 1000, 0);

    SetWindowText(_T("Ditto"));
	
	m_trayIcon.Create(this, IDR_MENU, _T("Ditto"), CTrayNotifyIcon::LoadIcon(IDR_MAINFRAME), WmTrayNotify, 0, 1);
	m_trayIcon.SetDefaultMenuItem(ID_FIRST_SHOWQUICKPASTE, FALSE);	    

	//removed to keep Ditto from taking focus on start
    //m_trayIcon.MinimiseToTray(this);

	if (CGetSetOptions::GetShowStartupMessage())
	{
		CString msg = theApp.m_Language.GetString(_T("StartupMsg"), _T("Ditto is running minimized, Ditto can be opened by hot keys or by clicking the task tray icon"));
		m_trayIcon.SetBalloonDetails(msg, _T("Ditto"), CTrayNotifyIcon::BalloonStyle::Info, CGetSetOptions::GetBalloonTimeout());
	}

	theApp.m_Language.UpdateTrayIconRightClickMenu(&m_trayIcon.GetMenu());
	
    //Only if in release
    #ifndef _DEBUG
        {
            //If not showing the icon show it for 40 seconds so they can get to the option
            //in case they can't remember the hot keys or something like that
            if(!(CGetSetOptions::GetShowIconInSysTray()))
            {
                SetTimer(HideIconTimer, 40000, 0);
            }
        }
    #endif 

    //SetTimer(CloseWindowTimer, CMilliseconds::OneHour*24, 0);
	SetTimer(RemoveOldTempFilesTimer, CMilliseconds::OneHour * 6, 0);
    SetTimer(RemoveOldEntriesTimer, CMilliseconds::OneMinute*15, 0);
	SetTimer(CloseNoDbWindowTimer, 10000, 0);

	//found on some computers GetTickCount gettickcount returns a smaller value than other, can't explain
	//check here to see if we need to make an adjustment
	IdleSeconds();

    m_ulCopyGap = CGetSetOptions::GetCopyGap();

    if (!theApp.AfterMainCreate())
    {
        return -1;  // the failure is reported; CCP_MainApp::CreateMainWnd stops the start
    }

    m_thread.Start(this);

    return 0;
}

LRESULT CMainFrame::OnPlainTextPaste(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	DoTextOnlyPaste();
	return 1;
}

LRESULT CMainFrame::OnTrayNotification(WPARAM wParam, LPARAM lParam)
{
	if (WM_MOUSEFIRST <= LOWORD(lParam) && LOWORD(lParam) <= WM_MOUSELAST)
	{
		theApp.m_activeWnd.TrackActiveWnd(true);
	}

	//click on balloon
	if (lParam == 0x405)
	{
		SetTimer(DelayedShowDittoTimer, 100, NULL);		
	}
	
	m_trayIcon.OnTrayNotification(wParam, lParam);
	return 0L;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT &cs)
{
    if(cs.hMenu != NULL)
    {
        ::DestroyMenu(cs.hMenu); // delete menu if loaded
        cs.hMenu = NULL; // no menu for this window
    }

    if(!CFrameWnd::PreCreateWindow(cs))
    {
        return FALSE;
    }

    WNDCLASS wc;
    wc.style = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = AfxWndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = AfxGetInstanceHandle();
    wc.hIcon = NULL;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = _T("Ditto");

    // Create the QPaste window class
    if(!AfxRegisterClass(&wc))
    {
        return FALSE;
    }

    cs.lpszClass = wc.lpszClassName;

    return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
    void CMainFrame::AssertValid()const
    {
        CFrameWnd::AssertValid();
    }

    void CMainFrame::Dump(CDumpContext &dc)const
    {
        CFrameWnd::Dump(dc);
    }

#endif //_DEBUG

/////////////////////////////////////////////////////////////////////////////
// CMainFrame message handlers




void CMainFrame::OnFirstExit()
{
    this->SendMessage(WM_CLOSE, 0, 0);
}

LRESULT CMainFrame::OnHotKey(WPARAM wParam, LPARAM /*lParam*/)
{
	if (IsShowDittoHotKey(wParam))
	{
		OnShowDittoHotKey();
		return TRUE;
	}

	if (DoFirstTenHotKey(wParam) || DoCopyBufferHotKey(wParam))
	{
		return TRUE;
	}

	if (IsHotKey(theApp.m_pTextOnlyPaste, wParam))
	{
		DoTextOnlyPaste();
	}
	else if (IsHotKey(theApp.m_pSaveClipboard, wParam))
	{
		OnFirstSavecurrentclipboard();
	}
	else if (IsHotKey(theApp.m_pCopyAndSaveClipboard, wParam))
	{
		DoCopyAndSaveClipboard();
	}
	else
	{
		DoGlobalClipHotKey(wParam);
	}

    return TRUE;
}

bool CMainFrame::IsHotKey(const CHotKey* hotKey, WPARAM wParam)
{
	return hotKey && wParam == hotKey->m_Atom;
}

bool CMainFrame::IsShowDittoHotKey(WPARAM wParam)
{
	return IsHotKey(theApp.m_pDittoHotKey, wParam) ||
		IsHotKey(theApp.m_pDittoHotKey2, wParam) ||
		IsHotKey(theApp.m_pDittoHotKey3, wParam);
}

void CMainFrame::OnShowDittoHotKey()
{
    //If they still have the shift/ctrl keys down
    if(m_keyStateModifiers != 0 && m_quickPaste.IsWindowVisibleEx())
    {
        CLogger::Log(_T("On Show Ditto HotKey, key state modifiers are still down, moving selection"));

        if(m_bMovedSelectionMoveKeyState == false)
        {
            CLogger::Log(_T("Setting flag m_bMovedSelectionMoveKeyState to true, will paste when modifier keys are up"));
        }

        m_quickPaste.MoveSelection(true);
        m_bMovedSelectionMoveKeyState = true;
    }
    else if(CGetSetOptions::m_HideDittoOnHotKeyIfAlreadyShown && m_quickPaste.IsWindowTopLevel() && CGetSetOptions::GetShowPersistent() == FALSE)
    {
        CLogger::Log(_T("On Show Ditto HotKey, window is already visible, hiding window"));
        m_quickPaste.HideQPasteWnd();
    }
    else
    {
        CLogger::Log(_T("On Show Ditto HotKey, showing window"));

		StartKeyModifierTimer();

		ShowQPasteWithActiveWindowCheck();
    }

    //KillTimer(CloseWindowTimer);
    //SetTimer(CloseWindowTimer, CMilliseconds::OneHour *24, 0);
}

bool CMainFrame::DoFirstTenHotKey(WPARAM wParam)
{
	const std::array<CHotKey*, 10> positions{ theApp.m_pPosOne, theApp.m_pPosTwo, theApp.m_pPosThree, theApp.m_pPosFour, theApp.m_pPosFive,
		theApp.m_pPosSix, theApp.m_pPosSeven, theApp.m_pPosEight, theApp.m_pPosNine, theApp.m_pPosTen };

	for (int pos = 0; pos < static_cast<int>(positions.size()); pos++)
	{
		if (IsHotKey(positions[pos], wParam))
		{
			CLogger::Log(StrF(_T("Pos %d hot key"), pos + 1));
			DoFirstTenPositionsPaste(pos);
			return true;
		}
	}

	return false;
}

bool CMainFrame::DoCopyBufferHotKey(WPARAM wParam)
{
	const std::array<CopyBufferHotKeys, 5> buffers{ {
		{ theApp.m_pCopyBuffer1, theApp.m_pPasteBuffer1, theApp.m_pCutBuffer1 },
		{ theApp.m_pCopyBuffer2, theApp.m_pPasteBuffer2, theApp.m_pCutBuffer2 },
		{ theApp.m_pCopyBuffer3, theApp.m_pPasteBuffer3, theApp.m_pCutBuffer3 },
		{ theApp.m_pCopyBuffer4, theApp.m_pPasteBuffer4, theApp.m_pCutBuffer4 },
		{ theApp.m_pCopyBuffer5, theApp.m_pPasteBuffer5, theApp.m_pCutBuffer5 },
	} };

	for (int buffer = 0; buffer < static_cast<int>(buffers.size()); buffer++)
	{
		if (DoCopyBufferHotKey(buffers[buffer], buffer, wParam))
		{
			return true;
		}
	}

	return false;
}

bool CMainFrame::DoCopyBufferHotKey(const CopyBufferHotKeys& hotKeys, int buffer, WPARAM wParam)
{
	if (IsHotKey(hotKeys.copy, wParam))
	{
		CLogger::Log(StrF(_T("Copy buffer %d hot key"), buffer + 1));
		theApp.m_CopyBuffer.StartCopy(buffer);
		return true;
	}

	if (IsHotKey(hotKeys.paste, wParam))
	{
		CLogger::Log(StrF(_T("Paste buffer %d hot key"), buffer + 1));
		theApp.m_CopyBuffer.PastCopyBuffer(buffer);
		return true;
	}

	if (IsHotKey(hotKeys.cut, wParam))
	{
		CLogger::Log(StrF(_T("Cut buffer %d hot key"), buffer + 1));
		theApp.m_CopyBuffer.StartCopy(buffer, true);
		return true;
	}

	return false;
}

void CMainFrame::DoCopyAndSaveClipboard()
{
	CLogger::Log(StrF(_T("START of copy and save clipboard, sending copy")));

	theApp.m_activeWnd.SendCopy(CopyReasonEnum::COPY_TO_UNKOWN);

	int delay = CGetSetOptions::GetCopyAndSveDelay();
	CLogger::Log(StrF(_T("Copy and save clipboard, sending copy, delaying %dms before saving clipboard"), delay));
	Sleep(delay);

	CLogger::Log(StrF(_T("Copy and save clipboard, saving clipboard")));
	OnFirstSavecurrentclipboard();

	CLogger::Log(StrF(_T("END of copy and save clipboard")));
}

void CMainFrame::DoGlobalClipHotKey(WPARAM wParam)
{
	for(int i = 0; i < g_HotKeys.GetCount(); i++)
	{
		if(g_HotKeys[i] != NULL &&
			g_HotKeys[i]->m_Atom == wParam &&
			g_HotKeys[i]->m_clipId > 0)
		{
			if(g_HotKeys[i]->m_hkType == CHotKey::PASTE_OPEN_CLIP)
			{
				CLogger::Log(StrF(_T("Pasting clip from global shortcut, clipId: %d"), g_HotKeys[i]->m_clipId));
				PasteOrShowGroup(g_HotKeys[i]->m_clipId, -1, FALSE, TRUE, false);
			}
			else if(g_HotKeys[i]->m_hkType == CHotKey::MOVE_TO_GROUP)
			{
				CLogger::Log(StrF(_T("Global hot key to save clip to group Id: %d, Sending copy to save selection to this group"), g_HotKeys[i]->m_clipId));

				KillTimer(GroupDoubleClickTimer);
				m_doubleClickGroupId = -1;
				m_doubleClickGroupStartTime = 0;

				theApp.SetActiveGroupId(g_HotKeys[i]->m_clipId);
				theApp.m_activeWnd.SendCopy(CopyReasonEnum::COPY_TO_GROUP);
			}

			break;
		}
	}
}

void CMainFrame::ShowQPasteWithActiveWindowCheck()
{
	//Before we show our window find the current focused window for paste into
	theApp.m_activeWnd.TrackActiveWnd(true);

	if (CGetSetOptions::GetOpenToGroupByActiveExe() &&
		theApp.m_activeWnd.ActiveWnd() != NULL)
	{
		CString exeName = GetProcessName(theApp.m_activeWnd.ActiveWnd());
		if (exeName != _T(""))
		{
			theApp.TryEnterOldGroupState();
			CString query = StrF(_T("SELECT lID FROM Main WHERE bIsGroup = 1 AND mText = '%s' COLLATE NOCASE"), exeName.GetString());
			CppSQLite3Query q = theApp.m_db.execQueryEx(query);
			if (q.eof() == false)
			{
				int groupId = q.getIntField(_T("lID"));
				//this will revert back to the old group on hide of ditto
				theApp.EnterGroupID(groupId, TRUE, TRUE);

				CLogger::Log(StrF(_T("Opening Ditto to Group based on found group name, name: %s, GroupId: %d"), exeName.GetString(), groupId));
			}
			else
			{
				theApp.TryEnterOldGroupState();
			}
		}
	}

	m_quickPaste.ShowQPasteWnd(this, false, true, FALSE);
}

void CMainFrame::DoTextOnlyPaste()
{
	CClipboardSaveRestore textOnlyPaste;

	CLogger::Log(_T("Text Only paste, saving clipboard to be restored later"));
	textOnlyPaste.Save(TRUE);

	CLogger::Log(_T("Text Only paste, Add cf_text or cf_unicodetext to clipboard"));
	try
	{
		if (!textOnlyPaste.RestoreTextOnly())
		{
			CErrorReport::Show(_T("Text only paste stopped: the clipboard could not be opened."));
			return;
		}
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CErrorReport::Show(StrF(_T("Text only paste stopped: the clipboard data is malformed (%s)."), CString(error.what()).GetString()));
		return;
	}

	DWORD pasteDelay = CGetSetOptions::GetTextOnlyPasteDelay();

	CLogger::Log(StrF(_T("Text Only paste, delaying %d ms before sending paste"), pasteDelay));

	Sleep(pasteDelay);

	CLogger::Log(_T("Text Only paste, Sending paste"));
	theApp.m_activeWnd.SendPaste(false);

	CLogger::Log(_T("Text Only paste, Post sending paste"));
}

void CMainFrame::DoFirstTenPositionsPaste(int nPos)
{
    try
	{
		CString csSort = _T("");
		CString strFilter = _T("");
		bool pastedFromGroup = false;

		if (theApp.m_GroupID < 0 ||
			CGetSetOptions::GetUseUISelectedGroupForLastTenCopies() == FALSE)
		{
			//do not change this this directly relates to the views in the Main table
			csSort = "Main.bIsGroup ASC, "
				"Main.stickyClipOrder DESC, "
				"Main.clipOrder DESC";

			if (CGetSetOptions::m_bShowAllClipsInMainList)
			{
				if (CGetSetOptions::GetShowGroupsInMainList())
				{
					//found to be slower on large databases
					strFilter = "((Main.bIsGroup = 1 AND Main.lParentID = -1) OR Main.bIsGroup = 0)";
				}
				else
				{
					strFilter = "(Main.bIsGroup = 0)";
				}
			}
			else
			{
				strFilter = "((Main.bIsGroup = 1 AND Main.lParentID = -1) OR (Main.bIsGroup = 0 AND Main.lParentID = -1))";
			}
		}
		else
		{
			pastedFromGroup = true;

			//do not change this this directly relates to the views in the Main table
			csSort = "Main.bIsGroup ASC, "
				"Main.stickyClipGroupOrder DESC, "
				"Main.clipGroupOrder DESC";
			
			if (theApp.m_GroupID >= 0)
			{
				strFilter.Format(_T("Main.lParentID = %d"), theApp.m_GroupID);
			}
		}

		CString query = StrF(_T("SELECT lID, bIsGroup FROM Main WHERE %s ORDER BY %s LIMIT 1 OFFSET %d"), strFilter.GetString(), csSort.GetString(), nPos);

		CLogger::Log(StrF(_T("Doing Last Ten Paste, Index: %d Query: %s"), nPos, query.GetString()));

		CppSQLite3Query q = theApp.m_db.execQueryEx(query);

        if(q.eof() == false)
        {
			PasteOrShowGroup(q.getIntField(_T("lID")), CGetSetOptions::GetMoveClipsOnGlobal10(), false, CGetSetOptions::m_bSendPasteOnFirstTenHotKeys, pastedFromGroup);
        }
    }
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Pasting the clip at position %d failed: %s"), nPos, e.errorMessage()));
		return;
	}
}

void CMainFrame::StartKeyModifierTimer()
{
	if (CGetSetOptions::m_moveSelectionOnOpenHotkey)
	{
		m_keyModifiersTimerCount = 0;
		m_bMovedSelectionMoveKeyState = false;
		m_startKeyStateTime = GetTickCount64();
		m_keyStateModifiers = CAccels::GetKeyStateModifiers();
		SetTimer(KeyStateModifiersTimer, 50, NULL);
	}
}

void CMainFrame::PasteOrShowGroup(int dbId, BOOL updateClipTime, BOOL activeTarget, BOOL sendPaste, bool pastedFromGroup)
{
	try
	{
		bool isGroup = false;
		CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT bIsGroup FROM Main WHERE lID = %d"), dbId);
		if(q.eof() == false)
		{
			if(q.getIntField(_T("bIsGroup")) > 0)
			{
				isGroup = true;
			}
		}
		
		if(isGroup)
		{
			ULONGLONG maxDiff = static_cast<ULONGLONG>(CGetSetOptions::GetGroupDoubleClickTimeMS());
			ULONGLONG diff = GetTickCount64() - m_doubleClickGroupStartTime;

			if(m_doubleClickGroupId == dbId &&
				diff < maxDiff)
			{
				CLogger::Log(StrF(_T("Second Press of group hot key, group Id: %d, Sending copy to save selection to this group"), dbId));
				
				KillTimer(GroupDoubleClickTimer);
				m_doubleClickGroupId = -1;
				m_doubleClickGroupStartTime = 0;

				theApp.SetActiveGroupId(dbId);
				theApp.m_activeWnd.SendCopy(CopyReasonEnum::COPY_TO_GROUP);
			}
			else
			{
				m_doubleClickGroupId = dbId;
				m_doubleClickGroupStartTime = GetTickCount64();

				int doubleClickTime = CGetSetOptions::GetGroupDoubleClickTimeMS();

				SetTimer(GroupDoubleClickTimer, doubleClickTime, 0);

				CLogger::Log(StrF(_T("First Press of group hot key, group Id: %d, timeout: %d"), dbId, doubleClickTime));
			}
		}
		else
		{
			PasteSingleClip(dbId, updateClipTime, activeTarget, sendPaste, pastedFromGroup);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Pasting clip or group id %d failed: %s"), dbId, e.errorMessage()));
		return;
	}
}

void CMainFrame::PasteSingleClip(int dbId, BOOL updateClipTime, BOOL activeTarget, BOOL sendPaste, bool pastedFromGroup)
{
	KillTimer(GroupDoubleClickTimer);
	m_doubleClickGroupId = -1;
	m_doubleClickGroupStartTime = 0;

	BOOL bItWas{CGetSetOptions::m_bUpdateTimeOnPaste};
	if (updateClipTime != -1)
	{
		CGetSetOptions::m_bUpdateTimeOnPaste = updateClipTime;
	}

	CProcessPaste paste{};
	paste.m_pastedFromGroup = pastedFromGroup;
	paste.GetClipIDs().Add(dbId);

	if (activeTarget != -1)
	{
		paste.m_bActivateTarget = activeTarget ? true : false;;
	}

	if (sendPaste != -1)
	{
		paste.m_bSendPaste = sendPaste ? true : false;
	}
	paste.DoPaste();
	theApp.OnPasteCompleted();

	if (updateClipTime != -1)
	{
		CGetSetOptions::m_bUpdateTimeOnPaste = bItWas;
	}
}

void CMainFrame::DoDittoCopyBufferPaste(int nCopyBuffer)
{
    try
    {
        CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT lID FROM Main WHERE CopyBuffer = %d"), nCopyBuffer);

        if(q.eof() == false)
        {
            //Don't move these to the top
            BOOL bItWas = CGetSetOptions::m_bUpdateTimeOnPaste;
            CGetSetOptions::m_bUpdateTimeOnPaste = FALSE;

            CProcessPaste paste;
            paste.GetClipIDs().Add(q.getIntField(_T("lID")));
            paste.m_bActivateTarget = false;
            paste.DoPaste();
            theApp.OnPasteCompleted();

            CGetSetOptions::m_bUpdateTimeOnPaste = bItWas;
        }
    }
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Pasting Ditto copy buffer %d failed: %s"), nCopyBuffer, e.errorMessage()));
		return;
	}
}

// CloseWindowTimer has no entry: its handler does nothing (closing the window on it is disabled)
const std::array<CMainFrame::TimerHandler, 11> CMainFrame::s_timerHandlers{ {
	{ HideIconTimer, &CMainFrame::OnHideIconTimer },
	{ RemoveOldEntriesTimer, &CMainFrame::OnRemoveOldEntriesTimer },
	{ RemoveOldTempFilesTimer, &CMainFrame::OnRemoveOldTempFilesTimer },
	{ KeyStateModifiersTimer, &CMainFrame::OnKeyStateModifiersTimer },
	{ ActiveWindowTimer, &CMainFrame::OnActiveWindowTimer },
	{ ReadRandomDbFileTimer, &CMainFrame::OnReadRandomDbFileTimer },
	{ GroupDoubleClickTimer, &CMainFrame::OnGroupDoubleClickTimer },
	{ ScreenResolutionChangedTimer, &CMainFrame::OnScreenResolutionChangedTimer },
	{ DelayedShowDittoTimer, &CMainFrame::OnDelayedShowDittoTimer },
	{ SetWindowsThemeTimer, &CMainFrame::OnSetWindowsThemeTimer },
	{ CloseNoDbWindowTimer, &CMainFrame::OnCloseNoDbWindowTimer },
} };

void CMainFrame::OnTimer(UINT_PTR nIDEvent)
{
	for (const TimerHandler& handler : s_timerHandlers)
	{
		if (handler.timerId == nIDEvent)
		{
			(this->*handler.handle)();
			break;
		}
	}

    CFrameWnd::OnTimer(nIDEvent);
}

void CMainFrame::OnHideIconTimer()
{
	KillTimer(HideIconTimer);
	if (!CGetSetOptions::GetShowIconInSysTray())
	{
		m_trayIcon.Hide();
	}
}

void CMainFrame::OnRemoveOldEntriesTimer()
{
	m_thread.FireDeleteEntries();
}

void CMainFrame::OnRemoveOldTempFilesTimer()
{
	m_thread.FireRemoveTempFiles();
}

void CMainFrame::OnKeyStateModifiersTimer()
{
    m_keyModifiersTimerCount++;
    if(m_keyStateModifiers != 0)
    {
        BYTE keyState = CAccels::GetKeyStateModifiers();
        //Have they release the key state modifiers yet(ctrl, shift, alt)
        if((m_keyStateModifiers &keyState) == 0)
        {
            KillTimer(KeyStateModifiersTimer);
            long waitTime = static_cast<long>(GetTickCount64() - m_startKeyStateTime);

            if(m_bMovedSelectionMoveKeyState || m_keyModifiersTimerCount > CGetSetOptions::GetKeyStateWaitTimerCount())
            {
                CLogger::Log(StrF(_T("Timer KEY_STATE_MODIFIERS timeout count hit(%d), count (%d), time (%d), Move Selection from Modifer (%d) sending paste"), CGetSetOptions::GetKeyStateWaitTimerCount(), m_keyModifiersTimerCount, waitTime, m_bMovedSelectionMoveKeyState));
                m_quickPaste.OnKeyStateUp();
            }
            else
            {
                CLogger::Log(StrF(_T("Timer KEY_STATE_MODIFIERS count NOT hit(%d), count (%d) time (%d)"), CGetSetOptions::GetKeyStateWaitTimerCount(), m_keyModifiersTimerCount, waitTime));
                m_quickPaste.SetKeyModiferState(false);
            }

            m_keyStateModifiers = 0;
            m_keyModifiersTimerCount = 0;
            m_bMovedSelectionMoveKeyState = 0;
        }
    }
    else
    {
        KillTimer(KeyStateModifiersTimer);
    }
}

void CMainFrame::OnActiveWindowTimer()
{
	if(theApp.m_bShowingQuickPaste)
	{
		theApp.m_activeWnd.TrackActiveWnd(false);
	}
}

void CMainFrame::OnReadRandomDbFileTimer()
{
	m_thread.FireReadDbFile();
}

void CMainFrame::OnGroupDoubleClickTimer()
{
	KillTimer(GroupDoubleClickTimer);

	CLogger::Log(StrF(_T("Processing single click of groupId %d in timer, opening ditto to this group"), m_doubleClickGroupId));

	ULONGLONG maxDiff = static_cast<ULONGLONG>(CGetSetOptions::GetGroupDoubleClickTimeMS() * 1.5);
	ULONGLONG diff = GetTickCount64() - m_doubleClickGroupStartTime;

	if(diff < maxDiff)
	{
		if(m_doubleClickGroupId > -1)
		{
			if (theApp.EnterGroupID(m_doubleClickGroupId, FALSE, TRUE))
			{
				theApp.m_activeWnd.TrackActiveWnd(true);
				StartKeyModifierTimer();
				m_quickPaste.ShowQPasteWnd(this, false, true, FALSE);
			}
		}
	}
	else
	{
		CLogger::Log(StrF(_T("Something happened and we didn't process the group timer in time, Id: %d, Diff ms: %d, maxDiff: %d"), m_doubleClickGroupId, diff, maxDiff));
	}

	m_doubleClickGroupId = -1;
	m_doubleClickGroupStartTime = 0;
}

void CMainFrame::OnScreenResolutionChangedTimer()
{
	KillTimer(ScreenResolutionChangedTimer);
	m_quickPaste.OnScreenResolutionChange();
}

void CMainFrame::OnDelayedShowDittoTimer()
{
	KillTimer(DelayedShowDittoTimer);
	m_quickPaste.ShowQPasteWnd(this, false, false, FALSE);
}

void CMainFrame::OnSetWindowsThemeTimer()
{
	KillTimer(SetWindowsThemeTimer);
	auto theme = CGetSetOptions::GetTheme();
	if (theme == _T(""))
	{
		CGetSetOptions::m_Theme.Load(theme);

		auto visible = m_quickPaste.IsWindowVisibleEx();
		m_quickPaste.CloseQPasteWnd();

		if (visible)
		{
			m_quickPaste.ShowQPasteWnd(this, true, false, true);
		}
	}
}

void CMainFrame::OnCloseNoDbWindowTimer()
{
	KillTimer(CloseNoDbWindowTimer);
	theApp.CloseNoDbWindow();
}

LRESULT CMainFrame::OnShowTrayIcon(WPARAM wParam, LPARAM lParam)
{
    if(lParam)
    {
        if(!m_trayIcon.IsHidden())
        {
            KillTimer(HideIconTimer);
            SetTimer(HideIconTimer, 40000, 0);
        }
    }

    if(wParam)
    {
		m_trayIcon.Show();
    }
    else
    {
        m_trayIcon.Hide();
    }

    return TRUE;
}

void CMainFrame::OnFirstShowquickpaste()
{
    m_quickPaste.ShowQPasteWnd(this, false, false, FALSE);
}

void CMainFrame::OnFirstToggleConnectCV()
{
    theApp.ToggleConnectCV();
}

void CMainFrame::OnUpdateFirstToggleConnectCV(CCmdUI *pCmdUI)
{
    theApp.UpdateMenuConnectCV(pCmdUI->m_pMenu, ID_FIRST_TOGGLECONNECTCV);
}

LRESULT CMainFrame::OnClipboardCopied(WPARAM wParam, LPARAM /*lParam*/)
{
	CLogger::Log(_T("Start of function OnClipboardCopied, adding clip to thread for processing"));

	// retakes the clip released by the sender (CCopyThread::OnClipboardChange or OnFirstSavecurrentclipboard)
	std::unique_ptr<CClip> clip{reinterpret_cast<CClip*>(wParam)};
	if(clip)
	{
		m_thread.AddClipToSave(std::move(clip));
	}
    
    CLogger::Log(_T("End of function OnClipboardCopied"));	
    return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG *pMsg)
{
	//forward the mouse wheel onto the window under the cursor
	//normally windows only sends it to the window with focus, bypass this
	if ((pMsg->message == WM_MOUSEWHEEL || pMsg->message == WM_MOUSEHWHEEL) &&
		::GetCapture() == nullptr)
	{
		POINT mouse;
		GetCursorPos(&mouse);
		HWND hwndFromPoint = ::WindowFromPoint(mouse);

		if (pMsg->hwnd != hwndFromPoint)
		{
			DWORD winProcessId = 0;
			::GetWindowThreadProcessId(hwndFromPoint, &winProcessId);
			if (winProcessId == ::GetCurrentProcessId()) //no-fail!
			{
				pMsg->hwnd = hwndFromPoint;				
			}
		}

		if (GetKeyState(VK_SHIFT) & 0x8000)
			pMsg->message = WM_MOUSEHWHEEL;
	}

    return CFrameWnd::PreTranslateMessage(pMsg);
}

void CMainFrame::OnClose()
{
	if (m_pEditFrameWnd)
	{
		if (m_pEditFrameWnd->CloseAll() == false)
		{
			return;
		}
	}

    CloseAllOpenDialogs();

    CLogger::Log(_T("OnClose - before stop MainFrm thread"));
    m_thread.Stop();
    CLogger::Log(_T("OnClose - after stop MainFrm thread"));

    theApp.BeforeMainClose();

	m_PowerManager.Close();

    CFrameWnd::OnClose();
}

bool CMainFrame::CloseAllOpenDialogs()
{
    bool bRet = false;
    DWORD dwordProcessId;
    DWORD dwordChildWindowProcessId;
    GetWindowThreadProcessId(this->m_hWnd, &dwordProcessId);
    ASSERT(dwordProcessId);

	CArray<CWnd*, CWnd*> openDialogs;

    CWnd *pTempWnd = GetDesktopWindow()->GetWindow(GW_CHILD);
    while((pTempWnd = pTempWnd->GetWindow(GW_HWNDNEXT)) != NULL)
    {
        if(pTempWnd->GetSafeHwnd() == NULL)
        {
            break;
        }

        GetWindowThreadProcessId(pTempWnd->GetSafeHwnd(), &dwordChildWindowProcessId);
        if(dwordChildWindowProcessId == dwordProcessId)
        {
            TCHAR szTemp[100];
            GetClassName(pTempWnd->GetSafeHwnd(), szTemp, 100);

            // #32770 is class name for dialogs so don't process the message if it is a dialog
            if(_tcscmp(szTemp, _T("#32770")) == 0)
            {
				openDialogs.Add(pTempWnd);                
                bRet = true;
            }
        }
    }

	for (int i = 0; i < openDialogs.GetCount(); i++)
	{
		openDialogs[i]->PostMessage(WM_CLOSE, 0, 0);
	}

    MSG msg;
    while(PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return bRet;
}

LRESULT CMainFrame::OnOwnedErrorMsg(WPARAM wParam, LPARAM /*lParam*/)
{
	const std::unique_ptr<CString> message(reinterpret_cast<CString*>(wParam));

	ShowErrorMessage(_T("Ditto"), *message);

	return TRUE;
}

CString WndName(HWND hParent)
{
    TCHAR cWindowText[200];

    ::GetWindowText(hParent, cWindowText, 100);

    int nCount = 0;

    while(_tcslen(cWindowText) <= 0)
    {
        hParent = ::GetParent(hParent);
        if(hParent == NULL)
        {
            break;
        }

        ::GetWindowText(hParent, cWindowText, 100);

        nCount++;
        if(nCount > 100)
        {
            CLogger::Log(_T("GetTargetName reached maximum search depth of 100"));
            break;
        }
    }

    return cWindowText;
}

void CMainFrame::ShowEditWnd(CClipIDs& Ids)
{
	CWaitCursor wait;

	bool bCreatedWindow = false;
	if (m_pEditFrameWnd == NULL)
	{
		// MFC's DYNCREATE factory (the destructor is protected): the frame deletes itself in PostNcDestroy
		// (also when LoadFrame fails); m_pEditFrameWnd only observes it until CDittoMessage::EditWndClosing
		m_pEditFrameWnd = static_cast<CEditFrameWnd*>(CEditFrameWnd::CreateObject());
		if (m_pEditFrameWnd == NULL)
		{
			CErrorReport::Show(_T("Opening the edit window failed (the window could not be allocated)."));
			return;
		}
		if (!m_pEditFrameWnd->LoadFrame(IDR_MAINFRAME))
		{
			// the failed frame is already deleted
			m_pEditFrameWnd = NULL;
			CErrorReport::Show(_T("Opening the edit window failed."));
			return;
		}
		bCreatedWindow = true;
	}
	if (m_pEditFrameWnd)
	{
		m_pEditFrameWnd->EditIds(Ids);
		m_pEditFrameWnd->SetNotifyWnd(m_hWnd);

		if (bCreatedWindow)
		{
			CSize sz;
			CPoint pt;
			CGetSetOptions::GetEditWndSize(sz);
			CGetSetOptions::GetEditWndPoint(pt);
			CRect cr(pt, sz);
			EnsureWindowVisible(&cr);
			m_pEditFrameWnd->MoveWindow(cr);
		}

		m_pEditFrameWnd->ShowWindow(SW_SHOW);
		m_pEditFrameWnd->SetForegroundWindow();
		m_pEditFrameWnd->SetFocus();
	}
}

LRESULT CMainFrame::OnEditWndClose(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_pEditFrameWnd = NULL;
	return TRUE;
}

void CMainFrame::ShowErrorMessage(CString csTitle, CString csMessage)
{
    CLogger::Log(StrF(_T("ShowErrorMessage %s - %s"), csTitle.GetString(), csMessage.GetString()));
	m_trayIcon.SetBalloonDetails(csMessage, csTitle, CTrayNotifyIcon::BalloonStyle::Error, CGetSetOptions::GetBalloonTimeout());
}

void CMainFrame::OnFirstImport()
{
    theApp.ImportClips(theApp.m_MainhWnd);
}

LRESULT CMainFrame::OnSetConnected(WPARAM wParam, LPARAM lParam)
{
    if(wParam)
    {
        theApp.SetConnectCV(true);		
    }
    else if(lParam)
    {
        theApp.SetConnectCV(false);		
    }

    return TRUE;
}

LRESULT CMainFrame::OnOpenCloseWindow(WPARAM wParam, LPARAM lParam)
{
	if(wParam)
	{
		StartKeyModifierTimer();
		ShowQPasteWithActiveWindowCheck();
	}
	else if(lParam)
	{
		m_quickPaste.HideQPasteWnd();
	}

	return TRUE;
}

void CMainFrame::OnDestroy()
{
	if (m_pEditFrameWnd)
	{
		m_pEditFrameWnd->DestroyWindow();
	}

    CFrameWnd::OnDestroy();
}

void CMainFrame::OnFirstNewclip()
{
    CClipIDs IDs;
    IDs.Add( - 1);
    theApp.EditItems(IDs, true, true);
}

void CMainFrame::OnFirstOption()
{
	if(m_pOptions)
	{
		::SetForegroundWindow(m_pOptions->m_hWnd);
	}
	else
	{
		auto options{std::make_unique<COptionsSheet>(_T(""))};
		options->SetNotifyWnd(m_hWnd);
		m_pOptions = std::move(options);

		m_pOptions->Create();
		m_pOptions->ShowWindow(SW_SHOW);
	}
}

void CMainFrame::OnFirstGlobalhotkeys()
{
	if(m_pGlobalClips)
	{
		::SetForegroundWindow(m_pGlobalClips->m_hWnd);
	}
	else
	{
		auto globalClips{std::make_unique<GlobalClips>()};
		globalClips->SetNotifyWnd(m_hWnd);
		m_pGlobalClips = std::move(globalClips);

		CAlphaBlend tran;
		tran.SetTransparent(m_hWnd, 0, 1);

		m_pGlobalClips->Create(IDD_GLOBAL_CLIPS, NULL);
		m_pGlobalClips->ShowWindow(SW_SHOW);
	}
}

LRESULT CMainFrame::OnShowOptions(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	OnFirstOption();
	return 0;
}

LRESULT CMainFrame::OnOptionsClosed(WPARAM wParam, LPARAM /*lParam*/)
{
	BOOL themeChanged = (BOOL)wParam;
	m_trayIcon.MinimiseToTray(this);
	CAlphaBlend tran;
	tran.SetTransparent(m_hWnd, 255, 0);

	m_pOptions.reset();

	if (themeChanged)
	{
		CGetSetOptions::m_Theme.Load(CGetSetOptions::GetTheme());

		m_quickPaste.CloseQPasteWnd();
	}
	else
	{
		if (m_quickPaste.m_pwndPaste != NULL)
		{
			m_quickPaste.m_pwndPaste->PostMessage(CQListCtrl::NmPostOptionsWindow);
		}
	}

	m_trayIcon.SetMenu(NULL, IDR_MENU);
	theApp.m_Language.UpdateTrayIconRightClickMenu(&m_trayIcon.GetMenu());

	if (CGetSetOptions::GetShowIconInSysTray())
	{
		m_trayIcon.Show();
	}
	else
	{
		m_trayIcon.Hide();
	}
	

	return 0;
}

LRESULT CMainFrame::OnGlobalClipsClosed(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_trayIcon.MinimiseToTray(this);
	CAlphaBlend tran;
	tran.SetTransparent(m_hWnd, 255, 0);

	m_pGlobalClips.reset();

	return 0;
}

void CMainFrame::RefreshShowInTaskBar()
{
	BOOL windowVisible = m_quickPaste.IsWindowVisibleEx();

	m_quickPaste.CloseQPasteWnd();

	if (windowVisible)
	{
		m_quickPaste.ShowQPasteWnd(this, true, false, true);
	}
}

LRESULT CMainFrame::OnDeleteClipDataClosed(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	m_trayIcon.MinimiseToTray(this);
	CAlphaBlend tran;
	tran.SetTransparent(m_hWnd, 255, 0);

	m_pDeleteClips.reset();

	return 0;
}

void CMainFrame::OnFirstDeleteclipdata()
{
	//this->ShowWindow(SW_HIDE);
	if (m_pDeleteClips)
	{
		::SetForegroundWindow(m_pDeleteClips->m_hWnd);
	}
	else
	{
		auto deleteClips{std::make_unique<CDeleteClipData>()};
		deleteClips->SetNotifyWnd(m_hWnd);
		m_pDeleteClips = std::move(deleteClips);

		CAlphaBlend tran;
		tran.SetTransparent(m_hWnd, 0, 1);

		m_pDeleteClips->Create(IDD_DELETE_CLIP_DATA, NULL);
		m_pDeleteClips->ShowWindow(SW_SHOW);
	}
}

LRESULT CMainFrame::OnSaveClipboardMessage(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	OnFirstSavecurrentclipboard();
	return TRUE;
}

void CMainFrame::OnFirstSavecurrentclipboard()
{
	CLogger::Log(_T("Start Saving the current clipboard to the database"));
	const std::unique_ptr<CClipTypes> types(theApp.LoadTypesFromDB());
	if(!types)
	{
		CLogger::Log(_T("Failed to load supported types from the db, not saving to the db"));
		return;
	}

	auto clip = std::make_unique<CClip>();
	try
	{
		if(!clip->LoadFromClipboard(types.get(), CGetSetOptions::m_regexHelper, false, _T("")))
		{
			CLogger::Log(_T("Failed to load clips from the clipboard, not saving to db"));
			return;
		}
	}
	catch(const DittoCore::ClipboardFormatError& error)
	{
		CErrorReport::Show(StrF(_T("The clipboard was not saved: its data is malformed (%s)."), CString(error.what()).GetString()));
		return;
	}

	CLogger::Log(_T("Loaded clips from the clipboard, sending message to save to the db"));
	if(::PostMessage(m_hWnd, CDittoMessage::ClipboardCopied, reinterpret_cast<WPARAM>(clip.get()), 0))
	{
		clip.release(); // ownership: CMainFrame::OnClipboardCopied retakes it in a std::unique_ptr
	}
}

LRESULT CMainFrame::OnReAddTaskBarIcon(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if(CGetSetOptions::GetShowIconInSysTray())
	{
		m_trayIcon.SetIcon(CTrayNotifyIcon::LoadIcon(IDR_MAINFRAME));
	}
	return TRUE;
}

LRESULT CMainFrame::OnReOpenDatabase(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	CLogger::Log(StrF(_T("OnReOpenDatabase, Start closing and reopening database Delay: %d"), CGetSetOptions::GetWindowsResumeDelayReOpenDbMS()));

	try 
	{
		Sleep(CGetSetOptions::GetWindowsResumeDelayReOpenDbMS());
		m_quickPaste.CloseQPasteWnd();
		theApp.m_db.close();
		OpenDatabase(CGetSetOptions::GetDBPath());
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(StrF(_T("Reopening the clip database after resume failed: %s"), e.errorMessage()));
		return FALSE;
	}

	CLogger::Log(StrF(_T("OnReOpenDatabase, End closing and reopening database Delay: %d"), CGetSetOptions::GetWindowsResumeDelayReOpenDbMS()));

	return TRUE;
}

LRESULT CMainFrame::OnShowMsgWindow(WPARAM wParam, LPARAM /*lParam*/)
{
	// retakes the message released by CMainFrmThread::OnSaveClips
	const std::unique_ptr<CString> message{reinterpret_cast<CString*>(wParam)};

	m_trayIcon.SetBalloonDetails(message->GetBuffer(), _T("Ditto"), CTrayNotifyIcon::BalloonStyle::Info, CGetSetOptions::GetBalloonTimeout());

	return TRUE;
}

LRESULT CMainFrame::OnShowDittoGroup(WPARAM wParam, LPARAM /*lParam*/)
{
	int groupId = (int)wParam;
	CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT bIsGroup FROM Main WHERE lID = %d"), groupId);
	if(q.eof() == false)
	{
		if(q.getIntField(_T("bIsGroup")) > 0)
		{
			PasteOrShowGroup(groupId, FALSE, FALSE, FALSE, false);
		}
	}

	return TRUE;
}

void CMainFrame::OnFirstFixupstickycliporder()
{
	ReOrderStickyClips(-1, theApp.m_db);
}

LRESULT CMainFrame::OnResolutionChange(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_startupScreenWidth != GetScreenWidth() ||
		m_startupScreenHeight != GetScreenHeight())
	{
		m_startupScreenWidth = GetScreenWidth();
		m_startupScreenHeight = GetScreenHeight();

		SetTimer(ScreenResolutionChangedTimer, 1000, NULL);
	}

	return TRUE;
}

void CMainFrame::OnWinIniChange(LPCTSTR lpszSection)
{
	CFrameWnd::OnWinIniChange(lpszSection);

	if (lpszSection != NULL &&
		wcscmp(lpszSection, L"ImmersiveColorSet") == 0)
	{
		CLogger::Log(StrF(_T("OnWinIniChange %s, setting timer to 1000ms to change theme"), lpszSection));
		KillTimer(SetWindowsThemeTimer);
		SetTimer(SetWindowsThemeTimer, 1000, NULL);
	}
}


void CMainFrame::OnFirstShowstartupmessage()
{
	BOOL existing = CGetSetOptions::GetShowStartupMessage();
	CGetSetOptions::SetShowStartupMessage(!existing);
}


void CMainFrame::OnUpdateFirstShowstartupmessage(CCmdUI *pCmdUI)
{
	if (pCmdUI == NULL ||
		pCmdUI->m_pMenu == NULL)
	{
		return;
	}

	if (CGetSetOptions::GetShowStartupMessage())
	{
		pCmdUI->m_pMenu->CheckMenuItem(ID_FIRST_SHOWSTARTUPMESSAGE, MF_CHECKED);
	}
	else
	{
		pCmdUI->m_pMenu->CheckMenuItem(ID_FIRST_SHOWSTARTUPMESSAGE, MF_UNCHECKED);
	}
}


void CMainFrame::OnFirstBackupdatabase()
{
	BackupDbPrompt(m_hWnd);
}

void CMainFrame::OnFirstRestoredatabase()
{
	RestoreDbPrompt(m_hWnd);
}

LRESULT CMainFrame::OnBackupDb(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	OnFirstBackupdatabase();
	return TRUE;
}

LRESULT CMainFrame::OnRestoreDb(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	OnFirstRestoredatabase();
	return TRUE;
}

void CMainFrame::OnFirstDeleteallnonusedclips()
{
	int nRet = MessageBox(theApp.m_Language.GetString("Delete_All_Non_Used_Clips", "Delete all clips that are not groups, in groups, marked as never auto delete, has a shortcut key or marked as sticky.\r\n\r\nThis cannot be undone."), _T("Ditto"), MB_OKCANCEL | MB_TOPMOST);
	if (nRet != IDOK)
	{
		return;
	}

	DeleteNonUsedClips(false);

	theApp.RefreshView();
}

LRESULT CMainFrame::OnPasteClip(WPARAM wParam, LPARAM /*lParam*/)
{
	PasteOrShowGroup((int)wParam, TRUE, FALSE, TRUE, false);
	return TRUE;
}

LRESULT CMainFrame::OnEditClip(WPARAM wParam, LPARAM /*lParam*/)
{
	CClipIDs IDs;
	IDs.Add((int)wParam);

	bool textOnly = false;
	if (GetKeyState(VK_SHIFT) & 0x8000)
	{
		textOnly = true;
	}

	theApp.EditItems(IDs, true, textOnly);
	return TRUE;
}

void CMainFrame::OnSetFocus(CWnd* pOldWnd)
{
	CFrameWnd::OnSetFocus(pOldWnd);

	//int nRet = MessageBox(_T("focused"), _T("Ditto"), MB_YESNO | MB_TOPMOST);

	// TODO: Add your message handler code here
}

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
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame() :
	m_quickPaste(theApp.Services().Settings(), theApp.Services().State(), theApp.Services().Database(), theApp.Services().ActiveWindow()),
	m_thread(theApp.Services().Settings(), theApp.Services().IdleTime(), theApp.Services().Database(), theApp.Services().Clipboard(), theApp.Services().Windows())
{
	m_pEditFrameWnd = NULL;
	m_keyStateModifiers = 0;
	m_startKeyStateTime = 0;
	m_bMovedSelectionMoveKeyState = false;
	m_keyModifiersTimerCount = 0;
	m_doubleClickGroupId = -1;
	m_doubleClickGroupStartTime = 0;
}

CAppServices& CMainFrame::Services() const
{
	return theApp.Services();
}

CGetSetOptions& CMainFrame::Settings() const
{
	return Services().Settings();
}

CMainFrame::~CMainFrame()
{
	Settings().SetMainHWND(0);
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
	{
		return -1;
	}

	m_PowerManager.Start(m_hWnd);

	////Center the main window so message boxes are in the center
	CRect rcScreen = CMonitorGeometry::DefaultMonitorRect();
	CPoint cpCenter = rcScreen.CenterPoint();
	MoveWindow(cpCenter.x, cpCenter.x, 1, 1);

	m_startupScreenWidth = CMonitorGeometry::GetScreenWidth();
	m_startupScreenHeight = CMonitorGeometry::GetScreenHeight();

	//Then set the main window to transparent so it's never shown
	//if it is shown then only the task tray icon
	//m_Transparency.SetTransparent(m_hWnd, 0, true);

	SetWindowText(_T(""));

	CLogger::Log(_T("Setting polling timer to track focus"));
	SetTimer(ActiveWindowTimer, Settings().FocusWndTimerTimeout(), 0);

	SetTimer(ReadRandomDbFileTimer, Settings().ReadRandomFileInterval() * 1000, 0);

	SetWindowText(_T("Ditto"));

	m_trayIcon.Create(this, IDR_MENU, _T("Ditto"), CTrayNotifyIcon::LoadIcon(IDR_MAINFRAME), WmTrayNotify, 0, 1);
	m_trayIcon.SetDefaultMenuItem(ID_FIRST_SHOWQUICKPASTE, FALSE);

	//removed to keep Ditto from taking focus on start
	//m_trayIcon.MinimiseToTray(this);

	if (Settings().GetShowStartupMessage())
	{
		CString msg = Services().Language().GetString(_T("StartupMsg"), _T("Ditto is running minimized, Ditto can be opened by hot keys or by clicking the task tray icon"));
		m_trayIcon.SetBalloonDetails(msg, _T("Ditto"), CTrayNotifyIcon::BalloonStyle::Info, Settings().GetBalloonTimeout());
	}

	Services().Language().UpdateTrayIconRightClickMenu(&m_trayIcon.GetMenu());

//Only if in release
#ifndef _DEBUG
	{
		//If not showing the icon show it for 40 seconds so they can get to the option
		//in case they can't remember the hot keys or something like that
		if (!(Settings().GetShowIconInSysTray()))
		{
			SetTimer(HideIconTimer, 40000, 0);
		}
	}
#endif

	//SetTimer(CloseWindowTimer, CMilliseconds::OneHour*24, 0);
	SetTimer(RemoveOldTempFilesTimer, CMilliseconds::OneHour * 6, 0);
	SetTimer(RemoveOldEntriesTimer, CMilliseconds::OneMinute * 15, 0);
	SetTimer(CloseNoDbWindowTimer, 10000, 0);

	//found on some computers GetTickCount gettickcount returns a smaller value than other, can't explain
	//check here to see if we need to make an adjustment
	Services().IdleTime().IdleSeconds();

	m_ulCopyGap = Settings().GetCopyGap();

	if (!theApp.AfterMainCreate())
	{
		return -1; // the failure is reported; CCP_MainApp::CreateMainWnd stops the start
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
		Services().ActiveWindow().TrackActiveWnd(true);
	}

	//click on balloon
	if (lParam == 0x405)
	{
		SetTimer(DelayedShowDittoTimer, 100, NULL);
	}

	m_trayIcon.OnTrayNotification(wParam, lParam);
	return 0L;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if (cs.hMenu != NULL)
	{
		::DestroyMenu(cs.hMenu); // delete menu if loaded
		cs.hMenu = NULL;         // no menu for this window
	}

	if (!CFrameWnd::PreCreateWindow(cs))
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
	if (!AfxRegisterClass(&wc))
	{
		return FALSE;
	}

	cs.lpszClass = wc.lpszClassName;

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
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

	const CHotKeys& hotKeys{ Services().HotKeys() };
	if (IsHotKey(hotKeys.Named(CHotKeys::Id::TextOnlyPaste), wParam))
	{
		DoTextOnlyPaste();
	}
	else if (IsHotKey(hotKeys.Named(CHotKeys::Id::SaveClipboard), wParam))
	{
		OnFirstSavecurrentclipboard();
	}
	else if (IsHotKey(hotKeys.Named(CHotKeys::Id::CopyAndSaveClipboard), wParam))
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

bool CMainFrame::IsShowDittoHotKey(WPARAM wParam) const
{
	const CHotKeys& hotKeys{ Services().HotKeys() };
	return IsHotKey(hotKeys.Named(CHotKeys::Id::DittoHotKey), wParam) ||
		   IsHotKey(hotKeys.Named(CHotKeys::Id::DittoHotKey2), wParam) ||
		   IsHotKey(hotKeys.Named(CHotKeys::Id::DittoHotKey3), wParam);
}

void CMainFrame::OnShowDittoHotKey()
{
	//If they still have the shift/ctrl keys down
	if (m_keyStateModifiers != 0 && m_quickPaste.IsWindowVisibleEx())
	{
		CLogger::Log(_T("On Show Ditto HotKey, key state modifiers are still down, moving selection"));

		if (m_bMovedSelectionMoveKeyState == false)
		{
			CLogger::Log(_T("Setting flag m_bMovedSelectionMoveKeyState to true, will paste when modifier keys are up"));
		}

		m_quickPaste.MoveSelection(true);
		m_bMovedSelectionMoveKeyState = true;
	}
	else if (Settings().m_HideDittoOnHotKeyIfAlreadyShown && m_quickPaste.IsWindowTopLevel() && Settings().GetShowPersistent() == FALSE)
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
	const std::array<CHotKeys::Id, 10> positions{ CHotKeys::Id::PosOne, CHotKeys::Id::PosTwo, CHotKeys::Id::PosThree, CHotKeys::Id::PosFour, CHotKeys::Id::PosFive,
												  CHotKeys::Id::PosSix, CHotKeys::Id::PosSeven, CHotKeys::Id::PosEight, CHotKeys::Id::PosNine, CHotKeys::Id::PosTen };
	const CHotKeys& hotKeys{ Services().HotKeys() };

	for (int pos = 0; pos < static_cast<int>(positions.size()); pos++)
	{
		if (IsHotKey(hotKeys.Named(positions[pos]), wParam))
		{
			CLogger::Log(CStringUtil::Format(_T("Pos %d hot key"), pos + 1));
			DoFirstTenPositionsPaste(pos);
			return true;
		}
	}

	return false;
}

bool CMainFrame::DoCopyBufferHotKey(WPARAM wParam)
{
	const CHotKeys& hotKeys{ Services().HotKeys() };
	const std::array<CopyBufferHotKeys, 5> buffers{ {
		{ hotKeys.Named(CHotKeys::Id::CopyBuffer1), hotKeys.Named(CHotKeys::Id::PasteBuffer1), hotKeys.Named(CHotKeys::Id::CutBuffer1) },
		{ hotKeys.Named(CHotKeys::Id::CopyBuffer2), hotKeys.Named(CHotKeys::Id::PasteBuffer2), hotKeys.Named(CHotKeys::Id::CutBuffer2) },
		{ hotKeys.Named(CHotKeys::Id::CopyBuffer3), hotKeys.Named(CHotKeys::Id::PasteBuffer3), hotKeys.Named(CHotKeys::Id::CutBuffer3) },
		{ hotKeys.Named(CHotKeys::Id::CopyBuffer4), hotKeys.Named(CHotKeys::Id::PasteBuffer4), hotKeys.Named(CHotKeys::Id::CutBuffer4) },
		{ hotKeys.Named(CHotKeys::Id::CopyBuffer5), hotKeys.Named(CHotKeys::Id::PasteBuffer5), hotKeys.Named(CHotKeys::Id::CutBuffer5) },
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
		CLogger::Log(CStringUtil::Format(_T("Copy buffer %d hot key"), buffer + 1));
		Services().CopyBuffer().StartCopy(buffer);
		return true;
	}

	if (IsHotKey(hotKeys.paste, wParam))
	{
		CLogger::Log(CStringUtil::Format(_T("Paste buffer %d hot key"), buffer + 1));
		Services().CopyBuffer().PastCopyBuffer(buffer);
		return true;
	}

	if (IsHotKey(hotKeys.cut, wParam))
	{
		CLogger::Log(CStringUtil::Format(_T("Cut buffer %d hot key"), buffer + 1));
		Services().CopyBuffer().StartCopy(buffer, true);
		return true;
	}

	return false;
}

void CMainFrame::DoCopyAndSaveClipboard()
{
	CLogger::Log(CStringUtil::Format(_T("START of copy and save clipboard, sending copy")));

	Services().ActiveWindow().SendCopy(CopyReasonEnum::COPY_TO_UNKOWN);

	int delay = Settings().GetCopyAndSveDelay();
	CLogger::Log(CStringUtil::Format(_T("Copy and save clipboard, sending copy, delaying %dms before saving clipboard"), delay));
	Sleep(delay);

	CLogger::Log(CStringUtil::Format(_T("Copy and save clipboard, saving clipboard")));
	OnFirstSavecurrentclipboard();

	CLogger::Log(CStringUtil::Format(_T("END of copy and save clipboard")));
}

void CMainFrame::DoGlobalClipHotKey(WPARAM wParam)
{
	const CHotKeys& hotKeys{ Services().HotKeys() };
	for (int i = 0; i < hotKeys.GetCount(); i++)
	{
		if (hotKeys[i] != NULL &&
			hotKeys[i]->m_Atom == wParam &&
			hotKeys[i]->m_clipId > 0)
		{
			if (hotKeys[i]->m_hkType == CHotKey::PASTE_OPEN_CLIP)
			{
				CLogger::Log(CStringUtil::Format(_T("Pasting clip from global shortcut, clipId: %d"), hotKeys[i]->m_clipId));
				PasteOrShowGroup(hotKeys[i]->m_clipId, -1, FALSE, TRUE, false);
			}
			else if (hotKeys[i]->m_hkType == CHotKey::MOVE_TO_GROUP)
			{
				CLogger::Log(CStringUtil::Format(_T("Global hot key to save clip to group Id: %d, Sending copy to save selection to this group"), hotKeys[i]->m_clipId));

				KillTimer(GroupDoubleClickTimer);
				m_doubleClickGroupId = -1;
				m_doubleClickGroupStartTime = 0;

				Services().State().SetActiveGroupId(hotKeys[i]->m_clipId);
				Services().ActiveWindow().SendCopy(CopyReasonEnum::COPY_TO_GROUP);
			}

			break;
		}
	}
}

void CMainFrame::ShowQPasteWithActiveWindowCheck()
{
	//Before we show our window find the current focused window for paste into
	ExternalWindowTracker& activeWindow{ Services().ActiveWindow() };
	activeWindow.TrackActiveWnd(true);

	if (Settings().GetOpenToGroupByActiveExe() &&
		activeWindow.ActiveWnd() != NULL)
	{
		CString exeName = CWindowInspector::GetProcessName(activeWindow.ActiveWnd());
		if (exeName != _T(""))
		{
			Services().Groups().TryEnterOldGroupState();
			CString query = CStringUtil::Format(_T("SELECT lID FROM Main WHERE bIsGroup = 1 AND mText = '%s' COLLATE NOCASE"), exeName.GetString());
			CppSQLite3Query q = Services().Database().execQueryEx(query);
			if (q.eof() == false)
			{
				int groupId = q.getIntField(_T("lID"));
				//this will revert back to the old group on hide of ditto
				Services().Groups().EnterGroupID(groupId, TRUE, TRUE);

				CLogger::Log(CStringUtil::Format(_T("Opening Ditto to Group based on found group name, name: %s, GroupId: %d"), exeName.GetString(), groupId));
			}
			else
			{
				Services().Groups().TryEnterOldGroupState();
			}
		}
	}

	m_quickPaste.ShowQPasteWnd(this, false, true, FALSE);
}

void CMainFrame::DoTextOnlyPaste()
{
	CClipboardSaveRestore textOnlyPaste(Services().Windows(), Services().ClipboardFormats());

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
		CErrorReport::Show(CStringUtil::Format(_T("Text only paste stopped: the clipboard data is malformed (%s)."), CString(error.what()).GetString()));
		return;
	}

	DWORD pasteDelay = Settings().GetTextOnlyPasteDelay();

	CLogger::Log(CStringUtil::Format(_T("Text Only paste, delaying %d ms before sending paste"), pasteDelay));

	Sleep(pasteDelay);

	CLogger::Log(_T("Text Only paste, Sending paste"));
	Services().ActiveWindow().SendPaste(false);

	CLogger::Log(_T("Text Only paste, Post sending paste"));
}

void CMainFrame::DoFirstTenPositionsPaste(int nPos)
{
	try
	{
		CString csSort = _T("");
		CString strFilter = _T("");
		bool pastedFromGroup = false;
		const long groupId{ Services().State().m_GroupID };

		if (groupId < 0 ||
			Settings().GetUseUISelectedGroupForLastTenCopies() == FALSE)
		{
			//do not change this this directly relates to the views in the Main table
			csSort = "Main.bIsGroup ASC, "
					 "Main.stickyClipOrder DESC, "
					 "Main.clipOrder DESC";

			if (Settings().m_bShowAllClipsInMainList)
			{
				if (Settings().GetShowGroupsInMainList())
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

			if (groupId >= 0)
			{
				strFilter.Format(_T("Main.lParentID = %d"), groupId);
			}
		}

		CString query = CStringUtil::Format(_T("SELECT lID, bIsGroup FROM Main WHERE %s ORDER BY %s LIMIT 1 OFFSET %d"), strFilter.GetString(), csSort.GetString(), nPos);

		CLogger::Log(CStringUtil::Format(_T("Doing Last Ten Paste, Index: %d Query: %s"), nPos, query.GetString()));

		CppSQLite3Query q = Services().Database().execQueryEx(query);

		if (q.eof() == false)
		{
			PasteOrShowGroup(q.getIntField(_T("lID")), Settings().GetMoveClipsOnGlobal10(), false, Settings().m_bSendPasteOnFirstTenHotKeys, pastedFromGroup);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Pasting the clip at position %d failed: %s"), nPos, e.errorMessage()));
		return;
	}
}

void CMainFrame::StartKeyModifierTimer()
{
	if (Settings().m_moveSelectionOnOpenHotkey)
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
		CppSQLite3Query q = Services().Database().execQueryEx(_T("SELECT bIsGroup FROM Main WHERE lID = %d"), dbId);
		if (q.eof() == false)
		{
			if (q.getIntField(_T("bIsGroup")) > 0)
			{
				isGroup = true;
			}
		}

		if (isGroup)
		{
			ULONGLONG maxDiff = static_cast<ULONGLONG>(Settings().GetGroupDoubleClickTimeMS());
			ULONGLONG diff = GetTickCount64() - m_doubleClickGroupStartTime;

			if (m_doubleClickGroupId == dbId &&
				diff < maxDiff)
			{
				CLogger::Log(CStringUtil::Format(_T("Second Press of group hot key, group Id: %d, Sending copy to save selection to this group"), dbId));

				KillTimer(GroupDoubleClickTimer);
				m_doubleClickGroupId = -1;
				m_doubleClickGroupStartTime = 0;

				Services().State().SetActiveGroupId(dbId);
				Services().ActiveWindow().SendCopy(CopyReasonEnum::COPY_TO_GROUP);
			}
			else
			{
				m_doubleClickGroupId = dbId;
				m_doubleClickGroupStartTime = GetTickCount64();

				int doubleClickTime = Settings().GetGroupDoubleClickTimeMS();

				SetTimer(GroupDoubleClickTimer, doubleClickTime, 0);

				CLogger::Log(CStringUtil::Format(_T("First Press of group hot key, group Id: %d, timeout: %d"), dbId, doubleClickTime));
			}
		}
		else
		{
			PasteSingleClip(dbId, updateClipTime, activeTarget, sendPaste, pastedFromGroup);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Pasting clip or group id %d failed: %s"), dbId, e.errorMessage()));
		return;
	}
}

void CMainFrame::PasteSingleClip(int dbId, BOOL updateClipTime, BOOL activeTarget, BOOL sendPaste, bool pastedFromGroup)
{
	KillTimer(GroupDoubleClickTimer);
	m_doubleClickGroupId = -1;
	m_doubleClickGroupStartTime = 0;

	BOOL bItWas{ Settings().m_bUpdateTimeOnPaste };
	if (updateClipTime != -1)
	{
		Settings().m_bUpdateTimeOnPaste = updateClipTime;
	}

	CProcessPaste paste{ Services().ClipContext(), Services().ActiveWindow() };
	paste.m_pastedFromGroup = pastedFromGroup;
	paste.GetClipIDs().Add(dbId);

	if (activeTarget != -1)
	{
		paste.m_bActivateTarget = activeTarget ? true : false;
		;
	}

	if (sendPaste != -1)
	{
		paste.m_bSendPaste = sendPaste ? true : false;
	}
	paste.DoPaste();

	if (updateClipTime != -1)
	{
		Settings().m_bUpdateTimeOnPaste = bItWas;
	}
}

void CMainFrame::DoDittoCopyBufferPaste(int nCopyBuffer)
{
	try
	{
		CppSQLite3Query q = Services().Database().execQueryEx(_T("SELECT lID FROM Main WHERE CopyBuffer = %d"), nCopyBuffer);

		if (q.eof() == false)
		{
			//Don't move these to the top
			BOOL bItWas = Settings().m_bUpdateTimeOnPaste;
			Settings().m_bUpdateTimeOnPaste = FALSE;

			CProcessPaste paste(Services().ClipContext(), Services().ActiveWindow());
			paste.GetClipIDs().Add(q.getIntField(_T("lID")));
			paste.m_bActivateTarget = false;
			paste.DoPaste();

			Settings().m_bUpdateTimeOnPaste = bItWas;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Pasting Ditto copy buffer %d failed: %s"), nCopyBuffer, e.errorMessage()));
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
	if (!Settings().GetShowIconInSysTray())
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
	if (m_keyStateModifiers != 0)
	{
		BYTE keyState = CAccels::GetKeyStateModifiers();
		//Have they release the key state modifiers yet(ctrl, shift, alt)
		if ((m_keyStateModifiers & keyState) == 0)
		{
			KillTimer(KeyStateModifiersTimer);
			long waitTime = static_cast<long>(GetTickCount64() - m_startKeyStateTime);

			if (m_bMovedSelectionMoveKeyState || m_keyModifiersTimerCount > Settings().GetKeyStateWaitTimerCount())
			{
				CLogger::Log(CStringUtil::Format(_T("Timer KEY_STATE_MODIFIERS timeout count hit(%d), count (%d), time (%d), Move Selection from Modifer (%d) sending paste"), Settings().GetKeyStateWaitTimerCount(), m_keyModifiersTimerCount, waitTime, m_bMovedSelectionMoveKeyState));
				m_quickPaste.OnKeyStateUp();
			}
			else
			{
				CLogger::Log(CStringUtil::Format(_T("Timer KEY_STATE_MODIFIERS count NOT hit(%d), count (%d) time (%d)"), Settings().GetKeyStateWaitTimerCount(), m_keyModifiersTimerCount, waitTime));
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
	if (Services().State().m_bShowingQuickPaste)
	{
		Services().ActiveWindow().TrackActiveWnd(false);
	}
}

void CMainFrame::OnReadRandomDbFileTimer()
{
	m_thread.FireReadDbFile();
}

void CMainFrame::OnGroupDoubleClickTimer()
{
	KillTimer(GroupDoubleClickTimer);

	CLogger::Log(CStringUtil::Format(_T("Processing single click of groupId %d in timer, opening ditto to this group"), m_doubleClickGroupId));

	ULONGLONG maxDiff = static_cast<ULONGLONG>(Settings().GetGroupDoubleClickTimeMS() * 1.5);
	ULONGLONG diff = GetTickCount64() - m_doubleClickGroupStartTime;

	if (diff < maxDiff)
	{
		if (m_doubleClickGroupId > -1)
		{
			if (Services().Groups().EnterGroupID(m_doubleClickGroupId, FALSE, TRUE))
			{
				Services().ActiveWindow().TrackActiveWnd(true);
				StartKeyModifierTimer();
				m_quickPaste.ShowQPasteWnd(this, false, true, FALSE);
			}
		}
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("Something happened and we didn't process the group timer in time, Id: %d, Diff ms: %llu, maxDiff: %llu"), m_doubleClickGroupId, diff, maxDiff));
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
	auto theme = Settings().GetTheme();
	if (theme == _T(""))
	{
		Settings().m_Theme.Load(Settings(), theme);

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
	if (lParam)
	{
		if (!m_trayIcon.IsHidden())
		{
			KillTimer(HideIconTimer);
			SetTimer(HideIconTimer, 40000, 0);
		}
	}

	if (wParam)
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
	Services().Clipboard().ToggleConnectCV();
}

void CMainFrame::OnUpdateFirstToggleConnectCV(CCmdUI* pCmdUI)
{
	Services().Clipboard().UpdateMenuConnectCV(pCmdUI->m_pMenu, ID_FIRST_TOGGLECONNECTCV);
}

LRESULT CMainFrame::OnClipboardCopied(WPARAM wParam, LPARAM /*lParam*/)
{
	CLogger::Log(_T("Start of function OnClipboardCopied, adding clip to thread for processing"));

	// retakes the clip released by the sender (CCopyThread::OnClipboardChange or OnFirstSavecurrentclipboard)
	std::unique_ptr<CClip> clip{ reinterpret_cast<CClip*>(wParam) };
	if (clip)
	{
		m_thread.AddClipToSave(std::move(clip));
	}

	CLogger::Log(_T("End of function OnClipboardCopied"));
	return TRUE;
}

BOOL CMainFrame::PreTranslateMessage(MSG* pMsg)
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

	CWnd* pTempWnd = GetDesktopWindow()->GetWindow(GW_CHILD);
	while ((pTempWnd = pTempWnd->GetWindow(GW_HWNDNEXT)) != NULL)
	{
		if (pTempWnd->GetSafeHwnd() == NULL)
		{
			break;
		}

		GetWindowThreadProcessId(pTempWnd->GetSafeHwnd(), &dwordChildWindowProcessId);
		if (dwordChildWindowProcessId == dwordProcessId)
		{
			TCHAR szTemp[100];
			GetClassName(pTempWnd->GetSafeHwnd(), szTemp, 100);

			// #32770 is class name for dialogs so don't process the message if it is a dialog
			if (_tcscmp(szTemp, _T("#32770")) == 0)
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
	while (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE))
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
			Settings().GetEditWndSize(sz);
			Settings().GetEditWndPoint(pt);
			CRect cr(pt, sz);
			CMonitorGeometry::EnsureWindowVisible(&cr);
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
	CLogger::Log(CStringUtil::Format(_T("ShowErrorMessage %s - %s"), csTitle.GetString(), csMessage.GetString()));
	m_trayIcon.SetBalloonDetails(csMessage, csTitle, CTrayNotifyIcon::BalloonStyle::Error, Settings().GetBalloonTimeout());
}

void CMainFrame::OnFirstImport()
{
	Services().ClipCommands().ImportClips(Services().Windows().MainHwnd());
}

LRESULT CMainFrame::OnSetConnected(WPARAM wParam, LPARAM lParam)
{
	if (wParam)
	{
		Services().Clipboard().SetConnectCV(true);
	}
	else if (lParam)
	{
		Services().Clipboard().SetConnectCV(false);
	}

	return TRUE;
}

LRESULT CMainFrame::OnOpenCloseWindow(WPARAM wParam, LPARAM lParam)
{
	if (wParam)
	{
		StartKeyModifierTimer();
		ShowQPasteWithActiveWindowCheck();
	}
	else if (lParam)
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

void CMainFrame::PostNcDestroy()
{
	// the window and its children are gone; the frame deletes itself next
	if (Services().Windows().MainFrame() == this)
	{
		Services().Windows().SetMainFrame(NULL);
		Services().Windows().SetMainHwnd(NULL);
	}

	CFrameWnd::PostNcDestroy();
}

void CMainFrame::OnFirstNewclip()
{
	CClipIDs IDs;
	IDs.Add(-1);
	Services().ClipCommands().EditItems(IDs, true, true);
}

void CMainFrame::OnFirstOption()
{
	if (m_pOptions)
	{
		::SetForegroundWindow(m_pOptions->m_hWnd);
	}
	else
	{
		auto options{ std::make_unique<COptionsSheet>(_T("")) };
		options->SetNotifyWnd(m_hWnd);
		m_pOptions = std::move(options);

		m_pOptions->Create();
		m_pOptions->ShowWindow(SW_SHOW);
	}
}

void CMainFrame::OnFirstGlobalhotkeys()
{
	if (m_pGlobalClips)
	{
		::SetForegroundWindow(m_pGlobalClips->m_hWnd);
	}
	else
	{
		auto globalClips{ std::make_unique<GlobalClips>() };
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
		Settings().m_Theme.Load(Settings(), Settings().GetTheme());

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
	Services().Language().UpdateTrayIconRightClickMenu(&m_trayIcon.GetMenu());

	if (Settings().GetShowIconInSysTray())
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
		auto deleteClips{ std::make_unique<CDeleteClipData>() };
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
	const std::unique_ptr<CClipTypes> types(CClipDataReader(Services().Database()).LoadTypesFromDB());
	if (!types)
	{
		CLogger::Log(_T("Failed to load supported types from the db, not saving to the db"));
		return;
	}

	auto clip = std::make_unique<CClip>(Services().ClipContext());
	try
	{
		if (!clip->LoadFromClipboard(types.get(), Settings().m_regexHelper, false, _T("")))
		{
			CLogger::Log(_T("Failed to load clips from the clipboard, not saving to db"));
			return;
		}
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CErrorReport::Show(CStringUtil::Format(_T("The clipboard was not saved: its data is malformed (%s)."), CString(error.what()).GetString()));
		return;
	}

	CLogger::Log(_T("Loaded clips from the clipboard, sending message to save to the db"));
	if (::PostMessage(m_hWnd, CDittoMessage::ClipboardCopied, reinterpret_cast<WPARAM>(clip.get()), 0))
	{
		clip.release(); // ownership: CMainFrame::OnClipboardCopied retakes it in a std::unique_ptr
	}
}

LRESULT CMainFrame::OnReAddTaskBarIcon(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (Settings().GetShowIconInSysTray())
	{
		m_trayIcon.SetIcon(CTrayNotifyIcon::LoadIcon(IDR_MAINFRAME));
	}
	return TRUE;
}

LRESULT CMainFrame::OnReOpenDatabase(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	CLogger::Log(CStringUtil::Format(_T("OnReOpenDatabase, Start closing and reopening database Delay: %d"), Settings().GetWindowsResumeDelayReOpenDbMS()));

	try
	{
		Sleep(Settings().GetWindowsResumeDelayReOpenDbMS());
		m_quickPaste.CloseQPasteWnd();
		Services().Database().close();
		if (CDatabaseManager::OpenDatabase(Settings(), Services().Database(), Services().State(), Settings().GetDBPath()) == FALSE)
		{
			return FALSE; // OpenDatabase showed the error
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Reopening the clip database after resume failed: %s"), e.errorMessage()));
		return FALSE;
	}

	CLogger::Log(CStringUtil::Format(_T("OnReOpenDatabase, End closing and reopening database Delay: %d"), Settings().GetWindowsResumeDelayReOpenDbMS()));

	return TRUE;
}

LRESULT CMainFrame::OnShowMsgWindow(WPARAM wParam, LPARAM /*lParam*/)
{
	// retakes the message released by CMainFrmThread::OnSaveClips
	const std::unique_ptr<CString> message{ reinterpret_cast<CString*>(wParam) };

	m_trayIcon.SetBalloonDetails(message->GetBuffer(), _T("Ditto"), CTrayNotifyIcon::BalloonStyle::Info, Settings().GetBalloonTimeout());

	return TRUE;
}

LRESULT CMainFrame::OnShowDittoGroup(WPARAM wParam, LPARAM /*lParam*/)
{
	int groupId = (int)wParam;
	CppSQLite3Query q = Services().Database().execQueryEx(_T("SELECT bIsGroup FROM Main WHERE lID = %d"), groupId);
	if (q.eof() == false)
	{
		if (q.getIntField(_T("bIsGroup")) > 0)
		{
			PasteOrShowGroup(groupId, FALSE, FALSE, FALSE, false);
		}
	}

	return TRUE;
}

void CMainFrame::OnFirstFixupstickycliporder()
{
	CDatabaseManager::ReOrderStickyClips(-1, Services().Database());
}

LRESULT CMainFrame::OnResolutionChange(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if (m_startupScreenWidth != CMonitorGeometry::GetScreenWidth() ||
		m_startupScreenHeight != CMonitorGeometry::GetScreenHeight())
	{
		m_startupScreenWidth = CMonitorGeometry::GetScreenWidth();
		m_startupScreenHeight = CMonitorGeometry::GetScreenHeight();

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
		CLogger::Log(CStringUtil::Format(_T("OnWinIniChange %s, setting timer to 1000ms to change theme"), lpszSection));
		KillTimer(SetWindowsThemeTimer);
		SetTimer(SetWindowsThemeTimer, 1000, NULL);
	}
}


void CMainFrame::OnFirstShowstartupmessage()
{
	BOOL existing = Settings().GetShowStartupMessage();
	Settings().SetShowStartupMessage(!existing);
}


void CMainFrame::OnUpdateFirstShowstartupmessage(CCmdUI* pCmdUI)
{
	if (pCmdUI == NULL ||
		pCmdUI->m_pMenu == NULL)
	{
		return;
	}

	if (Settings().GetShowStartupMessage())
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
	CDatabaseBackupPrompt prompt(Settings(), Services().Language(), Services().Database(), Services().State(), Services().Windows());
	prompt.BackupDbPrompt(m_hWnd);
}

void CMainFrame::OnFirstRestoredatabase()
{
	CDatabaseBackupPrompt prompt(Settings(), Services().Language(), Services().Database(), Services().State(), Services().Windows());
	prompt.RestoreDbPrompt(m_hWnd);
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
	int nRet = MessageBox(Services().Language().GetString("Delete_All_Non_Used_Clips", "Delete all clips that are not groups, in groups, marked as never auto delete, has a shortcut key or marked as sticky.\r\n\r\nThis cannot be undone."), _T("Ditto"), MB_OKCANCEL | MB_TOPMOST);
	if (nRet != IDOK)
	{
		return;
	}

	CClipRetentionPolicy::DeleteNonUsedClips(Services().Database(), Services().Windows(), false);

	Services().Windows().RefreshView();
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

	Services().ClipCommands().EditItems(IDs, true, textOnly);
	return TRUE;
}

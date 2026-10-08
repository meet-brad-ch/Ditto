#include "stdafx.h"
#include "CP_Main.h" // the frame, quick paste window and copy thread types in the order they need
#include "ClipboardMonitor.h"
#include "AppState.h"
#include "AppWindows.h"
#include "ClipDataReader.h"
#include "ErrorReport.h"

CClipboardMonitor::CClipboardMonitor(CGetSetOptions& settings, CDittoDb& database, CMultiLanguage& language, CAppState& state, CAppWindows& windows, CDittoCopyBuffer& copyBuffer) :
	m_settings(settings),
	m_database(database),
	m_language(language),
	m_state(state),
	m_windows(windows),
	m_copyBuffer(copyBuffer)
{
}

bool CClipboardMonitor::Start(int connectOnStartup)
{
	const HWND mainHwnd{m_windows.MainHwnd()};
	ASSERT( mainHwnd );
	std::unique_ptr<CClipTypes> pTypes{CClipDataReader(m_database).LoadTypesFromDB()};
	if (!pTypes)
	{
		// LoadTypesFromDB reported why; without the clip types the copy thread cannot work
		return false;
	}
	// initialize to:
	// - mainHwnd = send CDittoMessage::ClipboardCopied messages to the main window
	// - true = use Asynchronous communication (PostMessage)
	// - true = enable copying on clipboard changes
	// - pTypes = the supported types to use
	m_CopyThread.Init(CCopyConfig(mainHwnd, true, true, std::move(pTypes)));

	if(connectOnStartup == FALSE || m_settings.GetConnectedToClipboard() == FALSE)
	{
		m_CopyThread.m_connectOnStartup = false;
		CLogger::Log(CStringUtil::Format(_T("Starting Ditto up disconnected from the clipboard, commandLine: %d, saved value: %d"), connectOnStartup, m_settings.GetConnectedToClipboard()));
		SetConnectCV(false);
	}
	else if(connectOnStartup == TRUE)
	{
		SetConnectCV(true);
		CLogger::Log(_T("Starting Ditto up connected from the clipboard, passed in true from command line to start connected"));
	}

	if (!m_CopyThread.CreateThread(CREATE_SUSPENDED))
	{
		CErrorReport::Show(CStringUtil::Format(_T("Starting the clipboard copy thread failed (error %u)."), ::GetLastError()));
		return false;
	}
	m_CopyThread.ResumeThread();
	return true;
}

void CClipboardMonitor::Stop()
{
	EnableCbCopy(false);
	m_CopyThread.Quit();
}

HWND CClipboardMonitor::GetClipboardViewer()
{
	return m_CopyThread.m_pClipboardViewer->m_hWnd;
}

bool CClipboardMonitor::EnableCbCopy(bool bState)
{
	return m_CopyThread.SetCopyOnChange(bState);
}

bool CClipboardMonitor::IsClipboardViewerConnected()
{
	return m_CopyThread.IsClipboardViewerConnected();
}

bool CClipboardMonitor::GetConnectCV()
{
	return m_CopyThread.GetConnectCV();
}

void CClipboardMonitor::SetConnectCV(bool bConnect)
{
	m_CopyThread.SetConnectCV(bConnect);
	m_settings.SetConnectedToClipboard(bConnect == true);

	CMainFrame* pMainFrame{m_windows.MainFrame()};
	if(bConnect)
	{
		pMainFrame->m_trayIcon.SetIcon(IDR_MAINFRAME);
		pMainFrame->m_trayIcon.SetTooltipText(_T("Ditto"));
	}
	else
	{
		pMainFrame->m_trayIcon.SetIcon(IDI_DITTO_NOCOPYCB);
		CString cs{};
		cs = _T("Ditto ");
		cs += m_language.GetString("disconnected", "[Disconnected]");
		pMainFrame->m_trayIcon.SetTooltipText(cs);
	}

	if(m_windows.QPasteWnd())
	{
		RefreshCaption(*m_windows.QPasteWnd());
	}
}

// returns the current Clipboard Viewer Connect state (though it might not yet
//  be actually connected -- check IsClipboardViewerConnected())
bool CClipboardMonitor::ToggleConnectCV()
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
void CClipboardMonitor::UpdateMenuConnectCV(CMenu* pMenu, UINT nMenuID)
{
	if(pMenu == NULL)
		return;

	bool bConnect = GetConnectCV();
	CString cs{};

	if(bConnect)
	{
		cs = m_language.GetString("Disconnect_Clipboard", "Disconnect from Clipboard.");
		pMenu->ModifyMenu(nMenuID, MF_BYCOMMAND, nMenuID, cs);
	}
	else
	{
		cs = m_language.GetString("Connect_Clipboard", "Connect to Clipboard.");
		pMenu->ModifyMenu(nMenuID, MF_BYCOMMAND, nMenuID, cs);
	}
}

void CClipboardMonitor::ShowPersistent(bool bVal)
{
	m_settings.SetShowPersistent(bVal);

	// give some visual indication
	if(m_state.m_bShowingQuickPaste)
	{
		ASSERT(m_windows.QPasteWnd());
		RefreshCaption(*m_windows.QPasteWnd());
	}
}

void CClipboardMonitor::RefreshCaption(CQPasteWnd& pasteWnd)
{
	pasteWnd.SetCaptionColorActive(m_settings.m_bShowPersistent, GetConnectCV());
	pasteWnd.RefreshNc();
}

void CClipboardMonitor::ReloadTypes()
{
	std::unique_ptr<CClipTypes> pTypes{CClipDataReader(m_database).LoadTypesFromDB()};

	if(pTypes)
	{
		m_CopyThread.SetSupportedTypes(std::move(pTypes));
	}
}

void CClipboardMonitor::OnCopyCompleted(long lLastID, int count, CopyReasonEnum::CopyReason copyReason)
{
	if(count <= 0)
	{
		return;
	}

	// update copy statistics
	m_settings.SetTripCopyCount(-count);
	m_settings.SetTotalCopyCount(-count);

	if(m_copyBuffer.Active())
	{
		m_copyBuffer.EndCopy(lLastID);
	}

	m_windows.RefreshView(copyReason);
}

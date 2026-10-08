#include "stdafx.h"
#include "ShowTaskBarIcon.h"
#include "CP_Main.h" // the frame types in the order they need
#include "AppState.h"
#include "AppWindows.h"

CShowTaskBarIcon::CShowTaskBarIcon(CAppWindows& windows, CAppState& state) :
	m_windows(windows),
	m_state(state)
{
	CMainFrame* pMainFrame{m_windows.MainFrame()};
	pMainFrame->m_trayIcon.MaximiseFromTray(pMainFrame);
	m_hWnd = pMainFrame->GetSafeHwnd();
	m_state.AddTaskbarIconUser();
}


CShowTaskBarIcon::~CShowTaskBarIcon(void)
{
	const long remainingRefs = m_state.ReleaseTaskbarIconUser();

	if(m_hWnd && ::IsWindow(m_hWnd) && remainingRefs == 0)
	{
		CMainFrame* pMainFrame{m_windows.MainFrame()};
		pMainFrame->m_trayIcon.MinimiseToTray(pMainFrame);
	}
}

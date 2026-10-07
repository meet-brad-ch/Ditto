#include "stdafx.h"
#include "ShowTaskBarIcon.h"
#include "CP_Main.h"

volatile long CShowTaskBarIcon::m_refCount = 0;

CShowTaskBarIcon::CShowTaskBarIcon(void)
{
	theApp.m_pMainFrame->m_trayIcon.MaximiseFromTray(theApp.m_pMainFrame);
	m_hWnd = theApp.m_pMainFrame->GetSafeHwnd();
	::InterlockedIncrement(&m_refCount);
}


CShowTaskBarIcon::~CShowTaskBarIcon(void)
{
	const long remainingRefs = ::InterlockedDecrement(&m_refCount);

	if(m_hWnd && ::IsWindow(m_hWnd) && remainingRefs == 0)
	{
		theApp.m_pMainFrame->m_trayIcon.MinimiseToTray(theApp.m_pMainFrame);
	}	
}

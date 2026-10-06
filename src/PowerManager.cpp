#include "stdafx.h"
#include "PowerManager.h"
#include "Misc.h"
#include "ErrorReport.h"

#pragma comment(lib, "powrprof.lib")

static HWND s_notifyHwnd;
static ULONG CALLBACK PowerChanged(PVOID Context, ULONG Type, PVOID Setting);

ULONG CALLBACK PowerChanged(PVOID Context, ULONG Type, PVOID Setting)
{
	//a
	//b
	//c
	CString cs;
	cs.Format(_T("PowerChanged Type %d"), Type);
	Log(cs);

	if(Type == PBT_APMRESUMEAUTOMATIC)
	{
		//had reports of the main window not showing clips after resuming (report was from a vmware vm), catch the resuming callback from windows
		//and close and reopen the database
		Log(_T("windows is RESUMING, sending message to main window to close and reopen the database/qpastewnd"));
		::PostMessage(s_notifyHwnd, WM_REOPEN_DATABASE, 0, 0);
	}

	return 0;
}



CPowerManager::CPowerManager()
{
	m_registrationHandle = 0;
}


CPowerManager::~CPowerManager(void)
{
	Close();
}


void CPowerManager::Start(HWND hWnd)
{
	s_notifyHwnd = hWnd;

	if (m_registrationHandle == 0)
	{
		static DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS callback{ PowerChanged, nullptr };

		const DWORD result{ PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, &callback, &m_registrationHandle) };
		if (result != ERROR_SUCCESS)
		{
			CErrorReport::Show(StrF(_T("Ditto could not register for resume notifications (PowerRegisterSuspendResumeNotification failed, error %u). The database is not reopened after sleep."), result));
		}
	}
}

void CPowerManager::Close()
{
	if (m_registrationHandle != 0)
	{
		const DWORD result{ PowerUnregisterSuspendResumeNotification(m_registrationHandle) };
		if (result != ERROR_SUCCESS)
		{
			CErrorReport::Show(StrF(_T("Ditto could not unregister from resume notifications (PowerUnregisterSuspendResumeNotification failed, error %u)."), result));
			return;
		}

		m_registrationHandle = 0;
	}
}

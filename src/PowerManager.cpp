#include "stdafx.h"
#include "PowerManager.h"
#include "Misc.h"
#include "ErrorReport.h"

#pragma comment(lib, "powrprof.lib")

ULONG CALLBACK CPowerManager::PowerChanged(PVOID Context, ULONG Type, PVOID /*Setting*/)
{
	const HWND notifyHwnd{static_cast<HWND>(Context)};

	CString cs;
	cs.Format(_T("PowerChanged Type %d"), Type);
	CLogger::Log(cs);

	if(Type == PBT_APMRESUMEAUTOMATIC)
	{
		//had reports of the main window not showing clips after resuming (report was from a vmware vm), catch the resuming callback from windows
		//and close and reopen the database
		CLogger::Log(_T("windows is RESUMING, sending message to main window to close and reopen the database/qpastewnd"));
		::PostMessage(notifyHwnd, CDittoMessage::ReopenDatabase, 0, 0);
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
	if (m_registrationHandle == 0)
	{
		m_subscribeParameters = std::make_unique<DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS>(DEVICE_NOTIFY_SUBSCRIBE_PARAMETERS{ PowerChanged, hWnd });

		const DWORD result{ PowerRegisterSuspendResumeNotification(DEVICE_NOTIFY_CALLBACK, m_subscribeParameters.get(), &m_registrationHandle) };
		if (result != ERROR_SUCCESS)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Ditto could not register for resume notifications (PowerRegisterSuspendResumeNotification failed, error %u). The database is not reopened after sleep."), result));
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
			CErrorReport::Show(CStringUtil::Format(_T("Ditto could not unregister from resume notifications (PowerUnregisterSuspendResumeNotification failed, error %u)."), result));
			// ownership: the registration stays active, so its parameters must stay valid until the process ends
			static_cast<void>(m_subscribeParameters.release());
		}

		// reported once: the destructor does not try again
		m_registrationHandle = 0;
		m_subscribeParameters.reset();
	}
}

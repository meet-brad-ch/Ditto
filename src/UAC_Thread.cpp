#include "stdafx.h"
#include "UAC_Thread.h"

#include "Misc.h"
#include "Options.h"
#include "QPasteWnd.h"
#include "cp_main.h"

CUAC_Thread::CUAC_Thread(int processId, ExternalWindowTracker& activeWindow) :
	m_activeWindow(activeWindow)
{
	m_processId = processId;

	AddEvent(UAC_PASTE, CStringUtil::Format(_T("Global\\UAC_PASTE_%d"), m_processId));
	AddEvent(UAC_COPY, CStringUtil::Format(_T("Global\\UAC_COPY_%d"), m_processId));
	AddEvent(UAC_CUT, CStringUtil::Format(_T("Global\\UAC_CUT_%d"), m_processId));

	AddEvent(UAC_EXIT, CStringUtil::Format(_T("Global\\UAC_EXIT_%d"), m_processId));

	m_waitTimeout = 30000;
}

CUAC_Thread::~CUAC_Thread(void)
{
}


void CUAC_Thread::OnTimeOut(void* /*param*/)
{
	bool close = false;
	DWORD exitCode = 0;

	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, m_processId);
	if (hProcess == NULL)
	{
		close = true;
	}
	else
	{
		if (GetExitCodeProcess(hProcess, &exitCode) == 0)
		{
			close = true;
		}
		else if (exitCode != STILL_ACTIVE)
		{
			close = true;
		}
	}

	if (close)
	{
		CLogger::Log(CStringUtil::Format(_T("Found parent process id (%d) is not running, Exit Code %d closing uac aware app"), m_processId, exitCode));
		this->CancelThread();
	}

	if (hProcess != NULL)
	{
		CloseHandle(hProcess);
	}
}

void CUAC_Thread::OnEvent(int eventId, void* /*param*/)
{
	ULONGLONG startTick = GetTickCount64();
	CLogger::Log(CStringUtil::Format(_T("Start of OnEvent, eventId: %s"), EnumName((eUacThreadEvents)eventId).GetString()));

	switch ((eUacThreadEvents)eventId)
	{
	case UAC_PASTE:
		m_activeWindow.SendPaste(false);
		break;
	case UAC_COPY:
		m_activeWindow.SendCopy(CopyReasonEnum::COPY_TO_UNKOWN);
		break;
	case UAC_CUT:
		m_activeWindow.SendCut();
		break;
	case UAC_EXIT:
		this->CancelThread();
		break;
	}

	ULONGLONG length = GetTickCount64() - startTick;
	CLogger::Log(CStringUtil::Format(_T("End of OnEvent, eventId: %s, Time: %llu(ms)"), EnumName((eUacThreadEvents)eventId).GetString(), length));
}

CString CUAC_Thread::EnumName(eUacThreadEvents e)
{
	switch (e)
	{
	case UAC_PASTE:
		return _T("Paste Elevated");
	case UAC_COPY:
		return _T("COPY Elevated");
	case UAC_CUT:
		return _T("Cut Elevated");
	case UAC_EXIT:
		return _T("Save Startup Elevated");
	}

	return _T("");
}

bool CUAC_Thread::UACPaste()
{
	bool ret = StartProcess();

	FirePaste();

	return ret;
}

bool CUAC_Thread::UACCopy()
{
	bool ret = StartProcess();

	FireCopy();

	return ret;
}

bool CUAC_Thread::UACCut()
{
	bool ret = StartProcess();

	FireCut();

	return ret;
}

bool CUAC_Thread::StartProcess()
{
	bool ret = true;
	CString mutexName;
	mutexName.Format(_T("DittoAdminPaste_%d"), GetCurrentProcessId());

	HANDLE mutex = CreateMutex(NULL, FALSE, mutexName);
	DWORD dwError = GetLastError();
	if (mutex == NULL)
	{
		CLogger::Log(CStringUtil::Format(_T("CreateMutex %s failed, error: %d"), mutexName.GetString(), dwError));
	}

	if (dwError == ERROR_ALREADY_EXISTS)
	{
		CLogger::Log(_T("Paste uac admin exe is already running just signalling paste"));
	}
	else
	{
		wchar_t szPath[MAX_PATH];
		if (GetModuleFileName(NULL, szPath, ARRAYSIZE(szPath)))
		{
			// Launch itself as administrator.
			SHELLEXECUTEINFO sei = { sizeof(sei) };
			sei.lpVerb = L"runas";
			sei.lpFile = szPath;
			CString csParam;
			csParam.Format(_T("/uacpaste:%d"), GetCurrentProcessId());
			sei.lpParameters = csParam;
			sei.nShow = SW_NORMAL;

			if (!ShellExecuteEx(&sei))
			{
				CLogger::Log(_T("Failed to startup paste as admin app, we are not pasting using admin app"));
				ret = false;
			}
			else
			{
				CLogger::Log(_T("Startup up ditto paste as admin app, this will send ctrl-v to the admin app"));
			}
		}
	}

	if (mutex != NULL)
	{
		CloseHandle(mutex);
	}

	return ret;
}

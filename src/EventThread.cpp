#include "StdAfx.h"
#include "EventThread.h"
#include "Misc.h"

#include <sddl.h>

#include <memory>
#include <stdexcept>
#include <string>

CEventThread::CEventThread(void)
{
	m_hEvt = CreateEvent(NULL, FALSE, FALSE, NULL);
	m_waitTimeout = INFINITE;
	m_threadRunning = false;
	m_exitThread = false;
	m_threadWasStarted = false;

	AddEvent(ExitEvent);
	AddEvent(RebuildEvents);
}

CEventThread::~CEventThread(void)
{
	Stop();

	for (EventMapType::iterator it = m_eventMap.begin(); it != m_eventMap.end(); it++)
	{
		CloseHandle(it->first);
	}
}

UINT CEventThread::EventThreadFnc(void* thisptr)
{
	CEventThread* threadClass = (CEventThread*)thisptr;
	threadClass->RunThread();
	return 0;
}

void CEventThread::AddEvent(int eventId)
{
	HANDLE handle = CreateEvent(NULL, FALSE, FALSE, _T(""));

	{
		ATL::CCritSecLock csLock(m_lock.m_sect);
		m_eventMap[handle] = eventId;
	}

	if (m_threadRunning)
	{
		FireEvent(RebuildEvents);
	}
}

void CEventThread::AddEvent(int eventId, HANDLE handle)
{
	{
		ATL::CCritSecLock csLock(m_lock.m_sect);
		m_eventMap[handle] = eventId;
	}

	if (m_threadRunning)
	{
		FireEvent(RebuildEvents);
	}
}

void CEventThread::AddEvent(int eventId, CString name)
{
	//handle creating events cross users/cross process
	//https://stackoverflow.com/questions/29976596/shared-global-event-between-a-service-user-mode-processes-doesnt-work
	// The elevated and the normal Ditto process (different tokens of the same user) both open these
	// Global\UAC_* events with EVENT_ALL_ACCESS. Upstream used a NULL DACL (everyone, anonymous
	// included); all access for Authenticated Users covers both processes.
	PSECURITY_DESCRIPTOR descriptor{};
	if (!ConvertStringSecurityDescriptorToSecurityDescriptor(_T("D:(A;;GA;;;AU)"), SDDL_REVISION_1, &descriptor, nullptr))
	{
		throw std::runtime_error("could not build the security descriptor of a UAC event (error " + std::to_string(::GetLastError()) + ")");
	}
	const std::unique_ptr<void, decltype(&::LocalFree)> descriptorOwner(descriptor, &::LocalFree);

	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = descriptor;

	HANDLE handle = CreateEvent(&sa, FALSE, FALSE, name);
	if (handle == NULL)
	{
		throw std::runtime_error("could not create a UAC event (error " + std::to_string(::GetLastError()) + ")");
	}

	{
		ATL::CCritSecLock csLock(m_lock.m_sect);
		m_eventMap[handle] = eventId;
	}

	if (m_threadRunning)
	{
		FireEvent(RebuildEvents);
	}
}

bool CEventThread::FireEvent(int eventId)
{
	HANDLE eventHandle = GetHandle(eventId);
	if (eventHandle != nullptr)
	{
		SetEvent(eventHandle);
		return true;
	}

	return false;
}

bool CEventThread::UndoFireEvent(int eventId)
{
	HANDLE eventHandle = GetHandle(eventId);
	if (eventHandle != nullptr)
	{
		ResetEvent(eventHandle);
		return true;
	}

	return false;
}

HANDLE CEventThread::GetHandle(int eventId)
{
	ATL::CCritSecLock csLock(m_lock.m_sect);
	for (auto it = m_eventMap.begin(); it != m_eventMap.end(); it++)
	{
		if (it->second == eventId)
		{
			return it->first;
		}
	}

	return nullptr;
}

bool CEventThread::RemoveEvent(int eventId)
{
	ATL::CCritSecLock csLock(m_lock.m_sect);
	for (auto it = m_eventMap.begin(); it != m_eventMap.end(); it++)
	{
		if (it->second == eventId)
		{
			if (m_threadRunning)
			{
				FireEvent(RebuildEvents);
			}

			CloseHandle(it->first);
			m_eventMap.erase(it);

			return true;
		}
	}

	return false;
}

void CEventThread::Start(void* param)
{
	if (m_threadRunning == false)
	{
		ResetEvent(m_hEvt);
		m_exitThread = false;
		m_param = param;
		m_thread = (HANDLE)_beginthreadex(NULL, 0, EventThreadFnc, this, 0, &m_threadID);

		// now wait until the thread is up and really running
		WaitForSingleObject(m_hEvt, 1000);
	}
	else
	{
		UndoFireEvent(ExitEvent);
	}
}

void CEventThread::WaitForThreadToExit(int waitTime)
{
	WaitForSingleObject(m_hEvt, waitTime);
}

void CEventThread::Stop(int waitTime)
{
	CLogger::Log(CStringUtil::Format(_T("Start of CEventThread::Stop(int waitTime) %d - Name: %s"), waitTime, m_threadName.GetString()));

	if (m_threadRunning)
	{
		m_exitThread = true;
		FireEvent(ExitEvent);

		if (waitTime > 0)
		{
			// wait on the thread handle (signalled for good once the thread has ended) and log each
			// period it is late. Upstream killed it with TerminateThread after waitTime, which can
			// leave a lock it held (the database lock, the heap) taken forever; a thread that never
			// ends now shows in the log by name.
			while (WAIT_TIMEOUT == WaitForSingleObject(m_thread, waitTime))
			{
				CLogger::Log(CStringUtil::Format(_T("CEventThread::Stop - %s has not ended after another %d ms, still waiting"), m_threadName.GetString(), waitTime));
			}
		}
	}

	CLogger::Log(CStringUtil::Format(_T("End of CEventThread::Stop(int waitTime) %d - Name: %s"), waitTime, m_threadName.GetString()));
};

void CEventThread::GetHandleVector(std::vector<HANDLE>& handles)
{
	ATL::CCritSecLock csLock(m_lock.m_sect);
	handles.clear();
	for (auto it = m_eventMap.begin(); it != m_eventMap.end(); it++)
	{
		if (it->first != 0)
		{
			handles.push_back(it->first);
		}
	}
}

void CEventThread::CheckForRebuildHandleVector(std::vector<HANDLE>& handles)
{
	ATL::CCritSecLock csLock(m_lock.m_sect);
	for (auto it = m_eventMap.begin(); it != m_eventMap.end(); it++)
	{
		if (it->second == RebuildEvents)
		{
			DWORD result = WaitForSingleObject(it->first, 0);
			if (result == WAIT_OBJECT_0)
			{
				GetHandleVector(handles);
			}
			break;
		}
	}
}

void CEventThread::RunThread()
{
	CLogger::Log(CStringUtil::Format(_T("Start of CEventThread::RunThread() Name: %s"), m_threadName.GetString()));

	m_threadRunning = true;
	m_threadWasStarted = true;
	std::vector<HANDLE> handles;

	GetHandleVector(handles);

	SetEvent(m_hEvt);
	ResetEvent(m_hEvt);

	while (m_exitThread == false)
	{
		CheckForRebuildHandleVector(handles);

		DWORD event = WaitForMultipleObjects((DWORD)handles.size(), handles.data(), FALSE, m_waitTimeout);

		if (event == WAIT_FAILED)
		{
			const DWORD errorMessageId = GetLastError();
			LPSTR messageBuffer = nullptr;
			size_t size = FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, errorMessageId, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, nullptr);

			CString message(messageBuffer, (int)size);

			LocalFree(messageBuffer);

			CLogger::Log(CStringUtil::Format(_T("CEventThread::RunThread() Error, error: %s - Name %s"), message.GetString(), m_threadName.GetString()));

			Sleep(1000);
		}
		else if (event == WAIT_TIMEOUT)
		{
			OnTimeOut(m_param);
		}
		else
		{
			const int handleIndex = event - WAIT_OBJECT_0;
			if (handleIndex < 0 || static_cast<size_t>(handleIndex) >= handles.size())
			{
				CLogger::Log(CStringUtil::Format(_T("CEventThread::RunThread() Error, Invalid handle index, index: %d, size: %d - Name %s"), handleIndex, handles.size(), m_threadName.GetString()));
				continue;
			}

			HANDLE firedHandle = handles[handleIndex];
			const int eventId = m_eventMap[firedHandle];
			if (eventId == ExitEvent)
			{
				break;
			}
			else if (eventId == RebuildEvents)
			{
				GetHandleVector(handles);
			}
			else
			{
				CLogger::Log(CStringUtil::Format(_T("Start of CEventThread::RunThread() - OnEvent %d - Name %s"), eventId, m_threadName.GetString()));
				OnEvent(eventId, m_param);
				CLogger::Log(CStringUtil::Format(_T("End of CEventThread::RunThread() - OnEvent %d - Name: %s"), eventId, m_threadName.GetString()));
			}
		}
	}

	UndoFireEvent(ExitEvent);

	SetEvent(m_hEvt);

	CLogger::Log(CStringUtil::Format(_T("End of CEventThread::RunThread() Name: %s"), m_threadName.GetString()));

	m_threadRunning = false;
}

#include "stdafx.h"
#include "CP_Main.h" // the settings and Misc types in the order they need
#include "AppState.h"

CLastAddedClip::Entry CLastAddedClip::Get() const
{
	const std::scoped_lock lock{m_lock};
	return m_entry;
}

void CLastAddedClip::Record(DWORD crc, int id)
{
	const std::scoped_lock lock{m_lock};
	m_entry = Entry{.crc = crc, .id = id};
}

void CLastAddedClip::ClearCrc()
{
	const std::scoped_lock lock{m_lock};
	m_entry.crc = 0;
}

CAppState::CAppState(CGetSetOptions& settings) :
	m_oldtStartUp(COleDateTime::GetCurrentTime()),
	m_settings(settings)
{
}

void CAppState::SaveCurrentGroupState()
{
	m_oldGroupID = m_GroupID;
	m_oldGroupParentID = m_GroupParentID;
	m_oldGroupText = m_GroupText;
}

void CAppState::ClearOldGroupState()
{
	m_oldGroupID = -2;
	m_oldGroupParentID = -2;
	m_oldGroupText = _T("");
}

long CAppState::GetValidGroupID() const
{
	return m_GroupID;
}

void CAppState::SetActiveGroupId(int groupId)
{
	m_activeGroupId = groupId;
	m_activeGroupStartTime = GetTickCount64();
}

int CAppState::GetActiveGroupId()
{
	int ret = -1;
	ULONGLONG maxDiff = m_settings.GetSaveToGroupTimeoutMS();
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

void CAppState::SetCopyReason(CopyReasonEnum::CopyReason copyReason)
{
	m_copyReason = copyReason;
	m_copyReasonStartTime = GetTickCount64();
}

CopyReasonEnum::CopyReason CAppState::GetCopyReason()
{
	CopyReasonEnum::CopyReason ret = CopyReasonEnum::COPY_TO_UNKOWN;
	ULONGLONG maxDiff = m_settings.GetCopyReasonTimeoutMS();
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

CLastAddedClip& CAppState::LastAddedClip()
{
	return m_lastAddedClip;
}

long CAppState::AddTaskbarIconUser()
{
	return ++m_taskbarIconUsers;
}

long CAppState::ReleaseTaskbarIconUser()
{
	return --m_taskbarIconUsers;
}

std::mutex& CAppState::LogFileLock()
{
	return m_logFileLock;
}

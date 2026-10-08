#include "stdafx.h"
#include "CP_Main.h" // the quick paste window and the database types in the order they need
#include "GroupNavigator.h"
#include "AppState.h"
#include "AppWindows.h"
#include "ErrorReport.h"

CGroupNavigator::CGroupNavigator(CAppState& state, CDittoDb& database, CAppWindows& windows) :
	m_state(state),
	m_database(database),
	m_windows(windows)
{
}

BOOL CGroupNavigator::TryEnterOldGroupState()
{
	BOOL enteredGroup = FALSE;

	if (m_state.m_oldGroupID > -2)
	{
		m_state.m_GroupID = m_state.m_oldGroupID;
		m_state.m_GroupParentID = m_state.m_oldGroupParentID;
		m_state.m_GroupText = m_state.m_oldGroupText;

		m_state.ClearOldGroupState();

		RefreshAfterGroupChange();

		enteredGroup = TRUE;
	}

	return enteredGroup;
}

BOOL CGroupNavigator::EnterGroupID(long lID, BOOL clearOldGroupState /* = TRUE*/, BOOL saveCurrentGroupState /* = FALSE*/)
{
	BOOL bResult = FALSE;

	if (m_state.m_GroupID == lID)
		return TRUE;

	ULONGLONG startTick = GetTickCount64();

	if (clearOldGroupState)
	{
		m_state.ClearOldGroupState();
	}

	if (saveCurrentGroupState)
	{
		m_state.SaveCurrentGroupState();
	}

	// if we are switching to the parent, focus on the previous group
	if (m_state.m_GroupParentID == lID && m_state.m_GroupID > 0)
		m_state.m_FocusID = m_state.m_GroupID;

	if (!OpenGroup(lID, bResult))
	{
		return FALSE;
	}

	FinishEnterGroup(bResult, startTick);

	return bResult;
}

bool CGroupNavigator::OpenGroup(long lID, BOOL& bResult)
{
	switch (lID)
	{
	case -1:
		m_state.m_FocusID = -1;
		m_state.m_GroupID = -1;
		m_state.m_GroupParentID = -1;
		m_state.m_GroupText = "History";
		bResult = TRUE;
		break;
	default: // Normal Group
		try
		{
			bResult = EnterStoredGroup(lID);
		}
		catch (CppSQLite3Exception& e)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Opening group id %ld failed: %s"), lID, e.errorMessage()));
			return false;
		}
		break;
	}

	return true;
}

void CGroupNavigator::FinishEnterGroup(BOOL bResult, ULONGLONG startTick)
{
	if (bResult)
	{
		RefreshAfterGroupChange();
	}

	ULONGLONG endTick = GetTickCount64();
	if ((endTick - startTick) > 150)
		CLogger::Log(CStringUtil::Format(_T("Paste Timing EnterParentId: %llu"), endTick - startTick));
}

void CGroupNavigator::RefreshAfterGroupChange()
{
	m_windows.RefreshView();
	if (m_windows.QPasteWnd())
		m_windows.QPasteWnd()->UpdateStatus(true);
}

BOOL CGroupNavigator::EnterStoredGroup(long lID)
{
	BOOL bResult{ FALSE };
	CppSQLite3Query q{ m_database.execQueryEx(_T("SELECT lParentID, mText, bIsGroup FROM Main WHERE lID = %d"), lID) };
	if (q.eof() == false)
	{
		if (q.getIntField(_T("bIsGroup")) > 0)
		{
			m_state.m_GroupID = lID;
			m_state.m_GroupParentID = q.getIntField(_T("lParentID"));
			m_state.m_GroupText = q.getStringField(_T("mText"));
			bResult = TRUE;
		}
	}

	return bResult;
}

void CGroupNavigator::SetGroupDefaultID(long lID)
{
	if (m_state.m_GroupDefaultID == lID)
	{
		return;
	}

	if (lID <= 0)
	{
		m_state.m_GroupDefaultID = 0;
	}
	else
	{
		m_state.m_GroupDefaultID = lID;
	}

	if (m_windows.QPasteWnd())
	{
		m_windows.QPasteWnd()->UpdateStatus();
	}
}

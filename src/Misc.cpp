#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"

// app-wide state, read and set by CIdleTime::IdleSeconds only; left for the composition root (Phase L3d)
int g_funnyGetTickCountAdjustment = -1;

double CIdleTime::IdleSeconds()
{
	LASTINPUTINFO info{};
	info.cbSize = sizeof(info);
	GetLastInputInfo(&info);
	// Compared with LASTINPUTINFO::dwTime, a 32-bit tick value, so keep 32-bit wrap-around arithmetic.
	DWORD currentTick{ static_cast<DWORD>(GetTickCount64()) };

	if(g_funnyGetTickCountAdjustment == -1)
	{
		if(currentTick < info.dwTime)
		{
			g_funnyGetTickCountAdjustment = 1;
		}
		else
		{
			g_funnyGetTickCountAdjustment = 0; 
		}		
	}
	
	if(g_funnyGetTickCountAdjustment == 1 || g_funnyGetTickCountAdjustment == 2)
	{
		//Output message the first time
		if(g_funnyGetTickCountAdjustment == 1)
		{
			CLogger::Log(CStringUtil::Format(_T("Adjusting time of get tickcount by: %d, on startup we found GetTickCount to be less than last input"), CGetSetOptions::GetFunnyTickCountAdjustment()));
			g_funnyGetTickCountAdjustment = 2;
		}
		currentTick += CGetSetOptions::GetFunnyTickCountAdjustment();
	}

	const double idleSeconds{ (currentTick - info.dwTime)/1000.0 };

	return idleSeconds;
}

int CMarkerInserter::Insert(CString& mainStr, CString& findStr, CString preInsert, CString postInsert, int linesPerRow)
{
	int replaceCount = 0;

	//Prevent infinite loop when user tries to replace nothing.
	if (findStr != "")
	{
		const InsertResult inserted{ InsertMarkers(mainStr, findStr, preInsert, postInsert) };
		replaceCount = inserted.replaceCount;

		TrimLeadingLines(mainStr, inserted.firstFindPos, linesPerRow);

		if(replaceCount > 0)
		{
			//use unprintable characters so it doesn't find copied html to convert
			mainStr.Replace(_T("\r\n"), _T("\x01\x05\x02"));
			mainStr.Replace(_T("\r"), _T("\x01\x05\x02"));
			mainStr.Replace(_T("\n"), _T("\x01\x05\x02"));
		}
	}

	return replaceCount;
}

CMarkerInserter::InsertResult CMarkerInserter::InsertMarkers(CString& mainStr, CString& findStr, const CString& preInsert, const CString& postInsert)
{
	InsertResult result{};

	int oldLen = findStr.GetLength();

	int foundPos = 0;
	int startFindPos = 0;
	int newPos = 0;
	int insertedLength = 0;

	CString mainLow(theApp.m_icuString.ToLowerStringEx(mainStr));
	CString findLow(theApp.m_icuString.ToLowerStringEx(findStr));
	findLow.MakeLower();

	int preLength = preInsert.GetLength();
	int postLength = postInsert.GetLength();

	while(TRUE)
	{
		foundPos = mainLow.Find(findLow, startFindPos);
		if (foundPos < 0)
			break;

		if (result.replaceCount == 0)
		{
			result.firstFindPos = foundPos + preLength;
		}

		newPos = foundPos + insertedLength;

		mainStr.Insert(newPos, preInsert);
		mainStr.Insert(newPos + preLength + oldLen, postInsert);

		startFindPos = foundPos + oldLen;

		insertedLength += preLength + postLength;

		result.replaceCount++;

		//safety check, make sure we don't look forever
		if (result.replaceCount > 100)
			break;
	}

	return result;
}

void CMarkerInserter::TrimLeadingLines(CString& mainStr, int firstFindPos, int linesPerRow)
{
	int foundPos = 0;
	int startFindPos = 0;
	int line = 0;
	int prevLinePos = 0;
	int prevPrevLinePos = 0;

	while (TRUE)
	{
		foundPos = mainStr.Find(_T("\n"), startFindPos);
		if (foundPos < 0)
			break;

		if (firstFindPos < foundPos)
		{
			if (line > linesPerRow - 1)
			{
				int lineStart = prevLinePos;
				if (linesPerRow > 1)
				{
					lineStart = prevPrevLinePos;
				}

				mainStr = _T("... ") + mainStr.Mid(lineStart + 1);
			}

			break;
		}

		startFindPos = foundPos + 1;
		prevPrevLinePos = prevLinePos;
		prevLinePos = foundPos;

		line++;

		//safety check, make sure we don't look forever
		if (line > 1000)
			break;
	}
}

void CMenuPopupUpdater::Update(CMenu *pPopupMenu, CWnd *pWnd)
{
	ASSERT(pPopupMenu != NULL);
	// Check the enabled state of various menu items.

	CCmdUI state;
	state.m_pMenu = pPopupMenu;
	ASSERT(state.m_pOther == NULL);
	ASSERT(state.m_pParentMenu == NULL);

	FindParentMenu(state, pPopupMenu, pWnd);

	state.m_nIndexMax = pPopupMenu->GetMenuItemCount();
	for (state.m_nIndex = 0; state.m_nIndex < state.m_nIndexMax;
		state.m_nIndex++)
	{
		UpdateItem(state, pPopupMenu, pWnd);
	}
}

void CMenuPopupUpdater::FindParentMenu(CCmdUI& state, CMenu *pPopupMenu, CWnd *pWnd)
{
	// Determine if menu is popup in top-level menu and set m_pOther to
	// it if so (m_pParentMenu == NULL indicates that it is secondary popup).
	HMENU hParentMenu{};
	if (AfxGetThreadState()->m_hTrackingMenu == pPopupMenu->m_hMenu)
	{
		state.m_pParentMenu = pPopupMenu;    // Parent == child for tracking popup.
	}
	else if ((hParentMenu = ::GetMenu(pWnd->m_hWnd)) != NULL)
	{
		CWnd* pParent = pWnd;
		// Child windows don't have menus--need to go to the top!
		if (pParent != NULL &&
			(hParentMenu = ::GetMenu(pParent->m_hWnd)) != NULL)
		{
			int nIndexMax = ::GetMenuItemCount(hParentMenu);
			for (int nMenuIndex = 0; nMenuIndex < nIndexMax; nMenuIndex++)
			{
				if (::GetSubMenu(hParentMenu, nMenuIndex) == pPopupMenu->m_hMenu)
				{
					// When popup is found, m_pParentMenu is containing menu.
					state.m_pParentMenu = CMenu::FromHandle(hParentMenu);
					break;
				}
			}
		}
	}
}

void CMenuPopupUpdater::UpdateItem(CCmdUI& state, CMenu *pPopupMenu, CWnd *pWnd)
{
	state.m_nID = pPopupMenu->GetMenuItemID(state.m_nIndex);
	if (state.m_nID == 0)
		return; // Menu separator or invalid cmd - ignore it.

	ASSERT(state.m_pOther == NULL);
	ASSERT(state.m_pMenu != NULL);
	if (state.m_nID == (UINT)-1)
	{
		// Possibly a popup menu, route to first item of that popup.
		state.m_pSubMenu = pPopupMenu->GetSubMenu(state.m_nIndex);
		if (state.m_pSubMenu == NULL ||
			(state.m_nID = state.m_pSubMenu->GetMenuItemID(0)) == 0 ||
			state.m_nID == (UINT)-1)
		{
			return;       // First item of popup can't be routed to.
		}
		state.DoUpdate(pWnd, TRUE);   // Popups are never auto disabled.
	}
	else
	{
		// Normal menu item.
		// Auto enable/disable if frame window has m_bAutoMenuEnable
		// set and command is _not_ a system command.
		state.m_pSubMenu = NULL;
		state.DoUpdate(pWnd, FALSE);
	}

	AdjustForMenuChanges(state, pPopupMenu);
}

void CMenuPopupUpdater::AdjustForMenuChanges(CCmdUI& state, CMenu *pPopupMenu)
{
	// Adjust for menu deletions and additions.
	UINT nCount = pPopupMenu->GetMenuItemCount();
	if (nCount < state.m_nIndexMax)
	{
		state.m_nIndex -= (state.m_nIndexMax - nCount);
		while (state.m_nIndex < nCount &&
			pPopupMenu->GetMenuItemID(state.m_nIndex) == state.m_nID)
		{
			state.m_nIndex++;
		}
	}
	state.m_nIndexMax = nCount;
}

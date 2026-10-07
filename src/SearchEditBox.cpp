// SearchEditBox.cpp : implementation file
//

#include "stdafx.h"
#include "cp_main.h"
#include "SearchEditBox.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CSearchEditBox

CSearchEditBox::CSearchEditBox()
{
}

CSearchEditBox::~CSearchEditBox()
{
}


BEGIN_MESSAGE_MAP(CSearchEditBox, CEdit)
	//{{AFX_MSG_MAP(CSearchEditBox)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSearchEditBox message handlers

BOOL CSearchEditBox::PreTranslateMessage(MSG* pMsg) 
{
	if(pMsg->message == WM_KEYDOWN &&
		HandleKeyDown(pMsg))
	{
		return TRUE;
	}

	return CEdit::PreTranslateMessage(pMsg);
}

bool CSearchEditBox::HandleKeyDown(const MSG* pMsg)
{
	if(pMsg->wParam == VK_RETURN)
	{
		HandleReturnKey();
		return true;
	}
	else if (IsListNavigationKey(pMsg->wParam))
	{
		if(CGetSetOptions::m_bFindAsYouType)
		{
			return SendKeyToParent(pMsg);
		}
	}
	else if(IsCutCopyDeleteKey(pMsg->wParam))
	{
		LONG lEditSel = GetSel();
		if(LOWORD(lEditSel) == HIWORD(lEditSel))
		{
			return SendKeyToParent(pMsg);
		}
	}

	return false;
}

void CSearchEditBox::HandleReturnKey()
{
	CWnd *pWnd = GetParent();
	if(pWnd)
	{
		if(CGetSetOptions::m_bFindAsYouType)
		{
			pWnd->SendMessage(NM_SEARCH_ENTER_PRESSED, 0, 0);
		}
		else
		{
			//Send a message to the parent to refill the lb from the search
			pWnd->PostMessage(CB_SEARCH, 0, 0);
		}
	}
}

bool CSearchEditBox::IsListNavigationKey(WPARAM key)
{
	return key == VK_DOWN ||
		key == VK_UP ||
		key == VK_F3;
}

bool CSearchEditBox::IsCutCopyDeleteKey(WPARAM key)
{
	return key == 'C' && CONTROL_PRESSED ||
		key == 'X' && CONTROL_PRESSED ||
		key == VK_DELETE;
}

bool CSearchEditBox::SendKeyToParent(const MSG* pMsg)
{
	CWnd *pWnd = GetParent();
	if(pWnd)
	{
		pWnd->SendMessage(CB_UPDOWN, pMsg->wParam, pMsg->lParam);
		return true;
	}

	return false;
}

// TrayWnd.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "TrayWnd.h"


// CTrayWnd

IMPLEMENT_DYNAMIC(CTrayWnd, CWnd)

CTrayWnd::CTrayWnd()
{
}

CTrayWnd::~CTrayWnd()
{
}

const UINT& CTrayWnd::TaskbarCreatedMessage()
{
	static const UINT message{::RegisterWindowMessage(_T("TaskbarCreated"))};
	return message;
}


BEGIN_MESSAGE_MAP(CTrayWnd, CWnd)
	ON_REGISTERED_MESSAGE(TaskbarCreatedMessage(), OnTaskBarCreated)
END_MESSAGE_MAP()

LRESULT CTrayWnd::OnTaskBarCreated(WPARAM /*wParam*/, LPARAM /*lParam*/)
{
	if(theApp.m_pMainFrame != NULL)
	{
		theApp.m_pMainFrame->PostMessage(CDittoMessage::ReaddTaskbarIcon, 0, 0);
	}
	
	return TRUE;
}



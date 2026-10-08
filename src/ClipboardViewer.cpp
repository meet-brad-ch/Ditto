// ClipboardViewer.cpp : implementation file
//

#include "stdafx.h"
#include "cp_main.h"
#include "ClipboardViewer.h"
#include "Misc.h"
#include "ErrorReport.h"
#include "..\Shared\Tokenizer.h"
#include "WildCardMatch.h"

/////////////////////////////////////////////////////////////////////////////
// CClipboardViewer

CGetSetOptions& CClipboardViewer::Settings() const
{
	return theApp.Services().Settings();
}

CClipboardViewer::CClipboardViewer(CCopyThread* pHandler) :
	m_pHandler(pHandler),
	m_bPinging(false),
	m_bIsConnected(false),
	m_bConnect(false),
	m_dwLastCopy(0),
	m_connectOnStartup(true)
{
	m_activeWindow = _T("");
}

CClipboardViewer::~CClipboardViewer()
{
}


BEGIN_MESSAGE_MAP(CClipboardViewer, CWnd)
	//{{AFX_MSG_MAP(CClipboardViewer)
	ON_WM_CREATE()
	ON_WM_TIMER()
	ON_WM_DESTROY()
	//}}AFX_MSG_MAP
	ON_MESSAGE(CDittoMessage::SetConnect, OnSetConnect)
	ON_MESSAGE(WM_CLIPBOARDUPDATE, OnClipboardChange)
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CClipboardViewer message handlers
void CClipboardViewer::Create()
{
	CString strParentClass = AfxRegisterWndClass(0);
	CWnd::CreateEx(0, strParentClass, _T("Ditto Clipboard Viewer"), 0, -1, -1, 0, 0, 0, 0);

	if(m_connectOnStartup)
	{
		SetConnect(true);
	}
}

// connects as a clipboard format listener; a failure is shown to the user, since no copy is saved without it
void CClipboardViewer::Connect()
{
	CLogger::Log(_T("Connect to Clipboard"));

	if(!::AddClipboardFormatListener(m_hWnd))
	{
		CErrorReport::Show(CStringUtil::Format(_T("Ditto could not listen for clipboard changes (AddClipboardFormatListener failed, error %u). Copies are not saved."), ::GetLastError()));
		return;
	}

	m_bIsConnected = true;
	m_bConnect = true;

	SetEnsureConnectedTimer();
}

void CClipboardViewer::SetEnsureConnectedTimer()
{
	SetTimer(TimerEnsureViewerInChain, CMilliseconds::OneMinute*5, NULL);
}

// disconnects as a clipboard viewer
void CClipboardViewer::Disconnect(bool bSendPing)
{
	CLogger::Log(_T("Disconnect From Clipboard"));

	KillTimer(TimerEnsureViewerInChain);

	if(m_bIsConnected && !::RemoveClipboardFormatListener(m_hWnd))
	{
		CErrorReport::Show(CStringUtil::Format(_T("Ditto could not stop listening for clipboard changes (RemoveClipboardFormatListener failed, error %u)."), ::GetLastError()));
	}

	m_bConnect = false;
	m_bIsConnected = false;
	if(bSendPing)
		SendPing();
}

void CClipboardViewer::SendPing()
{
	if(Settings().m_bEnsureConnectToClipboard)
	{
		if(OpenClipboard())
		{
			m_bPinging = true;
			SetClipboardData(theApp.m_PingFormat, CGlobalMemory::NewGlobalP("Ditto Ping", sizeof("Ditto Ping")));
			SetClipboardData(theApp.m_cfIgnoreClipboard , CGlobalMemory::NewGlobalP("Ignore", sizeof("Ignore")));

			SetTimer(TimerPing, 2000, NULL);
			CloseClipboard();
		}
	}
}

void CClipboardViewer::SetConnect(bool bConnect)
{
	m_bConnect = bConnect;
	if(bConnect)
	{
		if(m_bIsConnected == false)
		{
			Connect();
		}
		else
		{
			SendPing();
		}
	}
	else
	{
		Disconnect();
	}
}

/////////////////////////////////////////////////////////////////////////////
// CClipboardViewer message handlers

int CClipboardViewer::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if(CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	//Set up the clip board viewer
	if(m_connectOnStartup)
	{
		Connect();
	}
	
	return 0;
}

void CClipboardViewer::OnDestroy()
{
	Disconnect();
	CWnd::OnDestroy();
}

LRESULT CClipboardViewer::OnClipboardChange(WPARAM /*wParam*/, LPARAM /*lPara*/)
{
	CLogger::Log(CStringUtil::Format(_T("OnClipboardChange - Start")));
	ProcessClipboardChange();
	CLogger::Log(CStringUtil::Format(_T("OnClipboardChange - End")));

	return TRUE;
}

bool CClipboardViewer::GetIgnoreClipboardChange()
{
	if(::IsClipboardFormatAvailable(theApp.m_cfIgnoreClipboard))
	{
		CLogger::Log(_T("Clipboard Viewer Ignore clipboard format is on the clipboard, ignoring change"));
		return true;
	}

	if (Settings().m_enforceClipboardIgnoreFormats == false)
	{
		return false;
	}

	//https://learn.microsoft.com/en-us/windows/win32/dataxchg/clipboard-formats
	if (::IsClipboardFormatAvailable(theApp.m_excludeClipboardContentFromMonitorProcessing))
	{
		CLogger::Log(_T("ExcludeClipboardContentFromMonitorProcessing clipboard format is on the clipboard, ignoring change"));
		return true;
	}

	return false;
}

//The clipboard data has changed
void CClipboardViewer::ProcessClipboardChange()
{
	if(::IsClipboardFormatAvailable(theApp.m_PingFormat))
	{
		m_bPinging = false;
		return;
	}

	if(m_pHandler)
	{
		if(m_bIsConnected)
		{
			if(GetIgnoreClipboardChange() == false)
			{
				if(ValidActiveWnd())
				{          
					CLogger::Log(CStringUtil::Format(_T("OnDrawClipboard:: *** SetTimer *** %llu"), GetTickCount64()));

					KillTimer(TimerDrawClipboard);
					SetTimer(TimerDrawClipboard, Settings().m_lProcessDrawClipboardDelay, NULL);		
				}
			}
		}
		else
		{
			CLogger::Log(_T("Not connected, ignore clipboard change"));
		}
	}
}

bool CClipboardViewer::ValidActiveWnd()
{
	UpdateActiveWindowName();

	CString includeApps = Settings().GetCopyAppInclude().MakeLower();

	CLogger::Log(CStringUtil::Format(_T("INCLUDE app names: %s, Active App: %s"), includeApps.GetString(), m_activeWindow.GetString()));

	CString line;
	if(FindAppMatch(includeApps, line) == false)
	{
		CLogger::Log(CStringUtil::Format(_T("Didn't find a match to INCLUDE match %s, NOT SAVING COPY"), includeApps.GetString()));
		return false;
	}

	CLogger::Log(CStringUtil::Format(_T("Inlclude app names Found Match %s - %s"), line.GetString(), m_activeWindow.GetString()));

	CString excludeApps = Settings().GetCopyAppExclude().MakeLower();

	if(excludeApps != "")
	{
		CLogger::Log(CStringUtil::Format(_T("EXCLUDE app names %s, Active App: %s"), excludeApps.GetString(), m_activeWindow.GetString()));

		CString line2;
		if(FindAppMatch(excludeApps, line2))
		{
			CLogger::Log(CStringUtil::Format(_T("Exclude app names Found Match %s - %s - NOT SAVING COPY"), line2.GetString(), m_activeWindow.GetString()));

			return false;
		}
	}

	return true;
}

bool CClipboardViewer::FindAppMatch(const CString& apps, CString& line)
{
	CTokenizer token(apps, Settings().GetCopyAppSeparator());

	while(token.Next(line))
	{
		if(line != "")
		{
			if(CWildCardMatch::WildMatch(line.Trim(), m_activeWindow, ""))
			{
				return true;
			}
		}
	}

	return false;
}

void CClipboardViewer::UpdateActiveWindowName()
{
	m_activeWindow = _T("");

	HWND owner = ::GetClipboardOwner();
	if (owner != NULL)
	{
		DWORD PID = 0;
		::GetWindowThreadProcessId(owner, &PID);

		if (PID != 0)
		{
			m_activeWindow = CWindowInspector::GetProcessName(NULL, PID);
		}
	}

	//L"RuntimeBroker.exe" is what all modern apps report as
	if (m_activeWindow == _T(""))
	{
		HWND active = ::GetForegroundWindow();
		m_activeWindow = CWindowInspector::GetProcessName(active, 0);
	}

	m_activeWindow = m_activeWindow.MakeLower();
}

void CClipboardViewer::OnTimer(UINT_PTR nIDEvent)
{
	switch(nIDEvent)
	{
	case TimerEnsureViewerInChain:
		SendPing();
		break;

	case TimerDrawClipboard:
		OnDrawClipboardTimer(nIDEvent);
		break;

	case TimerPing:
		OnPingTimer();
		break;
	}

	CWnd::OnTimer(nIDEvent);
}

void CClipboardViewer::OnDrawClipboardTimer(UINT_PTR nIDEvent)
{
	KillTimer(nIDEvent);

	ULONGLONG dwNow = GetTickCount64();

	if(dwNow - m_dwLastCopy > Settings().m_dwSaveClipDelay || m_dwLastCopy > dwNow)
	{
		if (GetIgnoreClipboardChange() == false)
		{
			CLogger::Log(CStringUtil::Format(_T("OnDrawClipboard::OnTimer %llu"), dwNow));

			m_pHandler->OnClipboardChange(m_activeWindow);

			m_dwLastCopy = dwNow;
		}
	}
	else
	{
		CLogger::Log(CStringUtil::Format(_T("Clip copy to fast difference from last copy = %llu"), (dwNow - m_dwLastCopy)));
	}

	m_activeWindow = _T("");
}

void CClipboardViewer::OnPingTimer()
{
	KillTimer(TimerPing);

	//If we haven't received the change clipboard message then we are disconnected
	//if so reconnect
	if(m_bPinging)
	{
		if(m_bConnect)
		{
			CLogger::Log(_T("Ping Failed Reconnecting to clipboard"));
			Disconnect(false);
			Connect();
		}
		else
		{
			CLogger::Log(_T("Ping Failed but Connected set to FALSE so this is ok"));
		}
	}
	else
	{
		if(m_bConnect)
		{
			m_bIsConnected = true;
		}
	}
}

LRESULT CClipboardViewer::OnSetConnect(WPARAM wParam, LPARAM /*lParam*/)
{
	bool bConnect = wParam == TRUE;
	SetConnect(bConnect);
	return TRUE;
}

// CopyThread.cpp : implementation file
//

#include "stdafx.h"
#include "cp_main.h"
#include "CopyThread.h"
#include "ErrorReport.h"
#include "ClipboardFormatError.h"

#include <memory>

/////////////////////////////////////////////////////////////////////////////
// CCopyThread

IMPLEMENT_DYNCREATE(CCopyThread, CWinThread)

CCopyThread::CCopyThread():
	m_bQuit(false),
	m_bConfigChanged(false),
	m_connectOnStartup(true)
{
	m_bAutoDelete = false;
}

CCopyThread::~CCopyThread()
{
}

BOOL CCopyThread::InitInstance()
{
	m_pClipboardViewer = std::make_unique<CClipboardViewer>(this);
	m_pClipboardViewer->m_connectOnStartup = m_connectOnStartup;

	// the window is created within this thread and therefore uses its message queue
	m_pClipboardViewer->Create();

	return TRUE;
}

int CCopyThread::ExitInstance()
{
	m_pClipboardViewer->Disconnect(false);

	return CWinThread::ExitInstance();
}

// Called within Copy Thread:
void CCopyThread::OnClipboardChange(CString activeWindow)
{
	CLogger::Log(_T("OnClipboardChange - Start"));

	SyncConfig(); // synchronize with the main thread's copy configuration
	
	// if we are told not to copy on change, then we have nothing to do.
	if(!m_LocalConfig.m_bCopyOnChange)
		return;
	
	int groupId = Services().State().GetActiveGroupId();
	if(groupId > -1)
	{
		CLogger::Log(CStringUtil::Format(_T("LoadFromClipboard - loading clips into groupId: %d"), groupId));
	}

	auto pClip = std::make_unique<CClip>(Services().ClipContext());
	pClip->m_copyReason = Services().State().GetCopyReason();

	COleDataObjectEx oleData;
	CClipTypes* pSupportedTypes = m_LocalConfig.m_pSupportedTypes.get();

	// If we are copying from a Ditto Buffer or use advanced option
	// then save all to the database, so when we paste this it will paste 
	// just like you were using Ctrl-V
	std::shared_ptr<CClipTypes> availableTypes;
	if (Services().CopyBuffer().Active() || Settings().GetSupportAllTypes())
	{
		availableTypes = oleData.GetAvailableTypes(Services().Windows().MainHwnd());
		pSupportedTypes = availableTypes.get();
	}

	int bResult = FALSE;
	try
	{
		bResult = LoadClipWithRetry(*pClip, pSupportedTypes, activeWindow);
	}
	catch(const DittoCore::ClipboardFormatError& error)
	{
		// clipboard data comes from other processes: this copy is rejected, Ditto keeps running
		CErrorReport::Show(CStringUtil::Format(_T("A copy from %s was not saved: its clipboard data is malformed (%s)."),
			activeWindow.GetString(), CString(error.what()).GetString()));
		return;
	}

	pSupportedTypes = NULL;

	if(bResult != TRUE)
	{
		return; // nothing to save
	}

	if(groupId > -1)
	{
		pClip->m_parentId = groupId;
	}

	HandOverClip(pClip);

	CLogger::Log(_T("OnClipboardChange - End"));
}

int CCopyThread::LoadClipWithRetry(CClip& clip, CClipTypes* pSupportedTypes, const CString& activeWindow)
{
	CLogger::Log(_T("LoadFromClipboard - Before"));
	int bResult = clip.LoadFromClipboard(pSupportedTypes, Settings().m_regexHelper, true, activeWindow);
	CLogger::Log(_T("LoadFromClipboard - After"));

	if(bResult == FALSE)
	{
		DWORD delay = Settings().GetNoFormatsRetryDelay();
		if(delay > 0)
		{
			CLogger::Log(CStringUtil::Format(_T("LoadFromClipboard didn't find any clips to save, sleeping %dms, then trying again"), delay));
			Sleep(delay);

			CLogger::Log(_T("LoadFromClipboard #2 - Before"));
			bResult = clip.LoadFromClipboard(pSupportedTypes, Settings().m_regexHelper, true, activeWindow);
			CLogger::Log(_T("LoadFromClipboard #2 - After"));
		}
		else
		{
			CLogger::Log(_T("LoadFromClipboard didn't find any clips to save, retry setting is not set, not retrying"));
		}
	}

	return bResult;
}

CGetSetOptions& CCopyThread::Settings() const
{
	return theApp.Services().Settings();
}

CAppServices& CCopyThread::Services() const
{
	return theApp.Services();
}

void CCopyThread::HandOverClip(std::unique_ptr<CClip>& pClip)
{
	// the CDittoMessage::ClipboardCopied handler takes ownership of the clip
	if(m_LocalConfig.m_bAsyncCopy)
	{
		if(::PostMessage(m_LocalConfig.m_hClipHandler, CDittoMessage::ClipboardCopied, reinterpret_cast<WPARAM>(pClip.get()), 0))
		{
			pClip.release(); // ownership: CMainFrame::OnClipboardCopied retakes it in a std::unique_ptr
		}
		else
		{
			CLogger::Log(CStringUtil::Format(_T("Could not post the copied clip to the main window, GetLastError %d"), ::GetLastError()));
		}
	}
	else
	{
		::SendMessage(m_LocalConfig.m_hClipHandler, CDittoMessage::ClipboardCopied, reinterpret_cast<WPARAM>(pClip.release()), 0); // ownership: CMainFrame::OnClipboardCopied retakes it in a std::unique_ptr
	}
}

void CCopyThread::SyncConfig()
{
	if(m_bConfigChanged)
	{
		ATL::CCritSecLock csLock(m_cs.m_sect);

		// taken: the next sync copies only after the next change
		m_bConfigChanged = false;
		m_LocalConfig.CopySettingsFrom(m_SharedConfig);

		// null means that the types shouldn't be sync'ed: m_LocalConfig keeps its types
		if( m_SharedConfig.m_pSupportedTypes )
		{
			// now owned by LocalConfig; its old types are deleted
			m_LocalConfig.m_pSupportedTypes = std::move(m_SharedConfig.m_pSupportedTypes);
		}
	}
}

bool CCopyThread::IsClipboardViewerConnected()
{
	return m_pClipboardViewer->m_bIsConnected;
}

bool CCopyThread::GetConnectCV()
{
	return m_pClipboardViewer->GetConnect();
}

void CCopyThread::SetConnectCV(bool bConnect)
{
	if(m_pClipboardViewer && m_pClipboardViewer->m_hWnd != NULL)
	{
		::SendMessage( m_pClipboardViewer->m_hWnd, CDittoMessage::SetConnect, bConnect, 0 );
	}
}

void CCopyThread::SetSupportedTypes( std::unique_ptr<CClipTypes> pTypes )
{
	ATL::CCritSecLock csLock(m_cs.m_sect);

	// types not yet taken by SyncConfig are deleted
	m_SharedConfig.m_pSupportedTypes = std::move(pTypes);
	m_bConfigChanged = true;
}

HWND CCopyThread::SetClipHandler(HWND hWnd)
{
	ATL::CCritSecLock csLock(m_cs.m_sect);

	HWND hRet = m_SharedConfig.m_hClipHandler;
	m_SharedConfig.m_hClipHandler = hWnd;
	// an unchanged value keeps a change made before that SyncConfig has not taken yet
	m_bConfigChanged = m_bConfigChanged || (hRet != hWnd);

	return hRet;
}
HWND CCopyThread::GetClipHandler()
{
	ATL::CCritSecLock csLock(m_cs.m_sect);

	HWND hRet = m_SharedConfig.m_hClipHandler;

	return hRet;
}
bool CCopyThread::SetCopyOnChange(bool bVal)
{
	ATL::CCritSecLock csLock(m_cs.m_sect);

	bool bRet = m_SharedConfig.m_bCopyOnChange;
	m_SharedConfig.m_bCopyOnChange = bVal;
	m_bConfigChanged = m_bConfigChanged || (bRet != bVal);

	return bRet;
}
bool CCopyThread::GetCopyOnChange()
{
	ATL::CCritSecLock csLock(m_cs.m_sect);

	bool bRet = m_SharedConfig.m_bCopyOnChange;

	return bRet;
}
bool CCopyThread::SetAsyncCopy(bool bVal)
{
	ATL::CCritSecLock csLock(m_cs.m_sect);

	bool bRet = m_SharedConfig.m_bAsyncCopy;
	m_SharedConfig.m_bAsyncCopy = bVal;
	m_bConfigChanged = m_bConfigChanged || (bRet != bVal);

	return bRet;
}
bool CCopyThread::GetAsyncCopy()
{
	ATL::CCritSecLock csLock(m_cs.m_sect);

	bool bRet = m_SharedConfig.m_bAsyncCopy;

	return bRet;
}

void CCopyThread::Init(CCopyConfig cfg)
{
	ASSERT(!m_LocalConfig.m_pSupportedTypes);
	m_SharedConfig.CopySettingsFrom(cfg);
	m_SharedConfig.m_pSupportedTypes.reset();
	m_LocalConfig.CopySettingsFrom(cfg);
	// let m_LocalConfig own the m_pSupportedTypes
	m_LocalConfig.m_pSupportedTypes = std::move(cfg.m_pSupportedTypes);
}

bool CCopyThread::Quit()
{
	m_bQuit = true;
	m_pClipboardViewer->PostMessage( WM_QUIT );
	return CWinThread::PostThreadMessage( WM_QUIT, NULL, NULL ) != FALSE;
}
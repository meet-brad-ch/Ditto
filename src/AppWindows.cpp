#include "stdafx.h"
#include "CP_Main.h" // the frame and quick paste window types in the order they need
#include "AppWindows.h"
#include "AppState.h"

CAppWindows::CAppWindows(CAppState& state) :
	m_state(state)
{
}

CMainFrame* CAppWindows::MainFrame() const
{
	return m_pMainFrame;
}

void CAppWindows::SetMainFrame(CMainFrame* frame)
{
	m_pMainFrame = frame;
}

HWND CAppWindows::MainHwnd() const
{
	return m_mainHwnd;
}

void CAppWindows::SetMainHwnd(HWND hWnd)
{
	m_mainHwnd = hWnd;
}

CQPasteWnd* CAppWindows::QPasteWnd() const
{
	CMainFrame* const pMainFrame{ m_pMainFrame.load() };
	if (pMainFrame != NULL)
	{
		return pMainFrame->m_quickPaste.m_pwndPaste.get();
	}

	return NULL;
}

HWND CAppWindows::QPastehWnd() const
{
	CMainFrame* const pMainFrame{ m_pMainFrame.load() };
	if (pMainFrame != NULL)
	{
		if (pMainFrame->m_quickPaste.m_pwndPaste != NULL)
		{
			return pMainFrame->m_quickPaste.m_pwndPaste->GetSafeHwnd();
		}
	}

	return NULL;
}

void CAppWindows::RefreshView(CopyReasonEnum::CopyReason copyReason)
{
	CQPasteWnd* pWnd = QPasteWnd();
	if (pWnd)
	{
		if (m_state.m_bAsynchronousRefreshView)
		{
			pWnd->PostMessage(CDittoMessage::RefreshView, copyReason, 0);
		}
		else
		{
			pWnd->SendMessage(CDittoMessage::RefreshView, copyReason, 0);
		}
	}
}

void CAppWindows::RefreshClipInUI(int clipId, int updateFlags)
{
	CQPasteWnd* pWnd = QPasteWnd();
	if (pWnd)
	{
		if (m_state.m_bAsynchronousRefreshView)
		{
			pWnd->PostMessage(CDittoMessage::ReloadClipInUi, clipId, updateFlags);
		}
		else
		{
			pWnd->SendMessage(CDittoMessage::ReloadClipInUi, clipId, updateFlags);
		}
	}
}

void CAppWindows::OnDeleteID(long lID)
{
	if (QPasteWnd())
	{
		QPasteWnd()->PostMessage(CQListCtrl::NmItemDeleted, lID, 0);
	}
}

void CAppWindows::SetStatus(const TCHAR* status, bool bRepaintImmediately)
{
	m_state.m_Status = status;
	if (QPasteWnd())
	{
		QPasteWnd()->UpdateStatus(bRepaintImmediately);
	}
}

void CAppWindows::RefreshShowInTaskBar()
{
	CMainFrame* const pMainFrame{ m_pMainFrame.load() };
	if (pMainFrame != NULL)
	{
		pMainFrame->RefreshShowInTaskBar();
	}
}

void CAppWindows::PumpMessages(HWND hWnd)
{
	MSG KeyboardMsg{};
	while (::PeekMessage(&KeyboardMsg, hWnd, 0, 0, PM_REMOVE))
	{
		::TranslateMessage(&KeyboardMsg);
		::DispatchMessage(&KeyboardMsg);
	}
}

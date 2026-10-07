// QuickPaste.cpp: implementation of the CQuickPaste class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CP_Main.h"
#include "QuickPaste.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQuickPaste::CQuickPaste()
{
	m_forceResizeOnNextShow = false;
}

CQuickPaste::~CQuickPaste()
{
	m_pwndPaste.reset();
}

BOOL CQuickPaste::CloseQPasteWnd()
{
	if(m_pwndPaste)
	{		
		if(m_pwndPaste)
		{
			m_pwndPaste->CloseWindow();
			m_pwndPaste->DestroyWindow();
		}

		CLogger::Log(_T("CloseQPasteWnd called closing qpastewnd"));

		m_pwndPaste.reset();

		theApp.m_bShowingQuickPaste = false;
	}
	
	return TRUE;
}

void CQuickPaste::ShowQPasteWnd(CWnd *pParent, bool bAtPrevPos, bool bFromKeyboard, BOOL bReFillList)
{		
	CLogger::Log(StrF(_T("Start of ShowQPasteWnd, AtPrevPos: %d, FromKeyboard: %d, RefillList: %d"), bAtPrevPos, bFromKeyboard, bReFillList));

	if(IsReopenDatabaseRequested(bFromKeyboard))
	{
		CloseWndAndReopenDatabase();

		return;
	}

	if(ShowPersistentWnd())
	{
		return;
	}

	int nPosition = CGetSetOptions::GetQuickPastePosition();

	CPoint point;
	CSize csSize;

	if(!m_pwndPaste)
		m_pwndPaste = std::make_unique<CQPasteWnd>();

	m_pwndPaste->MinMaxWindow(CDittoWindow::ForceMax);

	GetInitialPointAndSize(point, csSize);

	CPoint ptCaret = CaretOrCenterPoint(csSize, point);

	ChooseWindowPoint(nPosition, bAtPrevPos, ptCaret, point, csSize);

	CRect crRect = CRect(point, csSize);

	bool forceMoveWindow = FixInitialRect(crRect, ptCaret);

	bool adjustRect = CreateWndIfNeeded(pParent, crRect);

	bool moveWindow = ShouldMoveWindow(nPosition, bAtPrevPos, forceMoveWindow);

	//If minimized
	if (m_pwndPaste->IsIconic())
	{
		m_pwndPaste->ShowWindow(SW_RESTORE);

		if (moveWindow)
		{
			MoveQPasteWnd(crRect, adjustRect);
		}
	}
	else
	{
		if (moveWindow)
		{
			MoveQPasteWnd(crRect, adjustRect);
		}


		// Show the window
		m_pwndPaste->ShowWindow(SW_SHOW);
	}

	m_pwndPaste->SetKeyModiferState(bFromKeyboard);

	if(bReFillList)
	{
		m_pwndPaste->ShowQPasteWindow(bReFillList);
	}
	
	// Refresh scrollbar colors to match current theme
	m_pwndPaste->RefreshScrollBarColors();
	
	m_pwndPaste->SetForegroundWindow();

	CLogger::Log(StrF(_T("END of ShowQPasteWnd, AtPrevPos: %d, FromKeyboard: %d, RefillList: %d, Position, %d %d %d %d"), bAtPrevPos, bFromKeyboard, bReFillList, crRect.left, crRect.top, crRect.right, crRect.bottom));

	m_forceResizeOnNextShow = false;
}

bool CQuickPaste::IsReopenDatabaseRequested(bool bFromKeyboard)
{
	return (bFromKeyboard == false && GetKeyState(VK_SHIFT) & 0x8000 && CKeyboard::IsControlPressed());
}

void CQuickPaste::CloseWndAndReopenDatabase()
{
	if(m_pwndPaste)
	{
		m_pwndPaste->CloseWindow();
		m_pwndPaste->DestroyWindow();
	}

	CLogger::Log(_T("CloseQPasteWnd called closing qpastewnd from keyboard"));

	m_pwndPaste.reset();

	theApp.m_db.close();
	OpenDatabase(CGetSetOptions::GetDBPath());
}

bool CQuickPaste::ShowPersistentWnd()
{
	if(CGetSetOptions::m_bShowPersistent && m_pwndPaste != nullptr)
	{
		m_pwndPaste->ShowWindow(SW_SHOW);
		m_pwndPaste->MinMaxWindow(CDittoWindow::ForceMax);
		m_pwndPaste->SetForegroundWindow();
		return true;
	}

	return false;
}

void CQuickPaste::GetInitialPointAndSize(CPoint &point, CSize &csSize)
{
	CRect rcPrev;

	//If it is a window get the rect otherwise get the saved point and size
	if (IsWindow(m_pwndPaste->m_hWnd) &&
		m_pwndPaste->IsIconic() == FALSE &&
		m_forceResizeOnNextShow == false)
	{
		m_pwndPaste->GetWindowRect(rcPrev);
		csSize = rcPrev.Size();
	}
	else
	{
		CGetSetOptions::GetQuickPastePoint(point);
		CGetSetOptions::GetQuickPasteSize(csSize);

		if (IsWindow(m_pwndPaste->m_hWnd))
		{
			csSize.cx = m_pwndPaste->m_DittoWindow.m_dpi.Scale(csSize.cx);
			csSize.cy = m_pwndPaste->m_DittoWindow.m_dpi.Scale(csSize.cy);
		}
	}
}

CPoint CQuickPaste::CaretOrCenterPoint(const CSize &csSize, CPoint &point)
{
	CPoint ptCaret = theApp.m_activeWnd.FocusCaret();
	if(ptCaret.x == -1 || ptCaret.y == -1)
	{
		CRect cr;
		::GetWindowRect(theApp.m_activeWnd.ActiveWnd(), cr);

		if(theApp.m_activeWnd.DesktopHasFocus() == false &&
			cr.Width() > 0 &&
			cr.Height() > 0)
		{
			ptCaret = cr.CenterPoint();
			ptCaret.x -= csSize.cx/2;
			ptCaret.y -= csSize.cy/2;
		}
		else
		{
			GetCursorPos(&point);

			CRect crPoint(point, CSize(1, 1));

			CRect crMonitor = MonitorRectFromRect(crPoint);

			ptCaret = crMonitor.CenterPoint();
			ptCaret.x -= csSize.cx/2;
			ptCaret.y -= csSize.cy/2;
		}
	}

	return ptCaret;
}

void CQuickPaste::ChooseWindowPoint(int nPosition, bool bAtPrevPos, const CPoint &ptCaret, CPoint &point, CSize &csSize)
{
	if(bAtPrevPos)
	{
		CGetSetOptions::GetQuickPastePoint(point);
		CGetSetOptions::GetQuickPasteSize(csSize);
	}
	else if (nPosition == CGetSetOptions::PosAtCaret)
	{
		point = ptCaret;
		if (CGetSetOptions::m_centerWindowBelowCursorOrCaret)
		{
			point.x -= csSize.cx / 2;
		}
	}
	else if (nPosition == CGetSetOptions::PosAtCursor)
	{
		GetCursorPos(&point);
		//keep the mouse from showing the tooltip because if overlaps with the top corner
		point.x += 2;
		point.y += 2;

		if (CGetSetOptions::m_centerWindowBelowCursorOrCaret)
		{
			point.x -= csSize.cx / 2;
		}
	}
	else if(nPosition == CGetSetOptions::PosAtPrevious)
		CGetSetOptions::GetQuickPastePoint(point);
}

bool CQuickPaste::FixInitialRect(CRect &crRect, const CPoint &ptCaret)
{
	bool forceMoveWindow = m_forceResizeOnNextShow;

	if(CGetSetOptions::m_bEnsureEntireWindowCanBeSeen)
	{
		if(EnsureWindowVisible(&crRect))
		{
			forceMoveWindow = true;
		}
	}

	if((crRect.left >= (crRect.right - 20)) ||
		(crRect.top >= (crRect.bottom - 20)))
	{
		CRect orig = crRect;
		crRect = CRect(ptCaret, CSize(300, 300));
		forceMoveWindow = true;

		CLogger::Log(StrF(_T("Invalid initial size %d %d %d %d, Centered Window %d %d %d %d"), orig.left, orig.top, orig.right, orig.bottom, crRect.left, crRect.top, crRect.right, crRect.bottom));
	}

	return forceMoveWindow;
}

bool CQuickPaste::CreateWndIfNeeded(CWnd *pParent, const CRect &crRect)
{
	bool adjustRect = false;

	if( !IsWindow(m_pwndPaste->m_hWnd) )
	{
		CWnd *pLocalParent = pParent;

		if(CGetSetOptions::GetShowInTaskBar())
		{
			pLocalParent = NULL;
		}

		VERIFY( m_pwndPaste->Create(crRect, pLocalParent) );

		adjustRect = true;
	}

	return adjustRect;
}

bool CQuickPaste::ShouldMoveWindow(int nPosition, bool bAtPrevPos, bool forceMoveWindow)
{
	return ((nPosition == CGetSetOptions::PosAtCaret) ||
		(nPosition == CGetSetOptions::PosAtCursor) ||
		bAtPrevPos ||
		forceMoveWindow);
}

void CQuickPaste::MoveQPasteWnd(CRect &crRect, bool adjustRect)
{
	if (adjustRect)
	{
		crRect.right = crRect.left + m_pwndPaste->m_DittoWindow.m_dpi.Scale(crRect.Width());
		crRect.bottom = crRect.top + m_pwndPaste->m_DittoWindow.m_dpi.Scale(crRect.Height());

		if (CGetSetOptions::m_bEnsureEntireWindowCanBeSeen)
		{
			EnsureWindowVisible(&crRect);
		}
	}

	m_pwndPaste->MoveWindow(crRect);
}

void CQuickPaste::MoveSelection(bool down)
{
	if(m_pwndPaste && CGetSetOptions::m_moveSelectionOnOpenHotkey)
	{
		if (IsWindow(m_pwndPaste->m_hWnd))
		{
			m_pwndPaste->MoveSelection(down, true);
		}
	}
}

void CQuickPaste::OnKeyStateUp()
{
	if(m_pwndPaste && CGetSetOptions::m_moveSelectionOnOpenHotkey)
	{
		if (IsWindow(m_pwndPaste->m_hWnd))
		{
			m_pwndPaste->OnKeyStateUp();
		}
	}
}

void CQuickPaste::SetKeyModiferState(bool bActive)
{
	if(m_pwndPaste && CGetSetOptions::m_moveSelectionOnOpenHotkey)
	{
		if (IsWindow(m_pwndPaste->m_hWnd))
		{
			m_pwndPaste->SetKeyModiferState(bActive);
		}
	}
}

void CQuickPaste::HideQPasteWnd()
{
	// Hide the window
	if(m_pwndPaste)
	{
		if (IsWindow(m_pwndPaste->m_hWnd))
			m_pwndPaste->HideQPasteWindow(true);
	}
}

BOOL CQuickPaste::IsWindowVisibleEx()
{
	if(m_pwndPaste)
		return IsWindowVisible(m_pwndPaste->m_hWnd);

	return FALSE;
}

bool CQuickPaste::IsWindowTopLevel()
{
	if(m_pwndPaste)
	{
		return ::GetForegroundWindow() == m_pwndPaste->GetSafeHwnd();
	}

	return false;
}

void CQuickPaste::OnScreenResolutionChange()
{
	if(m_pwndPaste != nullptr &&
		::IsWindow(m_pwndPaste->m_hWnd) &&
		m_pwndPaste->IsIconic() == FALSE &&
		IsWindowVisibleEx())
	{
		CLogger::Log(StrF(_T("Window Position changed, moving window to position as of this screen resolution %dx%d"), GetScreenWidth(), GetScreenHeight()));
		CPoint point;
		CSize csSize;

		CGetSetOptions::GetQuickPastePoint(point);
		CGetSetOptions::GetQuickPasteSize(csSize);

		csSize.cx = m_pwndPaste->m_DittoWindow.m_dpi.Scale(csSize.cx);
		csSize.cy = m_pwndPaste->m_DittoWindow.m_dpi.Scale(csSize.cy);

		m_pwndPaste->MoveWindow(point.x, point.y, csSize.cx, csSize.cy);
	}
	else
	{
		m_forceResizeOnNextShow = true;
	}
}
// EditWnd.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "EditWnd.h"
#include ".\editwnd.h"
#include "SaveAnimation.h"
#include "ProcessPaste.h"
#include "ErrorReport.h"
#include <memory>

IMPLEMENT_DYNAMIC(CEditWnd, CWnd)
CEditWnd::CEditWnd()
{
	m_lastSaveID = -1;
}

CEditWnd::~CEditWnd()
{
}

BEGIN_MESSAGE_MAP(CEditWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_SETFOCUS()
	ON_COMMAND(ID_BUTTON_SAVE, OnSave)
	ON_COMMAND(ID_BUTTON_SAVE_ALL, OnSaveAll)
	ON_COMMAND(ID_BUTTON_CLOSE, OnClose)
	ON_COMMAND(ID_BUTTON_NEW, OnNew)
	ON_COMMAND(ID_BUTTON_SAVE_CLOSE_CLIPBOARD, OnSaveCloseClipboard)
	ON_WM_DESTROY()
END_MESSAGE_MAP()

int CEditWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CWnd::OnCreate(lpCreateStruct) == -1)
		return -1;

	m_dpi.SetHwnd(m_hWnd);

	m_toolBarControl.CreateEx(this, TBSTYLE_FLAT, WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS);
	
	LoadToolbarDPI();

	m_toolBarControl.EnableWindow();
	
	m_tabControl.Create(WS_CHILD|WS_VISIBLE|WS_TABSTOP|CTabCtrlEx::ScsTop, CRect(0, 0, 0, 0), this, 101);
	
	//m_font.CreatePointFont(m_dpi.Scale(90), _T("Arial Unicode MS"), this->GetDC());
	m_font.CreateFont(-m_dpi.Scale(13), 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 3, 2, 1, 34, _T("Segoe UI"));
	m_updateDescriptionButton.Create(theApp.m_Language.GetString("Update_Desc", "Update clip description on save?"), WS_CHILD|WS_VISIBLE|BS_AUTOCHECKBOX, CRect(0,0,0,0), this, 101);
	m_updateDescriptionButton.SetFont(&m_font);

	if(theApp.Services().Settings().GetUpdateDescWhenSavingClip())
	{
		m_updateDescriptionButton.SetCheck(BST_CHECKED);
	}

	MoveControls();

	m_toolTipControl.Create(this);
	CRect cr;
	CString csText;

	m_toolBarControl.GetItemRect(m_toolBarControl.CommandToIndex(ID_BUTTON_NEW), cr);
	csText.Format(_T("%s     Ctrl - N"), theApp.m_Language.GetString("New_Clip", "New Clip").GetString());
	m_toolTipControl.AddTool(&m_toolBarControl, csText, cr, 1);

	m_toolBarControl.GetItemRect(m_toolBarControl.CommandToIndex(ID_BUTTON_SAVE), cr);
	csText.Format(_T("%s    Ctrl - S"), theApp.m_Language.GetString("Save", "Save").GetString());
	m_toolTipControl.AddTool(&m_toolBarControl, csText, cr, 2);

	m_toolBarControl.GetItemRect(m_toolBarControl.CommandToIndex(ID_BUTTON_SAVE_ALL), cr);
	csText.Format(_T("%s    Ctrl - Shift - S"), theApp.m_Language.GetString("Save_All", "Save All").GetString());
	m_toolTipControl.AddTool(&m_toolBarControl, csText, cr, 3);

	m_toolBarControl.GetItemRect(m_toolBarControl.CommandToIndex(ID_BUTTON_CLOSE), cr);
	csText.Format(_T("%s    Escape"), theApp.m_Language.GetString("Close", "Close Current Tab").GetString());
	m_toolTipControl.AddTool(&m_toolBarControl, csText, cr, 4);

	m_toolBarControl.GetItemRect(m_toolBarControl.CommandToIndex(ID_BUTTON_SAVE_CLOSE_CLIPBOARD), cr);
	csText.Format(_T("%s    Shift - Escape"), theApp.m_Language.GetString("Save_Close", "Save, Close and place on clipboard").GetString());
	m_toolTipControl.AddTool(&m_toolBarControl, csText, cr, 4);

	return 0;
}

void CEditWnd::LoadToolbarDPI()
{
	int scale = m_dpi.Scale(100);

	if (scale >= 225)
	{
		m_toolBarControl.LoadToolBar(IDR_EDIT_WND_225);
	}
	else if (scale >= 200)
	{
		m_toolBarControl.LoadToolBar(IDR_EDIT_WND_200);
	}
	else if (scale >= 175)
	{
		m_toolBarControl.LoadToolBar(IDR_EDIT_WND_175);
	}
	else if (scale >= 150)
	{
		m_toolBarControl.LoadToolBar(IDR_EDIT_WND_150);
	}
	else if (scale >= 125)
	{
		m_toolBarControl.LoadToolBar(IDR_EDIT_WND_125);
	}	
	else
	{
		m_toolBarControl.LoadToolBar(IDR_EDIT_WND);
	}
}

void CEditWnd::OnDpiChanged(CWnd* pParent, int dpi)
{
	m_dpi.Update(dpi);

	m_font.DeleteObject();
	m_font.CreateFont(-m_dpi.Scale(13), 0, 0, 0, 400, 0, 0, 0, DEFAULT_CHARSET, 3, 2, 1, 34, _T("Segoe UI"));
	
	m_updateDescriptionButton.SetFont(&m_font);
	
	m_tabControl.OnDpiChanged(pParent, dpi);

	LoadToolbarDPI();

	for (const std::unique_ptr<CDittoRulerRichEditCtrl>& pEdit : m_edits)
	{
		if (pEdit)
		{
			pEdit->OnDpiChanged(pParent, dpi);
		}
	}

	MoveControls();
}

void CEditWnd::OnSize(UINT nType, int cx, int cy)
{
	CWnd::OnSize(nType, cx, cy);

	if(::IsWindow(m_toolBarControl.GetSafeHwnd()))
	{
		MoveControls();
	}
}

void CEditWnd::MoveControls()
{
	int toolbarHeight = m_dpi.Scale(23);
	int descriptionHeight = m_dpi.Scale(20);

	CRect rc;
	GetClientRect(rc);

	CRect toolbarRect(0, 0, rc.Width(), toolbarHeight);
	m_toolBarControl.MoveWindow(toolbarRect);

	m_tabControl.MoveWindow(0, toolbarHeight, rc.Width(), rc.Height() - toolbarHeight - descriptionHeight);

	m_updateDescriptionButton.MoveWindow(2, rc.Height()- descriptionHeight, rc.Width()-m_dpi.Scale(2), descriptionHeight);
}

void CEditWnd::OnSaveAll() 
{
	INT_PTR size = m_edits.size();
	for(int tab = 0; tab < size; tab++)
	{
		DoSaveItem(tab);
	}
}

void CEditWnd::OnSave() 
{
	DoSave();
}

bool CEditWnd::DoSave()
{
	bool bRet = false;
	int nTab = m_tabControl.GetActiveTab();
	if(nTab >= 0 && nTab < (int)m_edits.size())
	{
		bRet = DoSaveItem(nTab);
	}

	return bRet;
}

bool CEditWnd::DoSaveItem(int index)
{
	bool bRet = false;
	BOOL bUpdateDesc = m_updateDescriptionButton.GetCheck();

	CDittoRulerRichEditCtrl *pEdit = m_edits[index].get();
	if(pEdit)
	{
		int nRet = pEdit->SaveToDB(bUpdateDesc);
		if(nRet > 0)
		{
			if(bUpdateDesc)
			{
				m_tabControl.SetTabTitle(index, pEdit->GetDesc());
			}

			if(nRet == CDittoRulerRichEditCtrl::SavedClipToDb)
			{
				CSaveAnimation Ani;
				CRect cr;
				pEdit->GetWindowRect(cr);
				CRect crSave;
				m_toolBarControl.GetItemRect(2, crSave);
				Ani.DoAnimation(cr, crSave, this);
			}

			m_lastSaveID = pEdit->GetDBID();
			bRet = true;
		}
		else
		{
			CString cs;
			cs.Format(_T("%s '%s'"), theApp.m_Language.GetString("ErrorSaving", "Error saving clip").GetString(), m_tabControl.GetTabTitle(index).GetString());
			MessageBox(cs, _T("Ditto"), MB_OK);
		}
	}

	return bRet;
}

bool CEditWnd::EditIds(CClipIDs &Ids)
{
	INT_PTR count = min(Ids.GetSize(), 10);
	for(int i = 0; i < count; i++)
	{
		if(IsIDAlreadyInEdit(Ids[i], true) < 0)
		{
			AddItem(Ids[i]);
		}
	}

	MoveControls();

	return true;
}

bool CEditWnd::AddItem(int id)
{
	bool bRet = false;
	// owned here until m_edits takes it, so a failed load does not leak it
	std::unique_ptr<CDittoRulerRichEditCtrl> pEdit{std::make_unique<CDittoRulerRichEditCtrl>()};
	if(pEdit)
	{
		CString csTitle;

		if(id >= 0)
		{
			try
			{
				CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT mText FROM Main where lID = %d"), id);
				if(q.eof() == false)
				{
					csTitle = q.getStringField(_T("mText"));
					csTitle = csTitle.Left(15);
				}
			}
			catch (CppSQLite3Exception& e)
			{
				CErrorReport::Show(CStringUtil::Format(_T("Opening clip id %d for editing failed: %s"), id, e.errorMessage()));
				return false;
			}
		}
		else
		{
			csTitle = theApp.m_Language.GetString("New", "New");
		}

		pEdit->Create(WS_TABSTOP|WS_CHILD|WS_VISIBLE, CRect(100, 100, 105, 105), this, 100, TRUE);
		pEdit->ShowToolbar();
		pEdit->LoadItem(id, csTitle);		

		m_tabControl.AddItem(csTitle, pEdit.get());
		int nTab = m_tabControl.GetTabCount();
		m_tabControl.SetActiveTab(nTab-1);

		m_edits.push_back(std::move(pEdit));
		bRet = true;
	}

	return bRet;

}

int CEditWnd::IsIDAlreadyInEdit(int id, bool bSetFocus)
{
	INT_PTR size = m_edits.size();
	for(int i = 0; i < size; i++)
	{
		CDittoRulerRichEditCtrl *pEdit = m_edits[i].get();
		if(pEdit)
		{
			if(pEdit->GetDBID() == id)
			{
				if(bSetFocus)
				{
					m_tabControl.SetActiveTab(i);
				}
				return (int)i;
			}
		}
	}
	return -1;
}

void CEditWnd::OnDestroy()
{
	CWnd::OnDestroy();

	INT_PTR size = m_edits.size();
	for(int i = 0; i < size; i++)
	{
		CDittoRulerRichEditCtrl *pEdit = m_edits[i].get();
		if(pEdit)
		{
			pEdit->DestroyWindow();

			m_edits[i].reset();
		}
	}

	m_edits.erase(m_edits.begin(), m_edits.end());

	theApp.Services().Settings().SetUpdateDescWhenSavingClip(m_updateDescriptionButton.GetCheck());
}

void CEditWnd::OnSetFocus(CWnd* pOldWnd)
{
	CWnd::OnSetFocus(pOldWnd);

	int nTab = m_tabControl.GetActiveTab();
	if(nTab >= 0 && nTab < (int)m_edits.size())
	{
		CDittoRulerRichEditCtrl *pEdit = m_edits[nTab].get();
		if(pEdit)
		{
			pEdit->SetFocus();
		}
	}
}

bool CEditWnd::CloseEdits(bool bPrompt)
{
	BOOL bUpdateDesc = m_updateDescriptionButton.GetCheck();

	int nTab = 0;
	for(std::vector<std::unique_ptr<CDittoRulerRichEditCtrl>>::iterator it = m_edits.begin(); it != m_edits.end();)
	{
		CDittoRulerRichEditCtrl *pEdit = it->get();
		if(pEdit)
		{
			if(pEdit->CloseEdit(bPrompt, bUpdateDesc) == false)
				return false;

			m_tabControl.DeleteItem(nTab);
			pEdit->DestroyWindow();

			// erasing the entry deletes the control
			it = m_edits.erase(it);
		}
		else
		{
			it++;
		}
		nTab++;
	}

	return true;
}

void CEditWnd::OnClose()
{	
	BOOL bUpdateDesc = m_updateDescriptionButton.GetCheck();
	int activeTab = m_tabControl.GetActiveTab();
	if(activeTab >= 0 && activeTab < (int)m_edits.size())
	{
		CDittoRulerRichEditCtrl *pEdit = m_edits[activeTab].get();
		if(pEdit)
		{
			if(pEdit->CloseEdit(true, bUpdateDesc))
			{
				m_tabControl.DeleteItem(activeTab);
				pEdit->DestroyWindow();
				// erasing the entry deletes the control
				m_edits.erase(m_edits.begin()+ activeTab);

				if(m_edits.size() <= 0)
				{
					CWnd *pParent = GetParent();
					if(pParent)
					{
						pParent->SendMessage(WM_CLOSE, 0, 0);
					}
				}
			}
		}
	}
}

void CEditWnd::OnNew()
{
	AddItem(-1);
}

void CEditWnd::OnSaveCloseClipboard()
{
	if(DoSave())
	{
		if(m_lastSaveID >= 0)
		{
			CProcessPaste Paste(theApp.Services().Settings());
			Paste.GetClipIDs().Add(m_lastSaveID);
			Paste.m_bSendPaste = false;
			Paste.DoPaste();
		}

		OnClose();
	}
}

BOOL CEditWnd::PreTranslateMessage(MSG* pMsg)
{
	m_toolTipControl.RelayEvent(pMsg);

	if(pMsg->message == WM_KEYDOWN)
	{
		if(HandleKeyDown(pMsg->wParam))
		{
			return TRUE;
		}
	}
	else if(pMsg->message == WM_SYSKEYDOWN) // ALT key is held down
	{
		switch( pMsg->wParam )
		{
			case VK_F4: 
				OnClose();
				return TRUE;
		}
	}

	return CWnd::PreTranslateMessage(pMsg);
}

bool CEditWnd::HandleKeyDown(WPARAM key)
{
	if(key == VK_ESCAPE)
	{
		if(GetKeyState(VK_SHIFT) & 0x8000)
		{
			OnSaveCloseClipboard();
		}
		else
		{
			OnClose();
		}
		return true;
	}
	else if(key == 'S')
	{
		if(CKeyboard::IsControlPressed())
		{
			if(GetKeyState(VK_SHIFT) & 0x8000)
			{
				OnSaveAll();
				return true;
			}
			else
			{
				OnSave();
				return true;
			}
		}
	}
	else if(key == 'N')
	{
		if(CKeyboard::IsControlPressed())
		{
			OnNew();
		}
	}

	return false;
}

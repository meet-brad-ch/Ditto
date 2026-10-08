// OptionsKeyBoard.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "OptionsKeyBoard.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsKeyBoard property page

IMPLEMENT_DYNCREATE(COptionsKeyBoard, CPropertyPage)

COptionsKeyBoard::COptionsKeyBoard() : CPropertyPage(COptionsKeyBoard::IDD)
{
	m_csTitle = theApp.Services().Language().GetString("KeyboardShortcutsTitle", "Keyboard Shortcuts");
	m_psp.pszTitle = m_csTitle;
	m_psp.dwFlags |= PSP_USETITLE;
	
	//{{AFX_DATA_INIT(COptionsKeyBoard)
	//}}AFX_DATA_INIT
}

COptionsKeyBoard::~COptionsKeyBoard()
{
}

void COptionsKeyBoard::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsKeyBoard)
	DDX_Control(pDX, IDC_CHECK_SEND_PASTE, m_btSendPaste);
	DDX_Control(pDX, IDC_CHECK_USE_UI_GROUP_LAST_10, m_UseUiGroupForLastTen);
	DDX_Control(pDX, IDC_HOTKEY9, m_Nine);
	DDX_Control(pDX, IDC_HOTKEY8, m_Eight);
	DDX_Control(pDX, IDC_HOTKEY7, m_Seven);
	DDX_Control(pDX, IDC_HOTKEY6, m_Six);
	DDX_Control(pDX, IDC_HOTKEY5, m_Five);
	DDX_Control(pDX, IDC_HOTKEY4, m_Four);
	DDX_Control(pDX, IDC_HOTKEY3, m_Three);
	DDX_Control(pDX, IDC_HOTKEY2, m_Two);
	DDX_Control(pDX, IDC_HOTKEY10, m_Ten);
	DDX_Control(pDX, IDC_HOTKEY1, m_One);
	DDX_Control(pDX, IDC_HOTKEY, m_HotKey);
	DDX_Control(pDX, IDC_HOTKEY_ACTIVATE_2, m_HotKey2);
	DDX_Control(pDX, IDC_HOTKEY_ACTIVATE_3, m_HotKey3);
	DDX_Control(pDX, IDC_HOTKEY_TEXT_ONLY, m_TextOnlyKey);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CHECK_MOVE_CLIPS_ON_PASTE, m_btMoveClipOnGlobal10);
	DDX_Control(pDX, IDC_HOTKEY_SAVE_CLIPBOARD, m_saveClipboardHotKey);
	DDX_Control(pDX, IDC_HOTKEY_COPYSAVECLIPBOARD, m_copyAndSaveClipboardCtrl);
}

BEGIN_MESSAGE_MAP(COptionsKeyBoard, CPropertyPage)
	//{{AFX_MSG_MAP(COptionsKeyBoard)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsKeyBoard message handlers

BOOL COptionsKeyBoard::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_pParent = (COptionsSheet *)GetParent();

	CHotKeys& hotKeys = theApp.Services().HotKeys();
	using Id = CHotKeys::Id;

	hotKeys.Named(Id::DittoHotKey)->CopyToCtrl(m_HotKey, m_hWnd, IDC_CHECK_WIN_DITTO);
	hotKeys.Named(Id::DittoHotKey2)->CopyToCtrl(m_HotKey2, m_hWnd, IDC_CHECK_WIN_DITTO2);
	hotKeys.Named(Id::DittoHotKey3)->CopyToCtrl(m_HotKey3, m_hWnd, IDC_CHECK_WIN_DITTO3);

	hotKeys.Named(Id::PosOne)->CopyToCtrl(m_One, m_hWnd, IDC_CHECK_WIN1);
	hotKeys.Named(Id::PosTwo)->CopyToCtrl(m_Two, m_hWnd, IDC_CHECK_WIN2);
	hotKeys.Named(Id::PosThree)->CopyToCtrl(m_Three, m_hWnd, IDC_CHECK_WIN3);
	hotKeys.Named(Id::PosFour)->CopyToCtrl(m_Four, m_hWnd, IDC_CHECK_WIN4);
	hotKeys.Named(Id::PosFive)->CopyToCtrl(m_Five, m_hWnd, IDC_CHECK_WIN5);
	hotKeys.Named(Id::PosSix)->CopyToCtrl(m_Six, m_hWnd, IDC_CHECK_WIN6);
	hotKeys.Named(Id::PosSeven)->CopyToCtrl(m_Seven, m_hWnd, IDC_CHECK_WIN7);
	hotKeys.Named(Id::PosEight)->CopyToCtrl(m_Eight, m_hWnd, IDC_CHECK_WIN8);
	hotKeys.Named(Id::PosNine)->CopyToCtrl(m_Nine, m_hWnd, IDC_CHECK_WIN9);
	hotKeys.Named(Id::PosTen)->CopyToCtrl(m_Ten, m_hWnd, IDC_CHECK_WIN10);
	hotKeys.Named(Id::TextOnlyPaste)->CopyToCtrl(m_TextOnlyKey, m_hWnd, IDC_CHECK_WIN_TEXT_ONLY);
	hotKeys.Named(Id::SaveClipboard)->CopyToCtrl(m_saveClipboardHotKey, m_hWnd, IDC_CHECK_WIN_SAVE_CLIPBOARD);
	hotKeys.Named(Id::CopyAndSaveClipboard)->CopyToCtrl(m_copyAndSaveClipboardCtrl, m_hWnd, IDC_CHECK_WIN_COPY_SAVE_CLIPBOARD);


	//Unregister hotkeys and Reregister them on cancel or ok
	hotKeys.UnregisterAll();

	CGetSetOptions& settings = theApp.Services().Settings();
	m_btSendPaste.SetCheck(settings.m_bSendPasteOnFirstTenHotKeys);
	m_UseUiGroupForLastTen.SetCheck(settings.GetUseUISelectedGroupForLastTenCopies());

	m_btMoveClipOnGlobal10.SetCheck(settings.GetMoveClipsOnGlobal10());

	m_HotKey.SetFocus();

	theApp.Services().Language().UpdateOptionShortcuts(this);
		
	return FALSE;
}

LRESULT COptionsKeyBoard::OnWizardNext() 
{
	return CPropertyPage::OnWizardNext();
}

BOOL COptionsKeyBoard::OnWizardFinish() 
{
	return CPropertyPage::OnWizardFinish();
}

BOOL COptionsKeyBoard::OnApply()
{
	CGetSetOptions& settings = theApp.Services().Settings();
	settings.SetSendPasteOnFirstTenHotKeys(m_btSendPaste.GetCheck());
	settings.SetMoveClipsOnGlobal10(m_btMoveClipOnGlobal10.GetCheck());
	settings.SetUseUISelectedGroupForLastTenCopies(m_UseUiGroupForLastTen.GetCheck());
					
	INT_PTR x,y;
	CString str;
	ARRAY keys;
	
	CHotKeys& hotKeys = theApp.Services().HotKeys();
	using Id = CHotKeys::Id;

	hotKeys.GetKeys( keys ); // save old keys just in case new ones are invalid

	hotKeys.Named(Id::DittoHotKey)->CopyFromCtrl(m_HotKey, m_hWnd, IDC_CHECK_WIN_DITTO);
	hotKeys.Named(Id::DittoHotKey2)->CopyFromCtrl(m_HotKey2, m_hWnd, IDC_CHECK_WIN_DITTO2);
	hotKeys.Named(Id::DittoHotKey3)->CopyFromCtrl(m_HotKey3, m_hWnd, IDC_CHECK_WIN_DITTO3);

	hotKeys.Named(Id::PosOne)->CopyFromCtrl(m_One, m_hWnd, IDC_CHECK_WIN1);
	hotKeys.Named(Id::PosTwo)->CopyFromCtrl(m_Two, m_hWnd, IDC_CHECK_WIN2);
	hotKeys.Named(Id::PosThree)->CopyFromCtrl(m_Three, m_hWnd, IDC_CHECK_WIN3);
	hotKeys.Named(Id::PosFour)->CopyFromCtrl(m_Four, m_hWnd, IDC_CHECK_WIN4);
	hotKeys.Named(Id::PosFive)->CopyFromCtrl(m_Five, m_hWnd, IDC_CHECK_WIN5);
	hotKeys.Named(Id::PosSix)->CopyFromCtrl(m_Six, m_hWnd, IDC_CHECK_WIN6);
	hotKeys.Named(Id::PosSeven)->CopyFromCtrl(m_Seven, m_hWnd, IDC_CHECK_WIN7);
	hotKeys.Named(Id::PosEight)->CopyFromCtrl(m_Eight, m_hWnd, IDC_CHECK_WIN8);
	hotKeys.Named(Id::PosNine)->CopyFromCtrl(m_Nine, m_hWnd, IDC_CHECK_WIN9);
	hotKeys.Named(Id::PosTen)->CopyFromCtrl(m_Ten, m_hWnd, IDC_CHECK_WIN10);
	hotKeys.Named(Id::TextOnlyPaste)->CopyFromCtrl(m_TextOnlyKey, m_hWnd, IDC_CHECK_WIN_TEXT_ONLY);
	hotKeys.Named(Id::SaveClipboard)->CopyFromCtrl(m_saveClipboardHotKey, m_hWnd, IDC_CHECK_WIN_SAVE_CLIPBOARD);
	hotKeys.Named(Id::CopyAndSaveClipboard)->CopyFromCtrl(m_copyAndSaveClipboardCtrl, m_hWnd, IDC_CHECK_WIN_COPY_SAVE_CLIPBOARD);

	ARRAY NewKeys;
	hotKeys.GetKeys(NewKeys);

	if(hotKeys.FindFirstConflict(NewKeys, &x, &y))
	{
		str =  hotKeys.ElementAt(x)->GetName();
		str += " and ";
		str += hotKeys.ElementAt(y)->GetName();
		str += " cannot be the same.";
		MessageBox(str);
		hotKeys.SetKeys(keys); // restore the original values
		return FALSE;
	}

	hotKeys.SaveAllKeys();
	hotKeys.RegisterAll(true);

	return CPropertyPage::OnApply();
}

void COptionsKeyBoard::OnCancel()
{
	theApp.Services().HotKeys().RegisterAll( true );
	CPropertyPage::OnCancel();
}

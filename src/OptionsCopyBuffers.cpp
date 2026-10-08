// OptionsCopyBuffers.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "OptionsCopyBuffers.h"


// COptionsCopyBuffers dialog

IMPLEMENT_DYNCREATE(COptionsCopyBuffers, CPropertyPage)

COptionsCopyBuffers::COptionsCopyBuffers() :
	CPropertyPage(COptionsCopyBuffers::IDD)
{
	m_csTitle = theApp.Services().Language().GetString("CopyBuffers", "Copy Buffers");
	m_psp.pszTitle = m_csTitle;
	m_psp.dwFlags |= PSP_USETITLE;
}

COptionsCopyBuffers::~COptionsCopyBuffers()
{
}

void COptionsCopyBuffers::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_COPY_1, m_CopyBuffer1);
	DDX_Control(pDX, IDC_PASTE_1, m_PasteBuffer1);
	DDX_Control(pDX, IDC_CUT_1, m_CutBuffer1);
	DDX_Control(pDX, IDC_COPY_2, m_CopyBuffer2);
	DDX_Control(pDX, IDC_PASTE_2, m_PasteBuffer2);
	DDX_Control(pDX, IDC_CUT_2, m_CutBuffer2);
	DDX_Control(pDX, IDC_COPY_3, m_CopyBuffer3);
	DDX_Control(pDX, IDC_PASTE_3, m_PasteBuffer3);
	DDX_Control(pDX, IDC_CUT_3, m_CutBuffer3);

	DDX_Control(pDX, IDC_COPY_4, m_CopyBuffer4);
	DDX_Control(pDX, IDC_PASTE_4, m_PasteBuffer4);
	DDX_Control(pDX, IDC_CUT_4, m_CutBuffer4);

	DDX_Control(pDX, IDC_COPY_5, m_CopyBuffer5);
	DDX_Control(pDX, IDC_PASTE_5, m_PasteBuffer5);
	DDX_Control(pDX, IDC_CUT_5, m_CutBuffer5);
}


BEGIN_MESSAGE_MAP(COptionsCopyBuffers, CPropertyPage)
END_MESSAGE_MAP()

BOOL COptionsCopyBuffers::OnInitDialog()
{
	CPropertyPage::OnInitDialog();

	CGetSetOptions& settings = theApp.Services().Settings();
	CHotKeys& hotKeys = theApp.Services().HotKeys();
	using Id = CHotKeys::Id;
	CCopyBufferItem Item;

	settings.GetCopyBufferItem(0, Item);
	hotKeys.Named(Id::CopyBuffer1)->CopyToCtrl(m_CopyBuffer1, m_hWnd, IDC_WIN_COPY_1);
	hotKeys.Named(Id::PasteBuffer1)->CopyToCtrl(m_PasteBuffer1, m_hWnd, IDC_WIN_PASTE_1);
	hotKeys.Named(Id::CutBuffer1)->CopyToCtrl(m_CutBuffer1, m_hWnd, IDC_WIN_CUT_1);
	CheckDlgButton(IDC_PLAY_SOUND_1, Item.m_bPlaySoundOnCopy);

	settings.GetCopyBufferItem(1, Item);
	hotKeys.Named(Id::CopyBuffer2)->CopyToCtrl(m_CopyBuffer2, m_hWnd, IDC_WIN_COPY_2);
	hotKeys.Named(Id::PasteBuffer2)->CopyToCtrl(m_PasteBuffer2, m_hWnd, IDC_WIN_PASTE_2);
	hotKeys.Named(Id::CutBuffer2)->CopyToCtrl(m_CutBuffer2, m_hWnd, IDC_WIN_CUT_2);
	CheckDlgButton(IDC_PLAY_SOUND_2, Item.m_bPlaySoundOnCopy);

	settings.GetCopyBufferItem(2, Item);
	hotKeys.Named(Id::CopyBuffer3)->CopyToCtrl(m_CopyBuffer3, m_hWnd, IDC_WIN_COPY_3);
	hotKeys.Named(Id::PasteBuffer3)->CopyToCtrl(m_PasteBuffer3, m_hWnd, IDC_WIN_PASTE_3);
	hotKeys.Named(Id::CutBuffer3)->CopyToCtrl(m_CutBuffer3, m_hWnd, IDC_WIN_CUT_3);
	CheckDlgButton(IDC_PLAY_SOUND_3, Item.m_bPlaySoundOnCopy);

	settings.GetCopyBufferItem(3, Item);
	hotKeys.Named(Id::CopyBuffer4)->CopyToCtrl(m_CopyBuffer4, m_hWnd, IDC_WIN_COPY_4);
	hotKeys.Named(Id::PasteBuffer4)->CopyToCtrl(m_PasteBuffer4, m_hWnd, IDC_WIN_PASTE_4);
	hotKeys.Named(Id::CutBuffer4)->CopyToCtrl(m_CutBuffer4, m_hWnd, IDC_WIN_CUT_4);
	CheckDlgButton(IDC_PLAY_SOUND_4, Item.m_bPlaySoundOnCopy);

	settings.GetCopyBufferItem(4, Item);
	hotKeys.Named(Id::CopyBuffer5)->CopyToCtrl(m_CopyBuffer5, m_hWnd, IDC_WIN_COPY_5);
	hotKeys.Named(Id::PasteBuffer5)->CopyToCtrl(m_PasteBuffer5, m_hWnd, IDC_WIN_PASTE_5);
	hotKeys.Named(Id::CutBuffer5)->CopyToCtrl(m_CutBuffer5, m_hWnd, IDC_WIN_CUT_5);
	CheckDlgButton(IDC_PLAY_SOUND_5, Item.m_bPlaySoundOnCopy);

	theApp.Services().Language().UpdateOptionCopyBuffers(this);

	return TRUE;
}

BOOL COptionsCopyBuffers::OnApply()
{
	CHotKeys& hotKeys = theApp.Services().HotKeys();
	using Id = CHotKeys::Id;

	ARRAY keys;
	hotKeys.GetKeys(keys); // save old keys just in case new ones are invalid

	hotKeys.Named(Id::CopyBuffer1)->CopyFromCtrl(m_CopyBuffer1, m_hWnd, IDC_WIN_COPY_1);
	hotKeys.Named(Id::PasteBuffer1)->CopyFromCtrl(m_PasteBuffer1, m_hWnd, IDC_WIN_PASTE_1);
	hotKeys.Named(Id::CutBuffer1)->CopyFromCtrl(m_CutBuffer1, m_hWnd, IDC_WIN_CUT_1);

	hotKeys.Named(Id::CopyBuffer2)->CopyFromCtrl(m_CopyBuffer2, m_hWnd, IDC_WIN_COPY_2);
	hotKeys.Named(Id::PasteBuffer2)->CopyFromCtrl(m_PasteBuffer2, m_hWnd, IDC_WIN_PASTE_2);
	hotKeys.Named(Id::CutBuffer2)->CopyFromCtrl(m_CutBuffer2, m_hWnd, IDC_WIN_CUT_2);

	hotKeys.Named(Id::CopyBuffer3)->CopyFromCtrl(m_CopyBuffer3, m_hWnd, IDC_WIN_COPY_3);
	hotKeys.Named(Id::PasteBuffer3)->CopyFromCtrl(m_PasteBuffer3, m_hWnd, IDC_WIN_PASTE_3);
	hotKeys.Named(Id::CutBuffer3)->CopyFromCtrl(m_CutBuffer3, m_hWnd, IDC_WIN_CUT_3);

	hotKeys.Named(Id::CopyBuffer4)->CopyFromCtrl(m_CopyBuffer4, m_hWnd, IDC_WIN_COPY_4);
	hotKeys.Named(Id::PasteBuffer4)->CopyFromCtrl(m_PasteBuffer4, m_hWnd, IDC_WIN_PASTE_4);
	hotKeys.Named(Id::CutBuffer4)->CopyFromCtrl(m_CutBuffer4, m_hWnd, IDC_WIN_CUT_4);

	hotKeys.Named(Id::CopyBuffer5)->CopyFromCtrl(m_CopyBuffer5, m_hWnd, IDC_WIN_COPY_5);
	hotKeys.Named(Id::PasteBuffer5)->CopyFromCtrl(m_PasteBuffer5, m_hWnd, IDC_WIN_PASTE_5);
	hotKeys.Named(Id::CutBuffer5)->CopyFromCtrl(m_CutBuffer5, m_hWnd, IDC_WIN_CUT_5);

	CGetSetOptions& settings = theApp.Services().Settings();
	CCopyBufferItem Item;
	settings.GetCopyBufferItem(0, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_1);
	settings.SetCopyBufferItem(0, Item);

	settings.GetCopyBufferItem(1, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_2);
	settings.SetCopyBufferItem(1, Item);

	settings.GetCopyBufferItem(2, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_3);
	settings.SetCopyBufferItem(2, Item);

	settings.GetCopyBufferItem(3, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_4);
	settings.SetCopyBufferItem(3, Item);

	settings.GetCopyBufferItem(4, Item);
	Item.m_bPlaySoundOnCopy = IsDlgButtonChecked(IDC_PLAY_SOUND_5);
	settings.SetCopyBufferItem(4, Item);

	INT_PTR x;
	INT_PTR y;
	ARRAY NewKeys;
	hotKeys.GetKeys(NewKeys);

	if (hotKeys.FindFirstConflict(NewKeys, &x, &y))
	{
		CString str = hotKeys.ElementAt(x)->GetName();
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

// OptionsQuickPaste.cpp : implementation file
//

#include "stdafx.h"
#include "FileDialogPath.h"
#include "CP_Main.h"
#include "OptionsQuickPaste.h"
#include ".\optionsquickpaste.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsQuickPaste property page

IMPLEMENT_DYNCREATE(COptionsQuickPaste, CPropertyPage)

COptionsQuickPaste::COptionsQuickPaste() : CPropertyPage(COptionsQuickPaste::IDD)
{
	m_csTitle = theApp.m_Language.GetString("QuickPasteTitle", "Quick Paste");
	m_psp.pszTitle = m_csTitle;
	m_psp.dwFlags |= PSP_USETITLE; 

	//{{AFX_DATA_INIT(COptionsQuickPaste)
		// NOTE: the ClassWizard will add member initialization here
	//}}AFX_DATA_INIT

	memset(&m_LogFont, 0, sizeof(LOGFONT));
}

COptionsQuickPaste::~COptionsQuickPaste()
{
	m_Font.DeleteObject();
}

CGetSetOptions& COptionsQuickPaste::Settings() const
{
	return theApp.Services().Settings();
}

void COptionsQuickPaste::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsQuickPaste)
	DDX_Control(pDX, IDC_CHECK_ENTIRE_WINDOW_IS_VISIBLE, m_EnsureEntireWindowVisible);
	DDX_Control(pDX, IDC_CHECK_SHOW_ALL_IN_MAIN_LIST, m_ShowAllInMainList);
	DDX_Control(pDX, IDC_CHECK_FIND_AS_YOU_TYPE, m_FindAsYouType);
	DDX_Control(pDX, IDC_DRAW_RTF, m_btDrawRTF);
	DDX_Control(pDX, IDC_SHOW_THUMBNAILS, m_btShowThumbnails);
	DDX_Control(pDX, IDC_BUTTON_DEFAULT_FAULT, m_btDefaultButton);
	DDX_Control(pDX, IDC_BUTTON_FONT, m_btFont);
	DDX_Control(pDX, IDC_SHOW_TEXT_FOR_FIRST_TEN_HOT_KEYS, m_btShowText);
	DDX_Control(pDX, IDC_LINES_ROW, m_eLinesPerRow);
	DDX_Control(pDX, IDC_TRANS_PERC, m_eTransparencyPercent);
	DDX_Control(pDX, IDC_TRANSPARENCY, m_btEnableTransparency);
	DDX_Control(pDX, IDC_DESC_SHOW_LEADING_WHITESPACE, m_btDescShowLeadingWhiteSpace);
	//}}AFX_DATA_MAP
	DDX_Control(pDX, IDC_CHECK_PROMPT_DELETE_CLIP, m_PromptForDelete);
	DDX_Control(pDX, IDC_COMBO_THEME, m_cbTheme);
	DDX_Control(pDX, IDC_CHECK_SHOW_SCROLL_BAR, m_alwaysShowScrollBar);
	DDX_Control(pDX, IDC_CHECK_ELEVATE_PRIVILEGES, m_elevatedPrivileges);
	DDX_Control(pDX, IDC_CHECK_SHOW_IN_TASKBAR, m_showInTaskBar);
	DDX_Control(pDX, IDC_EDIT_DIFF_PATH, m_diffPathEditBox);
}


BEGIN_MESSAGE_MAP(COptionsQuickPaste, CPropertyPage)
	//{{AFX_MSG_MAP(COptionsQuickPaste)
	ON_BN_CLICKED(IDC_BUTTON_FONT, OnButtonFont)
	ON_BN_CLICKED(IDC_BUTTON_DEFAULT_FAULT, OnButtonDefaultFault)
	//}}AFX_MSG_MAP
	ON_BN_CLICKED(IDC_BUTTON_THEME, OnBnClickedButtonTheme)
	ON_BN_CLICKED(IDC_BUTTON_DIFF_BROWSE, &COptionsQuickPaste::OnBnClickedButtonDiffBrowse)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsQuickPaste message handlers

BOOL COptionsQuickPaste::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	m_pParent = (COptionsSheet *)GetParent();
	
	m_btEnableTransparency.SetCheck(Settings().GetEnableTransparency());
	m_eTransparencyPercent.SetNumber(Settings().GetTransparencyPercent());
	m_eLinesPerRow.SetNumber(Settings().GetLinesPerRow());
	m_alwaysShowScrollBar.SetCheck(Settings().GetShowScrollBar());
	m_btShowThumbnails.SetCheck(Settings().m_bDrawThumbnail);
	m_btDrawRTF.SetCheck(Settings().m_bDrawRTF);

	m_EnsureEntireWindowVisible.SetCheck(Settings().m_bEnsureEntireWindowCanBeSeen);
	m_ShowAllInMainList.SetCheck(Settings().m_bShowAllClipsInMainList);
	m_FindAsYouType.SetCheck(Settings().m_bFindAsYouType);

	if(Settings().GetQuickPastePosition() == CGetSetOptions::PosAtCaret)
		CheckDlgButton(IDC_AT_CARET, BST_CHECKED);
	else if(Settings().GetQuickPastePosition() == CGetSetOptions::PosAtCursor)
		CheckDlgButton(IDC_AT_CURSOR, BST_CHECKED);
	else if(Settings().GetQuickPastePosition() == CGetSetOptions::PosAtPrevious)
		CheckDlgButton(IDC_AT_PREVIOUS, BST_CHECKED);

	m_btDescShowLeadingWhiteSpace.SetCheck(Settings().m_bDescShowLeadingWhiteSpace);

	m_btShowText.SetCheck(Settings().GetShowTextForFirstTenHotKeys());
	m_PromptForDelete.SetCheck(Settings().GetPromptWhenDeletingClips());
	m_elevatedPrivileges.SetCheck(Settings().GetPasteAsAdmin());
	m_showInTaskBar.SetCheck(Settings().GetShowInTaskBar());

	if(Settings().GetFont(m_LogFont))
	{		
		m_Font.CreateFontIndirect(&m_LogFont);
		m_btFont.SetFont(&m_Font);
	}
	else
	{
		CFont *ft =	m_btFont.GetFont();
		ft->GetLogFont(&m_LogFont);
	}

	m_diffPathEditBox.SetWindowText(Settings().GetDiffApp());

	CString cs;
	cs.Format(_T("Font - %s"), m_LogFont.lfFaceName);
	m_btFont.SetWindowText(cs);

	if (Settings().GetShowIfClipWasPasted())
		CheckDlgButton(IDC_CHECK_SHOW_CLIP_WAS_PASTED, BST_CHECKED);

	FillThemes();

	theApp.m_Language.UpdateOptionQuickPaste(this);
		
	return FALSE;
}

BOOL COptionsQuickPaste::OnApply() 
{
	Settings().SetEnableTransparency(m_btEnableTransparency.GetCheck());
	Settings().SetTransparencyPercent(m_eTransparencyPercent.GetNumber());
	Settings().SetLinesPerRow(m_eLinesPerRow.GetNumber());
	Settings().SetShowScrollBar(m_alwaysShowScrollBar.GetCheck());

	ApplyQuickPastePosition();

	Settings().SetDescShowLeadingWhiteSpace(m_btDescShowLeadingWhiteSpace.GetCheck());
	Settings().SetShowTextForFirstTenHotKeys(m_btShowText.GetCheck());
	Settings().SetDrawThumbnail(m_btShowThumbnails.GetCheck());
	Settings().SetDrawRTF(m_btDrawRTF.GetCheck());
	Settings().SetEnsureEntireWindowCanBeSeen(m_EnsureEntireWindowVisible.GetCheck());
	Settings().SetShowAllClipsInMainList(m_ShowAllInMainList.GetCheck());
	Settings().SetFindAsYouType(m_FindAsYouType.GetCheck());
	Settings().SetPromptWhenDeletingClips(m_PromptForDelete.GetCheck());
	Settings().SetPasteAsAdmin(m_elevatedPrivileges.GetCheck());

	BOOL prevValue = Settings().GetShowInTaskBar();
	Settings().SetShowInTaskBar(m_showInTaskBar.GetCheck());
	if(Settings().GetShowInTaskBar() != prevValue)
	{
		theApp.RefreshShowInTaskBar();
	}
	
	if(m_LogFont.lfWeight != 0)
	{
		Settings().SetFont(m_LogFont);
	}

	ApplyTheme();

	CString diffPath;
	m_diffPathEditBox.GetWindowText(diffPath);
	Settings().SetDiffApp(diffPath);

	if (IsDlgButtonChecked(IDC_CHECK_SHOW_CLIP_WAS_PASTED))
		Settings().SetShowIfClipWasPasted(TRUE);
	else
		Settings().SetShowIfClipWasPasted(FALSE);

	return CPropertyPage::OnApply();
}

void COptionsQuickPaste::ApplyQuickPastePosition()
{
	if(IsDlgButtonChecked(IDC_AT_CARET))
		Settings().SetQuickPastePosition(CGetSetOptions::PosAtCaret);
	else if(IsDlgButtonChecked(IDC_AT_CURSOR))
		Settings().SetQuickPastePosition(CGetSetOptions::PosAtCursor);
	else if(IsDlgButtonChecked(IDC_AT_PREVIOUS))
		Settings().SetQuickPastePosition(CGetSetOptions::PosAtPrevious);
}

void COptionsQuickPaste::ApplyTheme()
{
	CString currentTheme = Settings().GetTheme();

	CString csTheme;
	if(m_cbTheme.GetCurSel() >= 0)
	{
		m_cbTheme.GetLBText(m_cbTheme.GetCurSel(), csTheme);
		if (csTheme == s_defaultTheme)
		{
			Settings().SetTheme("");
			csTheme = _T("");
		}
		else
			Settings().SetTheme(csTheme);
	}
	else
	{
		Settings().SetTheme("");
	}

	if (currentTheme != csTheme)
	{
		m_pParent->m_themeChanged = TRUE;
	}
}

void COptionsQuickPaste::OnButtonFont() 
{
	CFontDialog dlg(&m_LogFont, (CF_TTONLY | CF_SCREENFONTS), 0, this);
	if(dlg.DoModal() == IDOK)
	{	
		m_Font.DeleteObject();

		memcpy(&m_LogFont, dlg.m_cf.lpLogFont, sizeof(LOGFONT));		

		m_Font.CreateFontIndirect(&m_LogFont);

		m_btFont.SetFont(&m_Font);

		CString cs;
		cs.Format(_T("Font - %s"), m_LogFont.lfFaceName);
		m_btFont.SetWindowText(cs);
	}
}

void COptionsQuickPaste::OnButtonDefaultFault() 
{
	CFont *ft =	m_btDefaultButton.GetFont();
	ft->GetLogFont(&m_LogFont);

	memset(&m_LogFont, 0, sizeof(m_LogFont));

	m_LogFont.lfHeight = -10;
	m_LogFont.lfWeight = 400;
	m_LogFont.lfCharSet = 1;
	_tcscpy(m_LogFont.lfFaceName, _T("Segoe UI"));

	m_Font.DeleteObject();
	m_Font.CreateFontIndirect(&m_LogFont);

	m_btFont.SetFont(&m_Font);

	CString cs;
	cs.Format(_T("Font - %s"), m_LogFont.lfFaceName);
	m_btFont.SetWindowText(cs);
}	

void COptionsQuickPaste::FillThemes()
{
	CString csFile = Settings().GetPath(CGetSetOptions::PathThemes);
	csFile += "*.xml";

	CString csTheme = Settings().GetTheme();

	CFileFind find;
	BOOL bCont = find.FindFile(csFile);
	bool bSetCurSel = false;

	while(bCont)
	{
		bCont = find.FindNextFile();

		CTheme theme;
		if (theme.Load(Settings(), find.GetFileTitle(), true, false))
		{
			if (theme.FileVersion() >= 2 && theme.FileVersion() < 100)
			{
				int nIndex = m_cbTheme.AddString(find.GetFileTitle());

				if (find.GetFileTitle() == csTheme)
				{
					m_cbTheme.SetCurSel(nIndex);
					bSetCurSel = true;
				}
			}
		}
	}

	int nIndex = m_cbTheme.AddString(s_defaultTheme);
	if(bSetCurSel == false)
	{
		m_cbTheme.SetCurSel(nIndex);
	}
}

void COptionsQuickPaste::OnBnClickedButtonTheme()
{
	CTheme theme;

	CString csTheme;
	m_cbTheme.GetLBText(m_cbTheme.GetCurSel(), csTheme);

	if(csTheme == s_defaultTheme)
		return;
	
	if(theme.Load(Settings(), csTheme, true, false))
	{
		CString csMessage;

		csMessage.Format(_T("Theme -  %s\n")
			_T("Version -   %d\n")
			_T("Author -   %s\n")
			_T("Notes -   %s"), csTheme.GetString(),
			theme.FileVersion(),
			theme.Author().GetString(),
			theme.Notes().GetString());

		MessageBox(csMessage, _T("Ditto"), MB_OK);
	}
	else
	{
		CString csError;
		csError.Format(_T("Error loading theme file - %s - reason = %s"), csTheme.GetString(), theme.LastError().GetString());

		MessageBox(csError, _T("Ditto"), MB_OK);
	}
}


void COptionsQuickPaste::OnBnClickedButtonDiffBrowse()
{
	OPENFILENAME	FileName;
	TCHAR			szFileName[400];
	TCHAR			szDir[400];

	memset(&FileName, 0, sizeof(FileName));
	memset(szFileName, 0, sizeof(szFileName));
	memset(&szDir, 0, sizeof(szDir));
	FileName.lStructSize = sizeof(FileName);
	FileName.lpstrTitle = _T("Diff Application");
	FileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;
	FileName.nMaxFile = _countof(szFileName);
	FileName.lpstrFile = szFileName;
	FileName.lpstrInitialDir = szDir;
	FileName.lpstrFilter = _T("*.exe");
	FileName.lpstrDefExt = _T("");
	FileName.hwndOwner = m_hWnd;

	if(GetOpenFileName(&FileName) == 0)
		return;

	CString csPath(CFileDialogPath::From(FileName));

	m_diffPathEditBox.SetWindowText(csPath);
}

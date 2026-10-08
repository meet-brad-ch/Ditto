// OptionsSheet.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "OptionsSheet.h"
#include "OptionsKeyBoard.h"
#include "OptionsGeneral.h"
#include "OptionsQuickPaste.h"
#include "OptionsStats.h"
#include "OptionsTypes.h"
#include "About.h"
#include "OptionsCopyBuffers.h"
#include "Misc.h"
#include "QuickPasteKeyboard.h"



/////////////////////////////////////////////////////////////////////////////
// COptionsSheet

IMPLEMENT_DYNAMIC(COptionsSheet, CPropertySheet)

COptionsSheet::COptionsSheet(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
	m_themeChanged = FALSE;
	m_hWndParent = NULL;

	EnableStackedTabs(TRUE);

	m_pGeneralOptions = std::make_unique<COptionsGeneral>();
	m_pKeyBoardOptions = std::make_unique<COptionsKeyBoard>();
	//m_pQuickPasteOptions = new COptionsQuickPaste;
	m_pQuickPasteShortCuts = std::make_unique<CQuickPasteKeyboard>();

	m_pCopyBuffers = std::make_unique<COptionsCopyBuffers>();
	m_pStats = std::make_unique<COptionsStats>();
	m_pTypes = std::make_unique<COptionsTypes>();
	m_pAbout = std::make_unique<CAbout>();

	AddPage(m_pGeneralOptions.get());
	AddPage(m_pTypes.get());
	AddPage(m_pKeyBoardOptions.get());
	AddPage(m_pCopyBuffers.get());
	//AddPage(m_pQuickPasteOptions);
	AddPage(m_pQuickPasteShortCuts.get());
	AddPage(m_pStats.get());
	AddPage(m_pAbout.get());


}

COptionsSheet::~COptionsSheet()
{
}

BEGIN_MESSAGE_MAP(COptionsSheet, CPropertySheet)
	//{{AFX_MSG_MAP(COptionsSheet)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	ON_WM_DESTROY()
	ON_WM_NCDESTROY()
	//ON_WM_CLOSE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsSheet message handlers

void COptionsSheet::OnDestroy()
{
	CPropertySheet::OnDestroy();
}

void COptionsSheet::SetNotifyWnd(HWND hWnd)
{
	m_hWndParent = hWnd;
}

BOOL COptionsSheet::OnInitDialog() 
{
	m_bModeless = FALSE;   
	m_nFlags |= WF_CONTINUEMODAL;

	HICON b = (HICON)LoadImage(AfxGetInstanceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, 64, 64, LR_SHARED);
	SetIcon(b, TRUE);

	BOOL bResult = CPropertySheet::OnInitDialog();

	SetWindowText(_T("Options"));

	theApp.Services().Language().UpdateOptionsSheet(this);

	::ShowWindow(::GetDlgItem(m_hWnd, ID_APPLY_NOW), SW_HIDE);

	m_bModeless = TRUE;
	m_nFlags &= ~WF_CONTINUEMODAL;

	return bResult;
}

void COptionsSheet::OnNcDestroy()
{
	CPropertySheet::OnNcDestroy();
	::PostMessage(m_hWndParent, CDittoMessage::OptionsClosed, m_themeChanged, 0);
}

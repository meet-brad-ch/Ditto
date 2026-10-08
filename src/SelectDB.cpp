// SelectDB.cpp : implementation file
//

#include "stdafx.h"
#include "FileDialogPath.h"
#include "cp_main.h"
#include "SelectDB.h"

/////////////////////////////////////////////////////////////////////////////
// CSelectDB dialog


CSelectDB::CSelectDB(CWnd* pParent /*=NULL*/)
	: CDialog(CSelectDB::IDD, pParent)
{
	//{{AFX_DATA_INIT(CSelectDB)
	//}}AFX_DATA_INIT
}


void CSelectDB::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CSelectDB)
	DDX_Control(pDX, IDC_PATH, m_ePath);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CSelectDB, CDialog)
	//{{AFX_MSG_MAP(CSelectDB)
	ON_BN_CLICKED(IDC_SELECT, OnSelect)
	ON_BN_CLICKED(IDC_USE_DEFAULT, OnUseDefault)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CSelectDB message handlers

BOOL CSelectDB::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	m_ePath.SetWindowText(theApp.Services().Settings().GetDBPath(FALSE));
	
	m_ePath.SetFocus();
	
	return FALSE;
}

void CSelectDB::OnOK() 
{
	CString csPath;
	m_ePath.GetWindowText(csPath);

	theApp.Services().Settings().SetDBPath(csPath);

	CDialog::OnOK();
}

void CSelectDB::OnSelect() 
{
	OPENFILENAME	FileName;

	TCHAR			szFileName[400];
	TCHAR			szDir[400];

	memset(&FileName, 0, sizeof(FileName));
	memset(szFileName, 0, sizeof(szFileName));
	memset(&szDir, 0, sizeof(szDir));

	FileName.lStructSize = sizeof(FileName);

	
	FileName.lpstrTitle = _T("Open Database");
	FileName.Flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT|OFN_PATHMUSTEXIST;
	FileName.nMaxFile = _countof(szFileName);
	FileName.lpstrFile = szFileName;
	FileName.lpstrInitialDir = szDir;
	FileName.lpstrFilter = _T("Database Files (.MDB)\0*.mdb");
	FileName.lpstrDefExt = _T("mdb");

	if(GetOpenFileName(&FileName) == 0)
		return;

	CString	csPath(CFileDialogPath::From(FileName));

	if(DatabaseSchemaUpgrader::ValidDB(csPath) == FALSE)
	{
		MessageBox(_T("Invalid Database"), _T("Ditto"), MB_OK);
		m_ePath.SetFocus();
	}
	else
		m_ePath.SetWindowText(csPath);	
}

void CSelectDB::OnUseDefault() 
{
	CGetSetOptions& settings = theApp.Services().Settings();
	settings.SetDBPath("");
	CString csPath = settings.GetDBPath();

	if(DatabaseSchemaUpgrader::ValidDB(csPath) == FALSE)
		DeleteFile(csPath);

	if(DatabaseLocator::CheckDBExists(settings, theApp.Services().Language(), theApp.Services().Database(), theApp.Services().State(), settings.GetDBPath()))
		EndDialog(IDOK);
}

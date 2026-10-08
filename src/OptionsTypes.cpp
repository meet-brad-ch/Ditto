// OptionsTypes.cpp : implementation file
//

#include "stdafx.h"
#include "CP_Main.h"
#include "OptionsTypes.h"
#include "..\Shared\ArrayEx.h"
#include "DimWnd.h"
#include "Misc.h"
#include "ErrorReport.h"
#include "DittoDbTransaction.h"

/////////////////////////////////////////////////////////////////////////////
// COptionsTypes property page

IMPLEMENT_DYNCREATE(COptionsTypes, CPropertyPage)

COptionsTypes::COptionsTypes() : CPropertyPage(COptionsTypes::IDD)
{
	m_csTitle = theApp.Services().Language().GetString("SupportedTypesTitle", "Supported Types");
	m_psp.pszTitle = m_csTitle;
	m_psp.dwFlags |= PSP_USETITLE; 

	//{{AFX_DATA_INIT(COptionsTypes)
	//}}AFX_DATA_INIT

	m_bSave = false;
}

COptionsTypes::~COptionsTypes()
{
}

void COptionsTypes::DoDataExchange(CDataExchange* pDX)
{
	CPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(COptionsTypes)
	DDX_Control(pDX, IDC_LIST1, m_List);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(COptionsTypes, CPropertyPage)
	//{{AFX_MSG_MAP(COptionsTypes)
	ON_BN_CLICKED(IDC_DELETE, OnDelete)
	ON_BN_CLICKED(IDC_ADD, OnAdd)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// COptionsTypes message handlers

BOOL COptionsTypes::OnApply() 
{
	if(m_bSave)
	{
		try
		{
			CDittoDb& database = theApp.Services().Database();
			// one transaction: upstream deleted the types first, so a failed insert left the list
			// empty or partial
			CDittoDbTransaction transaction(database);
			database.execDML(_T("DELETE FROM Types;"));

			// bound: a type name with a quote broke upstream's formatted INSERT
			CppSQLite3Statement insert = database.compileStatement(_T("INSERT INTO Types VALUES(NULL, ?);"));
			CString csText;
			int nCount = m_List.GetCount();
			for(int i = 0; i < nCount; i++)
			{
				m_List.GetText(i, csText);

				insert.bind(1, csText.GetString());
				insert.execDML();
				insert.reset();
			}

			transaction.Commit();
		}
		catch (CppSQLite3Exception& e)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Saving the supported clipboard types failed: %s"), e.errorMessage()));
			return FALSE;
		}

		// refresh our local cache
		theApp.Services().Clipboard().ReloadTypes();
	}
	
	return CPropertyPage::OnApply();
}

BOOL COptionsTypes::OnInitDialog() 
{
	CPropertyPage::OnInitDialog();

	try
	{
		CppSQLite3Query q = theApp.Services().Database().execQuery(_T("SELECT TypeText FROM Types"));
		if(q.eof())
		{
			m_List.AddString(_T("CF_TEXT"));
			// Registered formats are in 0xC000-0xFFFF, so they fit a CLIPFORMAT
			m_List.AddString(CClipboardFormats::GetFormatName(static_cast<CLIPFORMAT>(RegisterClipboardFormat(CF_RTF))));
			m_List.AddString(_T("CF_UNICODETEXT"));
			m_List.AddString(_T("CF_HDROP"));
			m_List.AddString(_T("CF_DIB"));
			m_List.AddString(CClipboardFormats::GetFormatName(CClipboardFormats::GetFormatID(_T("HTML Format"))));
			m_List.AddString(CClipboardFormats::GetFormatName(CClipboardFormats::GetFormatID(_T("PNG"))));
		}

		while(q.eof() == false)
		{
			m_List.AddString(q.getStringField(0));

			q.nextRow();
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Loading the supported clipboard types failed: %s"), e.errorMessage()));
		return FALSE;
	}

	m_List.SetFocus();

	theApp.Services().Language().UpdateOptionSupportedTypes(this);
	
	return FALSE;
}

void COptionsTypes::OnDelete() 
{
	int nCount = m_List.GetSelCount();
	if(nCount)
	{
		m_bSave = true;
		CArrayEx<int> items;
		items.SetSize(nCount);
		m_List.GetSelItems(nCount, items.GetData()); 
		items.SortDescending();

		for(int i = 0; i < nCount; i++)
			m_List.DeleteString(items[i]);
	}
}

#include "AddType.h"
void COptionsTypes::OnAdd() 
{
	CDimWnd dim(this->GetParent());
	CAddType add(this);

	if(add.DoModal() == IDOK)
	{
		INT_PTR nCount = add.m_csSelectedTypes.GetSize();
		if(nCount)
		{
			m_bSave = true;
			for(int i = 0; i < nCount; i++)
			{
				if(TextAllReadyThere(add.m_csSelectedTypes[i]) == FALSE)
					m_List.AddString(add.m_csSelectedTypes[i]);
			}
		}
	}	
}

BOOL COptionsTypes::TextAllReadyThere(const CString &cs)
{
	CString csThere;
	int nCount = m_List.GetCount();

	for(int i = 0; i < nCount; i++)
	{
		m_List.GetText(i, csThere);
		if(cs == csThere)
			return TRUE;
	}

	return FALSE;
}

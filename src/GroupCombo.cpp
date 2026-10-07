// GroupCombo.cpp : implementation file
//

#include "stdafx.h"
#include "cp_main.h"
#include "GroupCombo.h"

/////////////////////////////////////////////////////////////////////////////
// CGroupCombo

CGroupCombo::CGroupCombo()
{
	m_lSkipGroupID = -1;
}

CGroupCombo::~CGroupCombo()
{
}


BEGIN_MESSAGE_MAP(CGroupCombo, CComboBox)
	//{{AFX_MSG_MAP(CGroupCombo)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CGroupCombo message handlers

void CGroupCombo::FillCombo()
{
	ResetContent();

	int nIndex = AddString(_T("-No Group-"));
	// -1 marks the "No Group" entry; readers convert the item data back to a signed id
	SetItemData(nIndex, static_cast<DWORD_PTR>(-1));

	FillCombo(-1, 1);
}

// A CppSQLite3Exception propagates: the caller (CCopyProperties::OnInitDialog) reports it and closes
// its dialog, so a partly filled combo is never saved as the clip's group.
void CGroupCombo::FillCombo(long lParentID, long lSpaces)
{
	int nIndex{};
	CString csSpaces;

	for(int i = 0; i < lSpaces; i++)
	{
		csSpaces += "---";
	}

	//First time through
	if(lSpaces > 0)
	{
		csSpaces += " ";
		//ResetContent();
	}

	lSpaces++;

	CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT lID, mText FROM Main WHERE bIsGroup = 1 AND lParentID = %d"), lParentID);

	if(q.eof() == false)
	{
		while(!q.eof())
		{
			if(q.getIntField(_T("lID")) != m_lSkipGroupID)
			{
				nIndex = AddString(csSpaces + q.getStringField(_T("mText")));
				SetItemData(nIndex, q.getIntField(_T("lID")));

				FillCombo(q.getIntField(_T("lID")), lSpaces);
			}

			q.nextRow();
		}
	}
}

BOOL CGroupCombo::SetCurSelOnItemData(long lItemData)
{
	long lCount = GetCount();

	for(int i = 0; i < lCount; i++)
	{
		// the item data holds a group id (a long) or -1
		if(static_cast<long>(GetItemData(i)) == lItemData)
		{
			SetCurSel(i);
			return TRUE;
		}
	}

	SetCurSel(-1);

	return FALSE;
}

int CGroupCombo::GetItemDataFromCursel()
{
	int nCursel = GetCurSel();
	return (int)GetItemData(nCursel);
}
// FormatSQL.cpp: implementation of the CFormatSQL class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cp_main.h"
#include "FormatSQL.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CFormatSQL::CFormatSQL(CGetSetOptions& settings) :
	m_settings(settings)
{

}

CFormatSQL::~CFormatSQL()
{

}

void CFormatSQL::Parse(CString cs)
{
	//Replace all single ' with a double '
	cs.Replace(_T("'"), _T("''"));

	if(m_settings.GetSimpleTextSearch() ||
		m_settings.GetRegExTextSearch())
	{
		eSpecialTypes invalid = eINVALID;
		AddToSQL(cs, invalid, invalid);
		return;
	}

	cs.Replace(_T("["), _T(" "));
	cs.Replace(_T("]"), _T(" "));

	cs.Replace(_T("*"), _T("%"));

	int nLength = cs.GetLength();

	CString csCurrentWord;
	bool bInQuotes = false;
	eSpecialTypes eNotValue = eINVALID;
	eSpecialTypes eOrValue = eAND;

	for(int i = 0; i < nLength; i++)
	{
		switch(cs[i])
		{
		case '"':
			bInQuotes = !bInQuotes;
			break;
		case ' ':
			if(bInQuotes == false)
			{
				AddWord(csCurrentWord, eNotValue, eOrValue);
				csCurrentWord = "";
			}
			else
			{
				csCurrentWord += cs[i];
			}
			break;
		default:
			csCurrentWord += cs[i];
		}
	}

	if(csCurrentWord.GetLength() > 0)
		AddToSQL(csCurrentWord, eNotValue, eOrValue);
}

void CFormatSQL::AddWord(const CString& csCurrentWord, eSpecialTypes &eNotValue, eSpecialTypes &eOrValue)
{
	eSpecialTypes sp = ConvetToKey(csCurrentWord);
	switch(sp)
	{
	case eNOT:
		eNotValue = sp;
		break;
	case eOR:
	case eAND:
		eOrValue = sp;
		break;
	default:
		AddToSQL(csCurrentWord, eNotValue, eOrValue);
	}
}

CFormatSQL::eSpecialTypes CFormatSQL::ConvetToKey(CString cs)
{
	cs.MakeUpper();

	if(cs == "NOT" || 
		cs == "!")
	{
		return eNOT;
	}
	else if(cs == "OR")
	{
		return eOR;
	}
	else if(cs == "AND")
	{
		return eAND;
	}

	return eINVALID;
}

CString CFormatSQL::GetKeyWordString(eSpecialTypes eKeyWord)
{
	switch(eKeyWord)
	{
	case eNOT:
		return " NOT ";
	case eAND:
		return " AND ";
	case eOR:
		return " OR ";
	case eINVALID:
		return " ";
	}

	return " ";
}

bool CFormatSQL::AddToSQL(CString cs, eSpecialTypes &eNOTValue, eSpecialTypes &eORValue)
{
	CString csThisSQL;
	cs.TrimLeft();
	cs.TrimRight();

	if (m_settings.GetRegExTextSearch())
	{
		// SQLite's ICU regexp() matches the whole text: (?s:.*) on both sides finds the pattern
		// anywhere, and (?i) makes the match case-insensitive when the option is set
		const CString caseFlag = m_settings.GetRegexCaseInsensitive() ? _T("(?i)") : _T("");
		csThisSQL.Format(_T("%s REGEXP \'%s(?s:.*)(?:%s)(?s:.*)\'"), m_csVariable.GetString(), caseFlag.GetString(), cs.GetString());
	}
	else if (m_settings.GetSimpleTextSearch())
	{
		// the search text, not the column name: a % in it must match a literal %
		if (cs.Find(_T("%")) >= 0)
		{
			CString local(cs);
			local.Replace(_T("%"), _T("\\%"));
			
			//escape the % character, https://www.sqlitetutorial.net/sqlite-like/
			csThisSQL.Format(_T("%s LIKE \'%%%s%%\' ESCAPE \'\\\'"), m_csVariable.GetString(), local.GetString());
		}
		else
		{
			csThisSQL.Format(_T("%s LIKE \'%%%s%%\'"), m_csVariable.GetString(), cs.GetString());
		}
	}
	else if (cs.Find(_T("%")) < 0)
	{
		csThisSQL.Format(_T("%s%sLIKE \'%%%s%%\'"), m_csVariable.GetString(), GetKeyWordString(eNOTValue).GetString(), cs.GetString());
	}
	else
	{
		CString local(cs);
		local.Replace(_T("%"), _T("\\%"));
		csThisSQL.Format(_T("%s%sLIKE \'%%%s%%\' ESCAPE \'\\\'"), m_csVariable.GetString(), GetKeyWordString(eNOTValue).GetString(), local.GetString());
	}

	if(m_csWhere.GetLength() > 0)
	{
		m_csWhere += GetKeyWordString(eORValue) + csThisSQL;
	}
	else
	{
		m_csWhere = csThisSQL;
	}

	eNOTValue = eINVALID;
	eORValue = eAND;

	return true;
}

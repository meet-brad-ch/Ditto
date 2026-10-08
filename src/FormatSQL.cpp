// FormatSQL.cpp: implementation of the CFormatSQL class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cp_main.h"
#include "FormatSQL.h"
#include "SearchCondition.h"

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
	// the search SQL is built by DittoCore::SearchCondition (tested in DittoTests)
	DittoCore::SearchCondition::Options options{};
	options.regexCaseInsensitive = m_settings.GetRegexCaseInsensitive() != FALSE;
	if (m_settings.GetRegExTextSearch())
	{
		options.mode = DittoCore::SearchCondition::Mode::Regex;
	}
	else if (m_settings.GetSimpleTextSearch())
	{
		options.mode = DittoCore::SearchCondition::Mode::Simple;
	}

	m_csWhere = DittoCore::SearchCondition::Build(m_csVariable.GetString(), cs.GetString(), options).c_str();
}


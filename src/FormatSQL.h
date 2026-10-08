// FormatSQL.h: interface for the CFormatSQL class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_FORMATSQL_H__3D7AC79C_FDD8_4948_B7CD_601FB513F208__INCLUDED_)
#define AFX_FORMATSQL_H__3D7AC79C_FDD8_4948_B7CD_601FB513F208__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CGetSetOptions;

class CFormatSQL
{
public:
	/**
	 * @brief Creates an empty search condition builder.
	 * @param settings The application's settings (simple/regex search options); must outlive this object.
	 */
	explicit CFormatSQL(CGetSetOptions& settings);
	virtual ~CFormatSQL();

	/**
	 * @brief Builds the search condition of the column (SetVariable) for a search text, with the
	 *        settings' search mode (DittoCore::SearchCondition); it replaces an earlier condition.
	 * @param cs The search text as typed.
	 */
	void Parse(CString cs);

	CString GetSQLString()				{ return _T("(") + m_csWhere + _T(")"); }
	void	SetVariable(CString cs)		{ m_csVariable = cs;}

protected:
	CString m_csWhere;
	CString m_csVariable;

private:
	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
};

#endif // !defined(AFX_FORMATSQL_H__3D7AC79C_FDD8_4948_B7CD_601FB513F208__INCLUDED_)

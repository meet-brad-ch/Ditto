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

	void Parse(CString cs);

	CString GetSQLString()				{ return _T("(") + m_csWhere + _T(")"); }
	void	SetVariable(CString cs)		{ m_csVariable = cs;}

protected:
	CString m_csWhere;
	CString m_csVariable;
	enum eSpecialTypes{eINVALID, eNOT, eAND, eOR};
	

	bool AddToSQL(CString cs, eSpecialTypes &eNOTValue, eSpecialTypes &eORValue);
	CFormatSQL::eSpecialTypes ConvetToKey(CString cs);
	CString GetKeyWordString(eSpecialTypes eKeyWord);

private:
	/**
	 * @brief Parse's step for a finished word: a NOT/OR/AND keyword sets the operator for the
	 * next term, any other word is added as a term.
	 * @param csCurrentWord the word.
	 * @param eNotValue the pending NOT operator (reset when a term is added).
	 * @param eOrValue the pending AND/OR operator (reset when a term is added).
	 */
	void AddWord(const CString& csCurrentWord, eSpecialTypes &eNotValue, eSpecialTypes &eOrValue);

	/// The application's settings (not owned).
	CGetSetOptions& m_settings;
};

#endif // !defined(AFX_FORMATSQL_H__3D7AC79C_FDD8_4948_B7CD_601FB513F208__INCLUDED_)

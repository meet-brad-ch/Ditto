// MainTableFunctions.h: interface for the CMainTableFunctions class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAINTABLEFUNCTIONS_H__3AE1D19B_D68D_48E7_80CA_AB62B0447883__INCLUDED_)
#define AFX_MAINTABLEFUNCTIONS_H__3AE1D19B_D68D_48E7_80CA_AB62B0447883__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CMainTableFunctions
{
public:
	CMainTableFunctions();
	virtual ~CMainTableFunctions();

	static void LoadAcceleratorKeys(CAccels& accels, CppSQLite3DB& db);
	/** @brief The text of a clip description for display: tabs as two spaces, and without each
	line's leading indent unless @p showLeadingWhiteSpace.
	@param nMaxLines unused.
	@param OrigText the description.
	@param showLeadingWhiteSpace keep the leading white space (the DescShowLeadingWhiteSpace setting).
	@return the display text. */
	static CString GetDisplayText(int nMaxLines, const CString& OrigText, BOOL showLeadingWhiteSpace);
};

#endif // !defined(AFX_MAINTABLEFUNCTIONS_H__3AE1D19B_D68D_48E7_80CA_AB62B0447883__INCLUDED_)

// ProcessPaste.h: interface for the CProcessCopy class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_PROCESSPASTE_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_)
#define AFX_PROCESSPASTE_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "..\Shared\ArrayEx.h"
#include "Clip.h"
#include "ClipIds.h"
#include "OleClipSource.h"
#include "SpecialPasteOptions.h"

#include <functional>

/*------------------------------------------------------------------*\
	CProcessPaste
\*------------------------------------------------------------------*/
class CProcessPaste
{
public:
	// One COM reference to the data source; null once SetClipboard or InternalRelease took it
	COleClipSource*	m_pOle;
	bool m_bSendPaste;
	bool m_bActivateTarget;
	CSpecialPasteOptions m_pasteOptions;
	bool m_pastedFromGroup;
	CString m_lastErrorMessage;

	struct MarkAsPastedData 
	{
		CClipIDs ids;
		bool pastedFromGroup{};
		bool updateClipOrder{};
	};
	
	CProcessPaste();
	~CProcessPaste();

	CClipIDs& GetClipIDs() { return m_pOle->m_ClipIDs; }

	BOOL DoPaste();
	BOOL DoDrag();

	void MarkAsPasted(bool updateClipOrder);
	static UINT MarkAsPastedThread(LPVOID pParam);

private:
	BOOL RunAtBoundary(LPCTSTR operation, const std::function<BOOL()>& body);
	// MarkAsPastedThread's order step for one clip: gives it the newest group order (pastedFromGroup)
	// or the newest main order, so it moves to the top of its list
	static void MoveToTopOrder(int id, bool pastedFromGroup);
};

#endif // !defined(AFX_PROCESSPASTE_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_)

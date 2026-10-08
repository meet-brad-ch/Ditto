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

class CClipContext;
class CDittoDb;
class ExternalWindowTracker;

/*------------------------------------------------------------------*\
	CProcessPaste
\*------------------------------------------------------------------*/
class CProcessPaste
{
public:
	// One COM reference to the data source; null once SetClipboard or InternalRelease took it
	COleClipSource* m_pOle;
	bool m_bSendPaste;
	bool m_bActivateTarget;
	CSpecialPasteOptions m_pasteOptions;
	bool m_pastedFromGroup;
	CString m_lastErrorMessage;

	struct MarkAsPastedData
	{
		/**
		 * @brief Creates the data for MarkAsPastedThread.
		 * @param clipContext The clip services (the settings, the database, the windows), used by
		 *        the thread; must outlive the thread.
		 */
		explicit MarkAsPastedData(CClipContext& clipContext) :
			context(clipContext)
		{
		}

		/** @brief The clip services (not owned). */
		CClipContext& context;
		CClipIDs ids;
		bool pastedFromGroup{};
		bool updateClipOrder{};
		/** @brief The settings' m_bUpdateTimeOnPaste when the paste ran (callers override it only during the paste). */
		bool updateTimeOnPaste{};
	};

	/**
	 * @brief Creates a paste with a new, empty data source.
	 * @param context The clip services (the settings, the registered formats, the database, the
	 *        windows); must outlive this object and the paste's MarkAsPastedThread.
	 * @param activeWindow The tracker of the window the paste goes to; must outlive this object.
	 */
	CProcessPaste(CClipContext& context, ExternalWindowTracker& activeWindow);
	~CProcessPaste();

	CClipIDs& GetClipIDs() { return m_pOle->m_ClipIDs; }

	BOOL DoPaste();
	BOOL DoDrag();

	void MarkAsPasted(bool updateClipOrder);
	static UINT MarkAsPastedThread(LPVOID pParam);

private:
	/// The clip services (not owned).
	CClipContext& m_context;
	/// The application's settings (not owned; m_context.Settings()).
	CGetSetOptions& m_settings;
	/// The tracker of the window the paste goes to (not owned).
	ExternalWindowTracker& m_activeWindow;

	BOOL RunAtBoundary(LPCTSTR operation, const std::function<BOOL()>& body);
	// MarkAsPastedThread's order step for one clip in db: gives it the newest group order
	// (pastedFromGroup) or the newest main order, so it moves to the top of its list
	static void MoveToTopOrder(CDittoDb& db, int id, bool pastedFromGroup);

	/**
	 * @brief MarkAsPastedThread's database step: moves the clips to the top (when the options ask),
	 * sets their paste time and refreshes them in the UI.
	 * @param data The pasted clips.
	 * @param clipId Set to the clip being updated, for the error report.
	 * @throws CppSQLite3Exception When an update fails; the remaining steps are skipped.
	 */
	static void UpdatePastedClips(MarkAsPastedData& data, int& clipId);
};

#endif // !defined(AFX_PROCESSPASTE_H__185CBB6F_4B63_4397_8FF9_E18D777DA506__INCLUDED_)

#include "stdafx.h"
#include "CP_Main.h"
#include "ProcessPaste.h"
#include "ClipIds.h"
#include "ClipboardFormatError.h"
#include "ErrorReport.h"
#include "ClipContext.h"
#include <memory>

CProcessPaste::CProcessPaste(CClipContext& context, ExternalWindowTracker& activeWindow) :
	m_context(context),
	m_settings(context.Settings()),
	m_activeWindow(activeWindow)
{
	m_pOle = std::make_unique<COleClipSource>().release(); // ownership: COM reference count (one reference, held by this object until SetClipboard or InternalRelease)
	m_bSendPaste = true;
	m_bActivateTarget = true;
	m_pastedFromGroup = false;
}

CProcessPaste::~CProcessPaste()
{
	// a COM object: dropping the last reference deletes it (never delete it directly)
	if (m_pOle != nullptr)
	{
		m_pOle->InternalRelease();
	}
}

// Error boundary of a paste or drag: whatever stops the operation (malformed clip data, a database
// error, an MFC or a standard exception) ends it and is kept in m_lastErrorMessage for the caller
// to show.
BOOL CProcessPaste::RunAtBoundary(LPCTSTR operation, const std::function<BOOL()>& body)
{
	try
	{
		return body();
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		m_lastErrorMessage.Format(_T("the clip's data is malformed (%s)"), CString(error.what()).GetString());
	}
	catch (CppSQLite3Exception& error)
	{
		m_lastErrorMessage.Format(_T("database error %d (%s)"), error.errorCode(), error.errorMessage());
	}
	catch (CException* ex)
	{
		TCHAR szCause[255]{};
		ex->GetErrorMessage(szCause, _countof(szCause));
		ex->Delete();
		m_lastErrorMessage.Format(_T("%s exception: %s"), operation, szCause);
	}
	catch (const std::exception& error)
	{
		// e.g. std::bad_alloc, or std::runtime_error from a failed system call while rendering the clip
		m_lastErrorMessage.Format(_T("%s failed: %s"), operation, CString(error.what()).GetString());
	}
	CLogger::Log(m_lastErrorMessage);
	return FALSE;
}

BOOL CProcessPaste::DoPaste()
{
	bool handedToClipboard{ false };
	const BOOL ret = RunAtBoundary(_T("Paste"), [this, &handedToClipboard]() -> BOOL
								   {
		m_pOle->m_pasteOptions = m_pasteOptions;
		if (!m_pOle->DoImmediateRender())
		{
			return FALSE;
		}

		// MarkAsPasted() must be done first since it makes use of
		//  m_pOle->m_ClipIDs and m_pOle is inaccessible after
		//  SetClipboard is called.
		MarkAsPasted(m_pasteOptions.m_updateClipOrder);

		// Ignore the clipboard change that we will cause IF:
		// 1) we are pasting a single element, since the element is already
		//    in the db and its lDate was updated by MarkAsPasted().
		// OR
		// 2) we are pasting multiple, but the settings' m_bSaveMultiPaste is false
		if (GetClipIDs().GetSize() == 1 || !m_settings.m_bSaveMultiPaste)
		{
			m_pOle->CacheGlobalData(m_context.Formats().IgnoreClipboard(), CGlobalMemory::NewGlobalP("Ignore", sizeof("Ignore")));
		}
		else
		{
			m_pOle->CacheGlobalData(m_context.Formats().DelaySavingData(), CGlobalMemory::NewGlobalP("Delay", sizeof("Delay")));
		}

		m_pOle->SetClipboard(); // m_pOle is now managed by the OLE clipboard
		handedToClipboard = true;

		if (m_bSendPaste)
		{
			CLogger::Log(_T("Sending Paste to active window"));
			m_activeWindow.SendPaste(m_bActivateTarget);
		}
		else if (m_bActivateTarget)
		{
			CLogger::Log(_T("Activating active window"));
			m_activeWindow.ActivateTarget();
		}
		return TRUE; });

	if (handedToClipboard)
	{
		// The Clipboard now owns the allocated memory and will delete this data object when new
		// data is put on the Clipboard. Until then the destructor still owns it.
		m_pOle = NULL;
	}
	return ret;
}

BOOL CProcessPaste::DoDrag()
{
	const BOOL ret = RunAtBoundary(_T("Drag drop"), [this]() -> BOOL
								   {
		m_pOle->m_pasteOptions = m_pasteOptions;
		m_pOle->DoDelayRender();
		DROPEFFECT de = m_pOle->DoDragDrop(DROPEFFECT_COPY);
		if (de == DROPEFFECT_NONE)
		{
			return FALSE;
		}
		MarkAsPasted(m_pasteOptions.m_updateClipOrder);
		return TRUE; });

	//from https://www.codeproject.com/Articles/886711/Drag-Drop-Images-and-Drop-Descriptions-for-MFC-App
	//You may have noted the InternalRelease() function call.This is required here to delete the object.While it is possible to use
	//delete or create the object on the stack with Drag & Drop operations, it is not recommended to do so.
	// No try: InternalRelease only drops the reference and runs the (non-throwing) destructor.
	m_pOle->InternalRelease();

	// The Clipboard now owns the allocated memory
	// and will delete this data object
	// when new data is put on the Clipboard
	m_pOle = NULL; // m_pOle should not be accessed past this point

	return ret;
}

void CProcessPaste::MarkAsPasted(bool updateClipOrder)
{
	CLogger::Log(_T("start of MarkAsPasted"));

	CClipIDs& clips = GetClipIDs();

	m_settings.SetTripPasteCount(-1);
	m_settings.SetTotalPasteCount(-1);

	auto pData{ std::make_unique<MarkAsPastedData>(m_context) };
	for (int i = 0; i < clips.GetCount(); i++)
	{
		pData->ids.Add(clips.ElementAt(i));
	}
	pData->pastedFromGroup = m_pastedFromGroup;
	pData->updateClipOrder = updateClipOrder;
	// taken now: the thread runs after the caller restored its temporary value
	pData->updateTimeOnPaste = m_settings.m_bUpdateTimeOnPaste != FALSE;

	//Moved to a thread because when running from from U3 devices the write is time consuming
	if (AfxBeginThread(CProcessPaste::MarkAsPastedThread, pData.get(), THREAD_PRIORITY_LOWEST) != nullptr)
	{
		pData.release(); // ownership: MarkAsPastedThread retakes it in a std::unique_ptr
	}
	else
	{
		// shown: upstream skipped the update silently
		CErrorReport::Show(_T("The pasted clips' order and paste time were not updated: the update thread could not be started."));
	}

	CLogger::Log(_T("End of MarkAsPasted"));
}

UINT CProcessPaste::MarkAsPastedThread(LPVOID pParam)
{
	ULONGLONG startTick = GetTickCount64();

	static CEvent UpdateTimeEvent(TRUE, TRUE, _T("Ditto_Update_Clip_Time"), NULL);
	UpdateTimeEvent.ResetEvent();

	CLogger::Log(_T("Start of MarkAsPastedThread"));

	BOOL bRet = FALSE;
	int clipId = 0;
	// owns the data from MarkAsPasted, also when an update below fails
	const std::unique_ptr<MarkAsPastedData> pData{ static_cast<MarkAsPastedData*>(pParam) };

	try
	{
		if (pData)
		{
			UpdatePastedClips(*pData, clipId);

			bRet = TRUE;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		// the remaining updates and the UI refresh are skipped; the event below is still set so the list query does not wait
		CErrorReport::Show(CStringUtil::Format(_T("Updating the order and paste time of pasted clip id %d failed: %s"), clipId, e.errorMessage()));
	}

	CLogger::Log(_T("End of MarkAsPastedThread"));

	ULONGLONG endTick = GetTickCount64();
	if ((endTick - startTick) > 350)
		CLogger::Log(CStringUtil::Format(_T("Paste Timing MarkAsPastedThread: %llu, ClipId: %d"), endTick - startTick, clipId));

	UpdateTimeEvent.SetEvent();
	return bRet;
}

void CProcessPaste::UpdatePastedClips(MarkAsPastedData& data, int& clipId)
{
	int refreshFlags = 0;

	int clipCount = (int)data.ids.GetCount();
	const CGetSetOptions& settings{ data.context.Settings() };
	CDittoDb& db{ data.context.Database() };

	if (data.updateTimeOnPaste &&
		data.updateClipOrder)
	{
		if (settings.m_refreshViewAfterPasting)
		{
			refreshFlags |= CClipRefreshFlags::AfterPasteSelectClip;
		}

		for (int i = 0; i < clipCount; i++)
		{
			int id = data.ids.ElementAt(i);
			clipId = id;
			MoveToTopOrder(db, id, data.pastedFromGroup);
		}
	}

	for (int i = 0; i < clipCount; i++)
	{
		int id = data.ids.ElementAt(i);
		clipId = id;
		db.execDMLEx(_T("UPDATE Main SET lastPasteDate = %d where lID = %d;"), (int)CTime::GetCurrentTime().GetTime(), id);
	}

	for (int i = 0; i < clipCount; i++)
	{
		int id = data.ids.ElementAt(i);
		data.context.Windows().RefreshClipInUI(id, refreshFlags);
	}
}

void CProcessPaste::MoveToTopOrder(CDittoDb& db, int id, bool pastedFromGroup)
{
	if (pastedFromGroup)
	{
		CppSQLite3Query q{ db.execQuery(_T("SELECT clipGroupOrder FROM Main ORDER BY clipGroupOrder DESC LIMIT 1")) };

		if (q.eof() == false)
		{
			double latestDate{ q.getFloatField(_T("clipGroupOrder")) };
			latestDate += 1;

			CLogger::Log(CStringUtil::Format(_T("Setting clipId: %d, GroupOrder: %f"), id, latestDate));

			db.execDMLEx(_T("UPDATE Main SET clipGroupOrder = %f where lID = %d;"), latestDate, id);
		}
	}
	else
	{
		CppSQLite3Query q{ db.execQuery(_T("SELECT clipOrder FROM Main ORDER BY clipOrder DESC LIMIT 1")) };

		if (q.eof() == false)
		{
			double latestDate{ q.getFloatField(_T("clipOrder")) };
			latestDate += 1;

			CLogger::Log(CStringUtil::Format(_T("Setting clipId: %d, order: %f"), id, latestDate));

			db.execDMLEx(_T("UPDATE Main SET clipOrder = %f where lID = %d;"), latestDate, id);
		}
	}
}

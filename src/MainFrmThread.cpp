#include "stdafx.h"
#include "MainFrmThread.h"
#include "DatabaseUtilities.h"
#include "Options.h"
#include "Misc.h"
#include "cp_main.h"
#include "ErrorReport.h"

CMainFrmThread::CMainFrmThread(CGetSetOptions& settings, CIdleTime& idleTime, CDittoDb& database, CClipboardMonitor& clipboard, CAppWindows& windows) :
	m_settings(settings),
	m_idleTime(idleTime),
	m_database(database),
	m_clipboard(clipboard),
	m_windows(windows)
{
	m_threadName = "CMainFrmThread";
	for (int eventEnum = 0; eventEnum < ECMAINFRMTHREADEVENTS_COUNT; eventEnum++)
	{
		AddEvent(eventEnum);
	}
}

CMainFrmThread::~CMainFrmThread(void)
{
}

void CMainFrmThread::AddClipToSave(std::unique_ptr<CClip> clip)
{
	ATL::CCritSecLock csLock(m_cs.m_sect);

	CLogger::Log(_T("Adding clip to thread for save to db"));
	m_saveClips.Add(std::move(clip));
	FireEvent(SAVE_CLIPS);
}

void CMainFrmThread::OnEvent(int eventId, void* /*param*/)
{
	switch ((eCMainFrmThreadEvents)eventId)
	{
	case DELETE_ENTRIES:
		OnDeleteEntries();
		break;
	case REMOVE_TEMP_FILES:
		OnRemoveTempFiles();
		break;
	case SAVE_CLIPS:
		OnSaveClips();
		break;
	case READ_DB_FILE:
		OnReadDbFile();
		break;
	}
}

//try and keep our db file in windows cache by randomly reading some data
//not sure if this does what i think it does but looking into issues with slow access on large dbs
void CMainFrmThread::OnReadDbFile()
{
	double idle = m_idleTime.IdleSeconds();

	if (idle < m_settings.ReadRandomFileIdleMin())
	{
		CString dbFile = m_settings.GetDBPath();
		__int64 dbSize = CFileSystem::FileSize(dbFile);

		srand((UINT)time(NULL));

		int random = rand() % (dbSize - 1024) + 1;

		CFile f;
		if (f.Open(dbFile, CFile::modeRead | CFile::shareDenyNone))
		{
			f.Seek(random, 0);
			char data[1024];
			f.Read(&data, 1024);

			f.Close();
		}
	}
}

void CMainFrmThread::OnDeleteEntries()
{
	CClipRetentionPolicy::RemoveOldEntries(m_settings, m_idleTime, m_windows, true);
}

void CMainFrmThread::OnRemoveTempFiles()
{
	CTempFileCleaner::DeleteDittoTempFiles(m_settings, TRUE);
}

void CMainFrmThread::OnSaveClips()
{
	CClipList localClips{};

	//Save the clips locally
	{
		ATL::CCritSecLock csLock(m_cs.m_sect);

		//localClips now owns the clips
		localClips = m_saveClips.TakeAll();
	}

	// the copy reason of the newest clip
	CopyReasonEnum::CopyReason copyReason{ CopyReasonEnum::COPY_TO_UNKOWN };
	if (!localClips.IsEmpty())
	{
		copyReason = localClips.Last().m_copyReason;
	}

	CLogger::Log(_T("SaveCopyClips Before AddToDb"));

	int count = localClips.AddToDB(true);

	CLogger::Log(CStringUtil::Format(_T("SaveCopyclips After AddToDb, Count: %d"), count));

	// the newest clip that was saved (set when count > 0): upstream used the newest clip, also
	// when its save failed
	const CClip* const pLastSaved{ localClips.LastSaved() };
	if (pLastSaved != nullptr)
	{
		const CClip& lastClip{ *pLastSaved };
		int Id = lastClip.m_id;

		CLogger::Log(CStringUtil::Format(_T("SaveCopyclips After AddToDb, Id: %d Before OnCopyCopyCompleted"), Id));

		m_clipboard.OnCopyCompleted(Id, count, copyReason);

		CLogger::Log(CStringUtil::Format(_T("SaveCopyclips After AddToDb, Id: %d After OnCopyCopyCompleted"), Id));

		if (lastClip.m_copyReason == CopyReasonEnum::COPY_TO_GROUP &&
			m_settings.GetShowMsgWndOnCopyToGroup())
		{
			CString groupName;
			try
			{
				CppSQLite3Query q = m_database.execQueryEx(_T("SELECT mText FROM Main WHERE lID = %d"), lastClip.m_parentId);
				if (q.eof() == false)
				{
					groupName = q.getStringField(0);
				}
			}
			catch (CppSQLite3Exception& e)
			{
				// upstream let this escape the worker thread's event handler
				CErrorReport::Show(CStringUtil::Format(_T("Reading the name of group %d for the copy message failed: %s"), lastClip.m_parentId, e.errorMessage()));
				return;
			}

			auto message{ std::make_unique<CString>() };
			message->Format(_T("Saved new clip \"%s\"\r\ndirectly to the group \"%s\""), lastClip.m_Desc.Left(35).GetString(), groupName.GetString());

			// posted to the handle: this thread must not touch the frame object, which can be gone
			// (NULL before the frame exists and after it is destroyed; PostMessage(NULL) would post to this thread)
			const HWND mainHwnd{ m_windows.MainHwnd() };
			if (mainHwnd != NULL && ::PostMessage(mainHwnd, CDittoMessage::ShowMsgWindow, reinterpret_cast<WPARAM>(message.get()), lastClip.m_parentId))
			{
				message.release(); // ownership: CMainFrame::OnShowMsgWindow retakes it in a std::unique_ptr
			}
		}
	}
}

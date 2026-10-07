#include "stdafx.h"
#include "MainFrmThread.h"
#include "DatabaseUtilities.h"
#include "Options.h"
#include "Misc.h"
#include "cp_main.h"

CMainFrmThread::CMainFrmThread(void)
{
	m_threadName = "CMainFrmThread";
    for(int eventEnum = 0; eventEnum < ECMAINFRMTHREADEVENTS_COUNT; eventEnum++)
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

void CMainFrmThread::OnEvent(int eventId, void * /*param*/)
{
    switch((eCMainFrmThreadEvents)eventId)
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
	double idle = IdleSeconds();

	if (idle < CGetSetOptions::ReadRandomFileIdleMin())
	{
		CString dbFile = CGetSetOptions::GetDBPath();
		__int64 dbSize = FileSize(dbFile);

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
    RemoveOldEntries(true);
}

void CMainFrmThread::OnRemoveTempFiles()
{
	DeleteDittoTempFiles(TRUE);
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
	CopyReasonEnum::CopyReason copyReason{CopyReasonEnum::COPY_TO_UNKOWN};
	if(!localClips.IsEmpty())
	{
		copyReason = localClips.Last().m_copyReason;
	}

	CLogger::Log(_T("SaveCopyClips Before AddToDb"));

	int count = localClips.AddToDB(true);

	CLogger::Log(StrF(_T("SaveCopyclips After AddToDb, Count: %d"), count));

	if(count > 0)
	{
		int Id = localClips.Last().m_id;

		CLogger::Log(StrF(_T("SaveCopyclips After AddToDb, Id: %d Before OnCopyCopyCompleted"), Id));

		theApp.OnCopyCompleted(Id, count, copyReason);

		CLogger::Log(StrF(_T("SaveCopyclips After AddToDb, Id: %d After OnCopyCopyCompleted"), Id));

		const CClip& lastClip{localClips.Last()};
		if (lastClip.m_copyReason == CopyReasonEnum::COPY_TO_GROUP &&
			CGetSetOptions::GetShowMsgWndOnCopyToGroup())
		{
			CString groupName;
			CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT mText FROM Main WHERE lID = %d"), lastClip.m_parentId);
			if (q.eof() == false)
			{
				groupName = q.getStringField(0);
			}

			auto message{std::make_unique<CString>()};
			message->Format(_T("Saved new clip \"%s\"\r\ndirectly to the group \"%s\""), lastClip.m_Desc.Left(35).GetString(), groupName.GetString());

			if (theApp.m_pMainFrame->PostMessageW(CDittoMessage::ShowMsgWindow, reinterpret_cast<WPARAM>(message.get()), lastClip.m_parentId))
			{
				message.release(); // ownership: CMainFrame::OnShowMsgWindow retakes it in a std::unique_ptr
			}
		}
	}
}
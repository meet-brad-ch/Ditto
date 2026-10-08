#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"
#include "ErrorReport.h"

long CClipDatabase::NewGroupID(int parentID, CString text)
{
	long lID{};
	CTime time{};
	time = CTime::GetCurrentTime();

	try
	{
		if(text.IsEmpty())
			text = time.Format("NewGroup %y/%m/%d %H:%M:%S");

		// bound values: the name is stored as typed (no quote doubling) and the time keeps 64 bits
		CppSQLite3Statement insert = theApp.m_db.compileStatement(
			_T("insert into Main (lDate, mText, lDontAutoDelete, bIsGroup, lParentID, stickyClipOrder, stickyClipGroupOrder) values(?, ?, ?, 1, ?, -(2147483647), -(2147483647));"));
		insert.bindInt64(1, time.GetTime());
		insert.bind(2, text);
		insert.bindInt64(3, time.GetTime());
		insert.bind(4, parentID);

		lID = (long)theApp.m_db.InsertReturningId(insert);
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Creating the group %s failed: %s"), text.GetString(), e.errorMessage()));
		return 0;
	}

	return lID;
}

BOOL CClipDatabase::DeleteAllIDs()
{
	try
	{
		theApp.m_db.execDML(_T("DELETE FROM Data;"));
		theApp.m_db.execDML(_T("DELETE FROM Main;"));
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Deleting all clips failed: %s"), e.errorMessage()));
		return FALSE;
	}

	return TRUE;
}

BOOL CClipDatabase::DeleteFormats(CGetSetOptions& settings, int parentID, ARRAY& formatIDs)
{
	if(formatIDs.GetSize() <= 0)
		return TRUE;

	try
	{
		//Delete the requested data formats
		const INT_PTR count{ formatIDs.GetSize() };
		for(int i{}; i < count; i++)
		{
			theApp.m_db.execDMLEx(_T("DELETE FROM Data WHERE lID = %d;"), formatIDs[i]);
		}

		CClip clip(settings);
		if(clip.LoadFormats(parentID))
		{
			const DWORD CRC{ clip.GenerateCRC() };

			//Update the main table with new size
			theApp.m_db.execDMLEx(_T("UPDATE Main SET CRC = %d WHERE lID = %d"), CRC, parentID);
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Deleting the selected formats of clip %d failed: %s"), parentID, e.errorMessage()));
		return FALSE;
	}

	return TRUE;
}

CString CClipDatabase::FolderPath(int folderId)
{
	CString folder{ _T("") };
	if (folderId > 0)
	{
		try
		{
			CStringArray arr;
			for (int i{}; i < 100; i++)
			{
				CppSQLite3Query parent = theApp.m_db.execQueryEx(_T("SELECT lID, mText, lParentID FROM Main WHERE lID = %d"), folderId);
				if (parent.eof() == false)
				{
					arr.Add(parent.getStringField(_T("mText")));
					folderId = parent.getIntField(_T("lParentID"));
				}
				else
				{
					break;
				}
			}

			folder = _T("Group Path: \\");
			for (INT_PTR folderPos{ arr.GetCount() - 1 }; folderPos >= 0; folderPos--)
			{
				folder += _T("\\");
				folder += arr[folderPos];
			}
		}
		catch (CppSQLite3Exception& e)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Reading the group path of group %d failed: %s"), folderId, e.errorMessage()));
			return _T("");
		}
	}

	return folder;
}

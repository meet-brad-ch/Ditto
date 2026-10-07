#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"
#include <algorithm>
#include <array>

void CTempFileCleaner::DeleteDittoTempFiles(BOOL checkFileLastAccess)
{
	CString csDir{ CGetSetOptions::GetPath(CGetSetOptions::PathRemoteFiles) };
	if (CFileSystem::FileExists(csDir))
	{
		DeleteFolderFiles(csDir, checkFileLastAccess, CTimeSpan(0, 1, 0, 0));
	}

	csDir = CGetSetOptions::GetPath(CGetSetOptions::PathDragFiles);
	if (CFileSystem::FileExists(csDir))
	{
		DeleteFolderFiles(csDir, checkFileLastAccess, CTimeSpan(0, 1, 0, 0));
	}

	csDir = CGetSetOptions::GetPath(CGetSetOptions::PathClipDiff);
	if (CFileSystem::FileExists(csDir))
	{
		DeleteFolderFiles(csDir, checkFileLastAccess, CTimeSpan(0, 1, 0, 0));
	}
}

void CTempFileCleaner::DeleteFolderFiles(CString csDir, BOOL checkFileLastAccess, CTimeSpan lastAccessOffset)
{
	// only Ditto's own temp folders are emptied
	static constexpr std::array<const TCHAR*, 4> tempFolderMarkers{ _T("\\ReceivedFiles\\"), _T("\\DragFiles\\"), _T("ClipCompare"), _T("EditClips") };
	if (std::none_of(tempFolderMarkers.begin(), tempFolderMarkers.end(), [&csDir](const TCHAR* marker) { return csDir.Find(marker) != -1; }))
		return;

	CLogger::Log(CStringUtil::Format(_T("Deleting files in Folder %s Check Last Access %d"), csDir.GetString(), checkFileLastAccess));

	CFolderPath::AddTrailingSlash(csDir);

	CTime ctOld{ CTime::GetCurrentTime() };
	CTime ctFile{};
	ctOld -= lastAccessOffset;

	CFileFind Find;

	CString csFindString{};
	csFindString.Format(_T("%s*.*"), csDir.GetString());

	BOOL bFound{ Find.FindFile(csFindString) };
	while(bFound)
	{
		bFound = Find.FindNextFile();

		if(Find.IsDots())
			continue;

		if(checkFileLastAccess &&
			Find.GetLastAccessTime(ctFile))
		{
			//Delete the remote copied file if it hasn't been used for the last day
			if(ctFile < ctOld)
			{
				CLogger::Log(CStringUtil::Format(_T("Deleting temp file %s"), Find.GetFilePath().GetString()));
				DeleteFile(Find.GetFilePath());
			}
		}
		else
		{
			CLogger::Log(CStringUtil::Format(_T("Deleting temp file %s"), Find.GetFilePath().GetString()));
			DeleteFile(Find.GetFilePath());
		}
	}
}

#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"
#include "FileDialogPath.h"

BOOL CDatabaseBackupPrompt::RestoreDbPrompt(CGetSetOptions& settings, HWND hwnd)
{
	BOOL ret{ false };

	OPENFILENAME ofn{};
	TCHAR szFile[400]{};
	TCHAR szDir[400]{};

	memset(&szFile, 0, sizeof(szFile));
	memset(szDir, 0, sizeof(szDir));
	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = _T("Ditto database backups (.zdb)\0*.zdb\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	//ofn.lpstrInitialDir = szDir;
	ofn.lpstrDefExt = _T("zdb");
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	if (GetOpenFileName(&ofn))
	{
		CWaitCursor wait;

		CString dbPath{ settings.GetDBPath() };
		CString backupPath(CFileDialogPath::From(ofn));
		ret = CDatabaseBackupService::RestoreDB(settings, backupPath);
	}

	return ret;
}

BOOL CDatabaseBackupPrompt::BackupDbPrompt(CGetSetOptions& settings, HWND hwnd)
{
	BOOL ret{ FALSE };

	OPENFILENAME ofn{};
	TCHAR szFile[400]{};
	TCHAR szDir[400]{};

	memset(&szFile, 0, sizeof(szFile));
	memset(szDir, 0, sizeof(szDir));
	memset(&ofn, 0, sizeof(ofn));

	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hwnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = _countof(szFile);
	ofn.lpstrFilter = _T("Ditto database backups (.zdb)\0*.zdb\0\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrDefExt = _T("zdb");
	// a save dialog: the file may be new (no OFN_FILEMUSTEXIST)
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	if (GetSaveFileName(&ofn))
	{
		CWaitCursor wait;

		CString dbPath{ settings.GetDBPath() };
		CString backupPath(CFileDialogPath::From(ofn));
		ret = CDatabaseBackupService::BackupDB(dbPath, backupPath);
	}

	return ret;
}

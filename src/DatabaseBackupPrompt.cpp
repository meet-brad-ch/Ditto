#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"
#include "FileDialogPath.h"

CDatabaseBackupPrompt::CDatabaseBackupPrompt(CGetSetOptions& settings, CMultiLanguage& language, CDittoDb& database, CAppState& state, CAppWindows& windows) :
	m_settings(settings),
	m_backupService(settings, language, database, state, windows)
{
}

BOOL CDatabaseBackupPrompt::RestoreDbPrompt(HWND hwnd)
{
	BOOL ret{ false };

	OPENFILENAME ofn{};
	TCHAR szFile[400]{};

	memset(&szFile, 0, sizeof(szFile));
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
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_NOCHANGEDIR;

	if (GetOpenFileName(&ofn))
	{
		CWaitCursor wait;

		CString backupPath(CFileDialogPath::From(ofn));
		ret = m_backupService.RestoreDB(backupPath);
	}

	return ret;
}

BOOL CDatabaseBackupPrompt::BackupDbPrompt(HWND hwnd)
{
	BOOL ret{ FALSE };

	OPENFILENAME ofn{};
	TCHAR szFile[400]{};

	memset(&szFile, 0, sizeof(szFile));
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

		CString dbPath{ m_settings.GetDBPath() };
		CString backupPath(CFileDialogPath::From(ofn));
		ret = m_backupService.BackupDB(dbPath, backupPath);
	}

	return ret;
}

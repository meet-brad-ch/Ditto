#pragma once

#include "DatabaseUtilities.h"

class CGetSetOptions;
class CMultiLanguage;
class CDittoDb;
class CAppState;
class CAppWindows;

/** @brief The file dialogs that back up and restore the database (.zdb files). */
class CDatabaseBackupPrompt
{
public:
	/**
	 * @brief Creates the prompts.
	 * @param settings The application's settings (the database path; passed on to the backup
	 * service); must outlive this object.
	 * @param language The UI texts (passed on to the backup service); must outlive this object.
	 * @param database The application's database connection (passed on to the backup service); must
	 * outlive this object.
	 * @param state The application state (passed on to the backup service); must outlive this object.
	 * @param windows The application's windows (passed on to the backup service); must outlive this
	 * object.
	 */
	CDatabaseBackupPrompt(CGetSetOptions& settings, CMultiLanguage& language, CDittoDb& database, CAppState& state, CAppWindows& windows);

	/**
	 * @brief Asks for a backup file (save dialog) and backs the database up to it.
	 * @param hwnd The dialog's owner window.
	 * @return TRUE when the backup was written; FALSE when cancelled or failed.
	 */
	BOOL BackupDbPrompt(HWND hwnd);

	/**
	 * @brief Asks for a backup file (open dialog) and restores the database from it.
	 * @param hwnd The dialog's owner window.
	 * @return TRUE when the database was restored; FALSE when cancelled or failed.
	 */
	BOOL RestoreDbPrompt(HWND hwnd);

private:
	/** @brief The application's settings (not owned). */
	CGetSetOptions& m_settings;
	/** @brief Backs up and restores the database. */
	CDatabaseBackupService m_backupService;
};

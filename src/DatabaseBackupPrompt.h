#pragma once

class CGetSetOptions;

/** @brief The file dialogs that back up and restore the database (.zdb files). */
class CDatabaseBackupPrompt
{
public:
	/**
	 * @brief Asks for a backup file (save dialog) and backs the database up to it.
	 * @param settings The application's settings (the database path).
	 * @param hwnd The dialog's owner window.
	 * @return TRUE when the backup was written; FALSE when cancelled or failed.
	 */
	static BOOL BackupDbPrompt(CGetSetOptions& settings, HWND hwnd);

	/**
	 * @brief Asks for a backup file (open dialog) and restores the database from it.
	 * @param settings The application's settings (passed on to RestoreDB).
	 * @param hwnd The dialog's owner window.
	 * @return TRUE when the database was restored; FALSE when cancelled or failed.
	 */
	static BOOL RestoreDbPrompt(CGetSetOptions& settings, HWND hwnd);
};

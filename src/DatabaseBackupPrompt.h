#pragma once

/** @brief The file dialogs that back up and restore the database (.zdb files). */
class CDatabaseBackupPrompt
{
public:
	/**
	 * @brief Asks for a backup file (save dialog) and backs the database up to it.
	 * @param hwnd The dialog's owner window.
	 * @return TRUE when the backup was written; FALSE when cancelled or failed.
	 */
	static BOOL BackupDbPrompt(HWND hwnd);

	/**
	 * @brief Asks for a backup file (open dialog) and restores the database from it.
	 * @param hwnd The dialog's owner window.
	 * @return TRUE when the database was restored; FALSE when cancelled or failed.
	 */
	static BOOL RestoreDbPrompt(HWND hwnd);
};

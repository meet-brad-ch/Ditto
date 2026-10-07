// DatabaseUtilites.h: interface for the CDatabaseUtilites class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_)
#define AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "DittoPopupWindow.h"
#include "sqlite/CppSQLite3.h"

/**
 * @brief Names, creates, opens and maintains the clip database file.
 */
class CDatabaseManager
{
public:
	/**
	 * @brief The configured database path.
	 * @return CGetSetOptions::GetDBPath().
	 */
	static CString GetDBName();
	/**
	 * @brief A database file name in the default location that does not exist yet (Ditto.db,
	 * Ditto_1.db, ...), for creating a new database.
	 * @return the full path of the free file name.
	 */
	static CString GetDefaultDBName();
	/**
	 * @brief Opens a database as the application's database (theApp.m_db), with the ICU extension
	 * and the configured busy timeout; also sets theApp.m_databaseOnNetworkShare.
	 * @param dbPath the database path.
	 * @return TRUE if it is open; FALSE (after showing the error) if opening failed.
	 */
	static BOOL OpenDatabase(CString dbPath);
	/**
	 * @brief Tells whether the application's database is open.
	 * @return theApp.m_db.IsDatabaseOpen().
	 */
	static BOOL IsDatabaseOpen();
	/**
	 * @brief Creates a new, empty database with Ditto's current schema.
	 * @param csFile the path of the new database file.
	 * @return TRUE on success; FALSE (after showing the error) if a statement failed.
	 */
	static BOOL CreateDB(CString csFile);
	/**
	 * @brief Former DAO compaction; does nothing now.
	 * @return TRUE.
	 */
	static BOOL CompactDatabase();
	/**
	 * @brief Former DAO repair; does nothing now.
	 * @return TRUE.
	 */
	static BOOL RepairDatabase();
	/**
	 * @brief Creates the directory part of a file path when it does not exist (one level only).
	 * @param csPath the file path.
	 * @return TRUE if the directory exists or was created, else FALSE.
	 */
	static BOOL EnsureDirectory(CString csPath);
	/**
	 * @brief Renumbers the sticky clip order of a level and, recursively, of every group in it.
	 * @param parentID the group whose clips are renumbered; -1 for the top level.
	 * @param db the open database.
	 */
	static void ReOrderStickyClips(int parentID, CppSQLite3DB& db);
};

/**
 * @brief Deletes the clips the retention options no longer keep (max entries, expiry, unused clips).
 */
class CClipRetentionPolicy
{
public:
	/**
	 * @brief Deletes the clips over the max entries and the expired clips (as configured), then
	 * empties the MainDeletes table in steps.
	 * @param checkIdleTime true to delete MainDeletes rows only while the computer is idle long enough.
	 * @return TRUE on success; FALSE (after showing the error) if a statement failed.
	 */
	static BOOL RemoveOldEntries(bool checkIdleTime);
	/**
	 * @brief Deletes every plain clip (no shortcut, not kept, not in a group, not sticky) and empties MainDeletes.
	 * @param fromAppWindow passed on to CClipIDs::DeleteIDs.
	 * @return TRUE.
	 */
	static BOOL DeleteNonUsedClips(bool fromAppWindow);

private:
	/**
	 * @brief RemoveOldEntries' max-entries step: deletes the plain clips (no shortcut, not kept, not in
	 * a group, not sticky) beyond the newest GetMaxEntries clips.
	 * @param db the open database.
	 */
	static void RemoveClipsOverMaxEntries(CppSQLite3DB& db);
	/**
	 * @brief RemoveOldEntries' expiry step: deletes the plain clips (no shortcut, not kept, not in a
	 * group, not sticky) last pasted more than GetExpiredEntries days ago.
	 * @param db the open database.
	 */
	static void RemoveExpiredClips(CppSQLite3DB& db);
};

/**
 * @brief Backs up (gzip) and restores the clip database, and makes numbered file copies.
 */
class CDatabaseBackupService
{
public:
	/**
	 * @brief Copies a file to the first free name path.001 ... path.050.
	 * @param csPath the file to copy.
	 * @return TRUE when a copy was made; FALSE after 50 tries.
	 */
	static BOOL CreateBackup(CString csPath);
	/**
	 * @brief Writes a gzip-compressed copy of the database, showing the progress in a popup.
	 * @param dbPath the database path.
	 * @param backupPath the backup file path.
	 * @return TRUE on success; FALSE (after showing the error) on failure.
	 */
	static BOOL BackupDB(CString dbPath, CString backupPath);
	/**
	 * @brief Unpacks a backup next to the current database, checks it, makes it the configured
	 * database, opens it and refreshes the view.
	 * @param backupPath the backup file path.
	 * @return TRUE on success; FALSE (after showing the error) on failure.
	 */
	static BOOL RestoreDB(CString backupPath);
};

namespace nsPath { class CPath; }

/**
 * @brief Finds, creates or replaces the clip database at startup.
 */
class DatabaseLocator
{
public:
	/**
	 * @brief Makes sure a usable database exists and opens it.
	 * @param csDBPath the configured database path; empty for the default location.
	 * @return TRUE if a database was found or created and opened; FALSE if it is on a network
	 * share or another drive than C: and missing, or could not be created or opened.
	 */
	static BOOL CheckDBExists(CString csDBPath);
	/**
	 * @brief Tells whether a database path is on a network share or on a drive other than C:.
	 * @param path the database path.
	 * @return true for an rtServerShare root, or a drive root/current-dir root with a letter other than C.
	 */
	static bool IsNetworkShareOrNonCDrive(nsPath::CPath& path);

private:
	/**
	 * @brief CheckDBExists' step for a missing file: creates it there, else at a new default path.
	 * @param csDBPath in: the missing database path; out: the path of the created database.
	 * @return the result of the last CreateDB call.
	 */
	static BOOL CreateMissingDB(CString& csDBPath);
	/**
	 * @brief CheckDBExists' step for an existing file: checks and upgrades it; a bad file is renamed
	 * to *_BAD.* (after telling the user) and a new database is created at a new default path.
	 * @param csDBPath in: the existing database path; out: the path of the database to open.
	 * @return TRUE for a valid database, else the result of CreateDB.
	 */
	static BOOL CheckExistingDB(CString& csDBPath);
};

/**
 * @brief Checks the clip database schema and upgrades older versions.
 */
class DatabaseSchemaUpgrader
{
public:
	/**
	 * @brief Checks that the database has Ditto's tables and runs every schema upgrade step.
	 * @param csPath the database path.
	 * @param bUpgrade not used.
	 * @return FALSE (after showing the error) if a required table is missing or an upgrade fails.
	 */
	static BOOL ValidDB(CString csPath, BOOL bUpgrade = TRUE);

private:
	/**
	 * @brief Queries the Main, Data and Types tables; throws if one is missing or lacks a column.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when a table or column is missing.
	 */
	static void CheckRequiredTables(CppSQLite3DB& db);
	/**
	 * @brief Drops the old delete_data_trigger; a failure is ignored.
	 * @param db the open database.
	 */
	static void DropDeleteDataTrigger(CppSQLite3DB& db);
	/**
	 * @brief Drops the old delete_copy_buffer_trigger; a failure is ignored.
	 * @param db the open database.
	 */
	static void DropCopyBufferTrigger(CppSQLite3DB& db);
	/**
	 * @brief Creates the delete_data_trigger that records deleted clips in MainDeletes; a failure is ignored.
	 * @param db the open database.
	 */
	static void CreateDeleteDataTrigger(CppSQLite3DB& db);
	/**
	 * @brief Creates the CopyBuffers table when it is missing.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when the table cannot be created.
	 */
	static void AddCopyBuffersTable(CppSQLite3DB& db);
	/**
	 * @brief Creates the MainDeletes table and its trigger when the table is missing.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when the table or trigger cannot be created.
	 */
	static void AddMainDeletesTable(CppSQLite3DB& db);
	/**
	 * @brief Creates the Main_ParentId, Main_IsGroup and Main_ShortCut indexes; stops at the
	 * first failure, which is ignored.
	 * @param db the open database.
	 */
	static void CreateMainIndexes(CppSQLite3DB& db);
	/**
	 * @brief Adds and fills clipOrder/clipGroupOrder (with their indexes) when they are missing.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when a step of the upgrade fails.
	 */
	static void AddClipOrderColumns(CppSQLite3DB& db);
	/**
	 * @brief Adds the globalShortCut column when it is missing.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when the column cannot be added.
	 */
	static void AddGlobalShortCutColumn(CppSQLite3DB& db);
	/**
	 * @brief Adds and fills the lastPasteDate column when it is missing (unset dates get the current time).
	 * @param db the open database.
	 * @throws CppSQLite3Exception when a step of the upgrade fails.
	 */
	static void AddLastPasteDateColumn(CppSQLite3DB& db);
	/**
	 * @brief Adds the stickyClipOrder/stickyClipGroupOrder columns when they are missing.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when a column cannot be added.
	 */
	static void AddStickyOrderColumns(CppSQLite3DB& db);
	/**
	 * @brief ValidDB's sticky-order step: when the Main_NoGroup index is missing, sets the unset sticky
	 * orders and creates the sticky-order indexes; a failed step is ignored, as the index may exist already.
	 * @param db the open database.
	 */
	static void UpgradeStickyOrderIndexes(CppSQLite3DB& db);
	/**
	 * @brief Adds the MoveToGroupShortCut/GlobalMoveToGroupShortCut columns when one is missing.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when a column cannot be added.
	 */
	static void AddMoveToGroupColumns(CppSQLite3DB& db);
	/**
	 * @brief Drops the replaced indexes and creates the current Main indexes if they do not exist.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when an index statement fails.
	 */
	static void CreateCurrentIndexes(CppSQLite3DB& db);
};

//BOOL CopyDownDatabase();
//BOOL CopyUpDatabase();

#endif // !defined(AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_)

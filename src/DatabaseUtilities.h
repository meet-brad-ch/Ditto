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

class CGetSetOptions;
class CDittoDb;
class CAppState;
class CAppWindows;
class CIdleTime;
class CMultiLanguage;

/**
 * @brief Names, creates, opens and maintains the clip database file.
 */
class CDatabaseManager
{
public:
	/**
	 * @brief The configured database path.
	 * @param settings the application's settings.
	 * @return settings.GetDBPath().
	 */
	static CString GetDBName(CGetSetOptions& settings);
	/**
	 * @brief A database file name in the default location that does not exist yet (Ditto.db,
	 * Ditto_1.db, ...), for creating a new database.
	 * @param settings the application's settings (the default database directory).
	 * @return the full path of the free file name.
	 */
	static CString GetDefaultDBName(CGetSetOptions& settings);
	/**
	 * @brief Opens a database as the application's database (closing the one open before), with the
	 * ICU extension and the configured busy timeout; also sets CAppState::m_databaseOnNetworkShare.
	 * @param settings the application's settings (the busy timeout).
	 * @param database the application's database connection.
	 * @param state the application state (whether the database lies on a network share).
	 * @param dbPath the database path.
	 * @return TRUE if it is open; FALSE (after showing the error) if opening failed.
	 */
	static BOOL OpenDatabase(CGetSetOptions& settings, CDittoDb& database, CAppState& state, CString dbPath);
	/**
	 * @brief Tells whether the application's database is open.
	 * @param database the application's database connection.
	 * @return database.IsDatabaseOpen().
	 */
	static BOOL IsDatabaseOpen(CDittoDb& database);
	/**
	 * @brief Creates a new, empty database with Ditto's current schema.
	 * @param csFile the path of the new database file.
	 * @return TRUE on success; FALSE (after showing the error) if a statement failed.
	 */
	static BOOL CreateDB(CString csFile);
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
	 * @param settings the application's settings (database path and retention options).
	 * @param idleTime the user's idle time (checked before each MainDeletes row).
	 * @param windows the application's windows (told of each deleted clip, CClipIDs::DeleteIDs).
	 * @param checkIdleTime true to delete MainDeletes rows only while the computer is idle long enough.
	 * @return TRUE on success; FALSE (after showing the error) if a statement failed.
	 */
	static BOOL RemoveOldEntries(CGetSetOptions& settings, CIdleTime& idleTime, CAppWindows& windows, bool checkIdleTime);
	/**
	 * @brief Deletes every plain clip (no shortcut, not kept, not in a group, not sticky) and empties MainDeletes.
	 * @param database the application's database connection.
	 * @param windows the application's windows (passed on to CClipIDs::DeleteIDs).
	 * @param fromAppWindow passed on to CClipIDs::DeleteIDs.
	 * @return TRUE on success; FALSE (after showing the error) if a statement failed.
	 */
	static BOOL DeleteNonUsedClips(CDittoDb& database, CAppWindows& windows, bool fromAppWindow);

private:
	/**
	 * @brief RemoveOldEntries' max-entries step: deletes the plain clips (no shortcut, not kept, not in
	 * a group, not sticky) beyond the newest GetMaxEntries clips.
	 * @param settings the application's settings (GetMaxEntries).
	 * @param windows the application's windows (told of each deleted clip).
	 * @param db the open database.
	 * @return false when the delete failed (CClipIDs::DeleteIDs showed the error).
	 * @throws CppSQLite3Exception when the query fails.
	 */
	static bool RemoveClipsOverMaxEntries(CGetSetOptions& settings, CAppWindows& windows, CDittoDb& db);
	/**
	 * @brief RemoveOldEntries' expiry step: deletes the plain clips (no shortcut, not kept, not in a
	 * group, not sticky) last pasted more than GetExpiredEntries days ago.
	 * @param settings the application's settings (GetExpiredEntries).
	 * @param windows the application's windows (told of each deleted clip).
	 * @param db the open database.
	 * @return false when the delete failed (CClipIDs::DeleteIDs showed the error).
	 * @throws CppSQLite3Exception when the query fails.
	 */
	static bool RemoveExpiredClips(CGetSetOptions& settings, CAppWindows& windows, CDittoDb& db);
};

/**
 * @brief Backs up (gzip) and restores the clip database, and makes numbered file copies.
 */
class CDatabaseBackupService
{
public:
	/**
	 * @brief Writes a gzip-compressed copy of the database, showing the progress in a popup.
	 * @param language the UI texts (the progress text).
	 * @param dbPath the database path.
	 * @param backupPath the backup file path.
	 * @return TRUE on success; FALSE (after showing the error) on failure.
	 */
	static BOOL BackupDB(CMultiLanguage& language, CString dbPath, CString backupPath);
	/**
	 * @brief Unpacks a backup next to the current database, checks it, makes it the configured
	 * database, opens it and refreshes the view.
	 * @param settings the application's settings (temp folder, default directory, database path).
	 * @param language the UI texts (the progress text).
	 * @param database the application's database connection (opened on the restored file).
	 * @param state the application state (passed on to CDatabaseManager::OpenDatabase).
	 * @param windows the application's windows (the view refreshed after the restore).
	 * @param backupPath the backup file path.
	 * @return TRUE on success; FALSE (after showing the error) on failure, also when the restored
	 * database cannot be opened.
	 */
	static BOOL RestoreDB(CGetSetOptions& settings, CMultiLanguage& language, CDittoDb& database, CAppState& state, CAppWindows& windows, CString backupPath);
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
	 * @param settings the application's settings (the database path is stored there when it changes).
	 * @param language the UI texts (the message about a bad database file).
	 * @param database the application's database connection (opened on the database found).
	 * @param state the application state (passed on to CDatabaseManager::OpenDatabase).
	 * @param csDBPath the configured database path; empty for the default location.
	 * @return TRUE if a database was found or created and opened; FALSE if it is on a network
	 * share or another drive than C: and missing, if it is in use or cannot be read at the moment
	 * (left unchanged and logged; the caller tries again later), or could not be created or opened.
	 */
	static BOOL CheckDBExists(CGetSetOptions& settings, CMultiLanguage& language, CDittoDb& database, CAppState& state, CString csDBPath);
	/**
	 * @brief Tells whether a database path is on a network share or on a drive other than C:.
	 * @param path the database path.
	 * @return true for an rtServerShare root, or a drive root/current-dir root with a letter other than C.
	 */
	static bool IsNetworkShareOrNonCDrive(nsPath::CPath& path);

private:
	/**
	 * @brief CheckDBExists' step for a missing file: creates it there, else at a new default path.
	 * @param settings the application's settings (default directory; the new path is stored there).
	 * @param csDBPath in: the missing database path; out: the path of the created database.
	 * @return the result of the last CreateDB call.
	 */
	static BOOL CreateMissingDB(CGetSetOptions& settings, CString& csDBPath);
	/**
	 * @brief CheckDBExists' step for an existing file: checks and upgrades it. A damaged file is
	 * renamed to name_BAD.ext (after telling the user) and a new database is created at a new
	 * default path. A file that is only in use or not readable now
	 * (CppSQLite3Exception::isUnavailable) is left as it is.
	 * @param settings the application's settings (default directory; the new path is stored there).
	 * @param language the UI texts (the message about the bad file).
	 * @param csDBPath in: the existing database path; out: the path of the database to open.
	 * @return TRUE for a valid database; FALSE for a database in use, or when the damaged file
	 * cannot be renamed (shown); else the result of CreateDB.
	 */
	static BOOL CheckExistingDB(CGetSetOptions& settings, CMultiLanguage& language, CString& csDBPath);
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
	/**
	 * @brief ValidDB's work without the report: opens the database, checks Ditto's tables and runs
	 * every schema upgrade step.
	 * @param csPath the database path.
	 * @throws CppSQLite3Exception when the database cannot be opened, a required table is missing or
	 * an upgrade fails; isUnavailable() tells a locked or unreadable database from a damaged one.
	 */
	static void CheckAndUpgrade(CString csPath);

private:
	/**
	 * @brief Queries the Main, Data and Types tables; throws if one is missing or lacks a column.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when a table or column is missing.
	 */
	static void CheckRequiredTables(CppSQLite3DB& db);
	/**
	 * @brief Drops the old delete_data_trigger if it exists.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when the drop fails (e.g. the database is locked).
	 */
	static void DropDeleteDataTrigger(CppSQLite3DB& db);
	/**
	 * @brief Drops the old delete_copy_buffer_trigger if it exists.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when the drop fails (e.g. the database is locked).
	 */
	static void DropCopyBufferTrigger(CppSQLite3DB& db);
	/**
	 * @brief Creates the delete_data_trigger that records deleted clips in MainDeletes, if it does
	 * not exist.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when the trigger cannot be created (e.g. the database is locked).
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
	 * @brief Creates the Main_ParentId and Main_IsGroup indexes if they do not exist.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when an index cannot be created (e.g. the database is locked).
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
	 * orders and creates the sticky-order indexes that do not exist.
	 * @param db the open database.
	 * @throws CppSQLite3Exception when a step fails (e.g. the database is locked).
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

#endif // !defined(AFX_DATABASEUTILITES_H__039F53EB_228F_4640_8009_3D2B1FF435D4__INCLUDED_)

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

#define DEFAULT_DB_NAME "Ditto.db"
#define ERROR_OPENING_DATABASE	2

BOOL CreateBackup(CString csPath);
CString GetDBName();
CString GetDefaultDBName();
BOOL OpenDatabase(CString csDB);
BOOL IsDatabaseOpen();

BOOL CheckDBExists(CString csDBPath);
BOOL ValidDB(CString csPath, BOOL bUpgrade=TRUE);
BOOL CreateDB(CString csPath);

BOOL CompactDatabase();
BOOL RepairDatabase();
BOOL RemoveOldEntries(bool checkIdleTime);
BOOL DeleteNonUsedClips(bool fromAppWindow);

BOOL EnsureDirectory(CString csPath);

BOOL BackupDB(CString dbPath, CString backupPath);
BOOL RestoreDB(CString backupPath);

void ReOrderStickyClips(int parentID, CppSQLite3DB &db);

namespace nsPath { class CPath; }

/**
 * @brief Finds, creates or replaces the clip database at startup (CheckDBExists forwards to it).
 */
class DatabaseLocator
{
public:
	/**
	 * @brief Makes sure a usable database exists and opens it (see CheckDBExists).
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
 * @brief Checks the clip database schema and upgrades older versions (ValidDB forwards to it).
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

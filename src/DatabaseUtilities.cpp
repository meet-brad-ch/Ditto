// DatabaseUtilites.cpp: implementation of the CDatabaseUtilites class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CP_Main.h"
#include "DatabaseUtilities.h"
#include "ProcessPaste.h"
#include <io.h>
#include "Path.h"
#include "..\Shared\TextConvert.h"
#include "DatabasePath.h"
#include "ErrorReport.h"
#include "GzipStream.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>
using namespace nsPath;

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////


BOOL CDatabaseBackupService::CreateBackup(CString csPath)
{
	CString csOriginal;
	int count = 0;
	// create a backup of the existing database
	do
	{
		count++;
		csOriginal = csPath + CStringUtil::Format(_T(".%03d"), count);
		// in case of some weird infinite loop
		if (count > 50)
		{
			ASSERT(0);
			return FALSE;
		}
	} while (!::CopyFile(csPath, csOriginal, TRUE));

	return TRUE;
}

CString CDatabaseManager::GetDBName(CGetSetOptions& settings)
{
	return settings.GetDBPath();
}

CString CDatabaseManager::GetDefaultDBName(CGetSetOptions& settings)
{
	CString csDefaultPath = settings.GetDefaultDBDirectory();

	CString csTempName = csDefaultPath + "Ditto.db";
	int i = 1;
	while (CFileSystem::FileExists(csTempName))
	{
		csTempName.Format(_T("%sDitto_%d.db"), csDefaultPath.GetString(), i);
		i++;
	}
	csDefaultPath = csTempName;

	return csDefaultPath;
}

BOOL DatabaseLocator::CheckDBExists(CGetSetOptions& settings, CMultiLanguage& language, CDittoDb& database, CAppState& state, CString csDBPath)
{
	// No path set (first run, or the settings were removed by an uninstall): open Ditto.db in the
	// default location, which may hold the existing history; it is created below only if missing
	if (csDBPath.IsEmpty())
	{
		const std::filesystem::path defaultDirectory{ settings.GetDefaultDBDirectory().GetString() };
		csDBPath = DittoCore::DatabasePath::Resolve({}, defaultDirectory).c_str();
		settings.SetDBPath(csDBPath);
	}

	CPath path(csDBPath);

	BOOL bRet = FALSE;
	if (CFileSystem::FileExists(csDBPath) == FALSE)
	{
		//if the database is on a shared drive, network share or anything other than C:\ than don't create a new db
		//Ditto will wait until that drive is available
		if (IsNetworkShareOrNonCDrive(path))
		{
			return FALSE;
		}

		bRet = CreateMissingDB(settings, csDBPath);
	}
	else
	{
		bRet = CheckExistingDB(settings, language, csDBPath);
	}

	if (bRet)
	{
		bRet = CDatabaseManager::OpenDatabase(settings, database, state, csDBPath);
	}

	return bRet;
}

bool DatabaseLocator::IsNetworkShareOrNonCDrive(CPath& path)
{
	int len = 0;
	auto rootType = path.GetRootType(&len);
	auto driveLetter = path.GetDriveLetter();

	return rootType == ERootType::rtServerShare ||
		((rootType == ERootType::rtDriveCur || rootType == rtDriveRoot) && driveLetter >= 'A' && driveLetter != 'C');
}

BOOL DatabaseLocator::CreateMissingDB(CGetSetOptions& settings, CString& csDBPath)
{
	//first try and create create a db at the same path that was selectd
	BOOL bRet = CDatabaseManager::CreateDB(csDBPath);

	//if that didn't work then go back to the default location
	if (CFileSystem::FileExists(csDBPath) == FALSE)
	{
		csDBPath = CDatabaseManager::GetDefaultDBName(settings);

		nsPath::CPath FullPath(csDBPath);
		CString csPath = FullPath.GetPath().GetStr();
		if (csPath.IsEmpty() == false && CFileSystem::FileExists(csDBPath) == FALSE)
		{
			CreateDirectory(csPath, NULL);
		}

		settings.SetDBPath(csDBPath);

		bRet = CDatabaseManager::CreateDB(csDBPath);
	}

	return bRet;
}

BOOL DatabaseLocator::CheckExistingDB(CGetSetOptions& settings, CMultiLanguage& language, CString& csDBPath)
{
	if (DatabaseSchemaUpgrader::ValidDB(csDBPath) != FALSE)
	{
		return TRUE;
	}

	//Db existed but was bad
	CString csMarkAsBad;

	csMarkAsBad = csDBPath;
	csMarkAsBad.Replace(_T("."), _T("_BAD."));

	CString csPath = CDatabaseManager::GetDefaultDBName(settings);

	CString cs;
	cs.Format(_T("%s \"%s\",\n")
		_T("%s \"%s\",\n")
		_T("%s,\n")
		_T("\"%s\""),
		language.GetString("Database_Format", "Unrecognized Database Format").GetString(),
		csDBPath.GetString(),
		language.GetString("File_Renamed", "the file will be renamed").GetString(),
		csMarkAsBad.GetString(),
		language.GetString("New_Database","and a new database will be created").GetString(),
		csPath.GetString());

	AfxMessageBox(cs);

	CFile::Rename(csDBPath, csMarkAsBad);

	csDBPath = csPath;

	BOOL bRet = CDatabaseManager::CreateDB(csDBPath);

	settings.SetDBPath(csDBPath);

	return bRet;
}

BOOL CDatabaseManager::IsDatabaseOpen(CDittoDb& database)
{
	return database.IsDatabaseOpen();
}

BOOL CDatabaseManager::OpenDatabase(CGetSetOptions& settings, CDittoDb& database, CAppState& state, CString dbPath)
{
	try
	{
		CPath path(dbPath);

		const bool onNetworkShareOrNonCDrive = DatabaseLocator::IsNetworkShareOrNonCDrive(path);

		state.m_databaseOnNetworkShare = false;
		if (onNetworkShareOrNonCDrive)
		{
			state.m_databaseOnNetworkShare = true;
		}


		database.close();
		database.open(dbPath);
		// ICU: Unicode case folding for LIKE, upper/lower and the regexp search
		database.loadExtension("ICU_Loader.dll", "sqlite3_icu_init");

		database.setBusyTimeout(settings.GetDbTimeout());

		return TRUE;
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Opening the clip database %s failed: %s"), dbPath.GetString(), e.errorMessage()));
		return FALSE;
	}
}

void CDatabaseManager::ReOrderStickyClips(int parentID, CppSQLite3DB& db)
{
	try
	{
		CLogger::Log(CStringUtil::Format(_T("Start of ReOrderStickyClips, ParentId %d"), parentID));

		//groups where created with 0 in these fields, fix them up if they are 0
		if (parentID == -1)
		{
			db.execDMLEx(_T("Update Main Set stickyClipOrder = -(2147483647) where bIsGroup = 1 AND stickyClipOrder = 0"));
			db.execDMLEx(_T("Update Main Set stickyClipGroupOrder = -(2147483647) where bIsGroup = 1 AND stickyClipGroupOrder = 0"));
		}

		CppSQLite3Query qGroup = db.execQueryEx(_T("SELECT lID, mText FROM Main WHERE bIsGroup = 1 AND lParentID = %d"), parentID);

		if (qGroup.eof() == false)
		{
			while (!qGroup.eof())
			{
				//Get all sticky clips at the top level or group
				CString sql = CStringUtil::Format(_T("SELECT lID FROM Main WHERE stickyClipOrder <> -(2147483647) AND lParentID = %d ORDER BY stickyClipOrder DESC"), parentID);
				if (parentID > -1)
				{
					sql = CStringUtil::Format(_T("SELECT lID FROM Main WHERE stickyClipGroupOrder <> -(2147483647) AND lParentID = %d ORDER BY stickyClipGroupOrder DESC"), parentID);
				}

				CppSQLite3Query qSticky = db.execQueryEx(sql);

				int order = 1;

				if (qSticky.eof() == false)
				{
					while (!qSticky.eof())
					{
						//set the new order
						if (parentID > -1)
						{
							db.execDMLEx(_T("Update Main Set stickyClipGroupOrder = %d where lID = %d"), order, qSticky.getIntField(_T("lID")));
						}
						else
						{
							db.execDMLEx(_T("Update Main Set stickyClipOrder = %d where lID = %d"), order, qSticky.getIntField(_T("lID")));
						}

						qSticky.nextRow();
						order--;
					}
				}

				ReOrderStickyClips(qGroup.getIntField(_T("lID")), db);

				qGroup.nextRow();
			}
		}

		CLogger::Log(CStringUtil::Format(_T("End of ReOrderStickyClips, ParentId %d"), parentID));
	}
	catch (CppSQLite3Exception& e)
	{
		// a recursive call stops only its own group; the parent level goes on with the next
		// group, as each group's sticky order is independent (void: no failure value to pass up)
		CErrorReport::Show(CStringUtil::Format(_T("Fixing the sticky clip order of group %d failed: %s"), parentID, e.errorMessage()));
		return;
	}
}

void DatabaseSchemaUpgrader::UpgradeStickyOrderIndexes(CppSQLite3DB& db)
{
	try
	{
		CppSQLite3Query q{db.execQuery(_T("PRAGMA index_info(Main_NoGroup);"))};
		int count{0};
		while (q.eof() == false)
		{
			count++;
			q.nextRow();
		}

		if (count == 0)
		{
			db.execDML(_T("Update Main set stickyClipOrder = -(2147483647) where stickyClipOrder IS NULL;"));
			db.execDML(_T("Update Main set stickyClipGroupOrder = -(2147483647) where stickyClipGroupOrder IS NULL;"));
			db.execDML(_T("Update Main set stickyClipOrder = -(2147483647) where stickyClipOrder = 0;"));
			db.execDML(_T("Update Main set stickyClipGroupOrder = -(2147483647) where stickyClipGroupOrder = 0;"));

			db.execDML(_T("CREATE INDEX Main_NoGroup ON Main(bIsGroup ASC, stickyClipOrder DESC, clipOrder DESC);"));
			db.execDML(_T("CREATE INDEX Main_InGroup ON Main(lParentId ASC, bIsGroup ASC, stickyClipGroupOrder DESC, clipGroupOrder DESC);"));
			db.execDML(_T("CREATE INDEX Data_ParentId_Format ON Data(lParentID COLLATE BINARY ASC, strClipBoardFormat COLLATE NOCASE ASC);"));
		}
	}
	catch (CppSQLite3Exception& e)
	{
		e.errorCode();
	}
}

BOOL DatabaseSchemaUpgrader::ValidDB(CString csPath, BOOL /*bUpgrade*/)
{
	try
	{
		CppSQLite3DB db;
		db.open(csPath);

		CheckRequiredTables(db);
		DropDeleteDataTrigger(db);
		DropCopyBufferTrigger(db);
		CreateDeleteDataTrigger(db);
		AddCopyBuffersTable(db);
		AddMainDeletesTable(db);
		CreateMainIndexes(db);
		AddClipOrderColumns(db);
		AddGlobalShortCutColumn(db);
		AddLastPasteDateColumn(db);
		AddStickyOrderColumns(db);

		UpgradeStickyOrderIndexes(db);

		AddMoveToGroupColumns(db);
		CreateCurrentIndexes(db);
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Checking and upgrading the clip database %s failed: %s"), csPath.GetString(), e.errorMessage()));
		return FALSE;
	}

		return TRUE;
}

void DatabaseSchemaUpgrader::CheckRequiredTables(CppSQLite3DB& db)
{
	db.execQuery(_T("SELECT lID, lDate, mText, lShortCut, lDontAutoDelete, ")
		_T("CRC, bIsGroup, lParentID, QuickPasteText ")
		_T("FROM Main"));

	db.execQuery(_T("SELECT lID, lParentID, strClipBoardFormat, ooData FROM Data"));

	db.execQuery(_T("SELECT lID, TypeText FROM Types"));
}

void DatabaseSchemaUpgrader::DropDeleteDataTrigger(CppSQLite3DB& db)
{
	try
	{
		db.execDML(_T("DROP TRIGGER delete_data_trigger"));
	}
	catch (CppSQLite3Exception& e)
	{
		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::DropCopyBufferTrigger(CppSQLite3DB& db)
{
	try
	{
		db.execDML(_T("DROP TRIGGER delete_copy_buffer_trigger"));
	}
	catch (CppSQLite3Exception& e)
	{
		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::CreateDeleteDataTrigger(CppSQLite3DB& db)
{
	//This was added later so try to add each time and catch the exception here
	try
	{
		db.execDML(_T("CREATE TRIGGER delete_data_trigger BEFORE DELETE ON Main FOR EACH ROW\n")
			_T("BEGIN\n")
			_T("INSERT INTO MainDeletes VALUES(old.lID, datetime('now'));\n")
			_T("END\n"));
	}
	catch (CppSQLite3Exception& e)
	{
		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::AddCopyBuffersTable(CppSQLite3DB& db)
{
	//This was added later so try to add each time and catch the exception here
	try
	{
		db.execQuery(_T("SELECT lID, lClipID, lCopyBuffer FROM CopyBuffers"));
	}
	catch (CppSQLite3Exception& e)
	{
		e.errorCode();

		db.execDML(_T("CREATE TABLE CopyBuffers(")
			_T("lID INTEGER PRIMARY KEY AUTOINCREMENT, ")
			_T("lClipID INTEGER,")
			_T("lCopyBuffer INTEGER)"));
	}
}

void DatabaseSchemaUpgrader::AddMainDeletesTable(CppSQLite3DB& db)
{
	//This was added later so try to add each time and catch the exception here
	try
	{
		db.execQuery(_T("SELECT clipId FROM MainDeletes"));
	}
	catch (CppSQLite3Exception& e)
	{
		e.errorCode();

		db.execDML(_T("CREATE TABLE MainDeletes(")
			_T("clipID INTEGER,")
			_T("modifiedDate)"));

		db.execDML(_T("CREATE TRIGGER MainDeletes_delete_data_trigger BEFORE DELETE ON MainDeletes FOR EACH ROW\n")
			_T("BEGIN\n")
			_T("DELETE FROM CopyBuffers WHERE lClipID = old.clipID;\n")
			_T("DELETE FROM Data WHERE lParentID = old.clipID;\n")
			_T("END\n"));
	}
}

void DatabaseSchemaUpgrader::CreateMainIndexes(CppSQLite3DB& db)
{
	try
	{
		db.execDML(_T("CREATE INDEX Main_ParentId on Main(lParentID DESC)"));
		db.execDML(_T("CREATE INDEX Main_IsGroup on Main(bIsGroup DESC)"));
		db.execDML(_T("CREATE INDEX Main_ShortCut on Main(lShortCut DESC)"));
	}
	catch (CppSQLite3Exception& e)
	{
		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::AddClipOrderColumns(CppSQLite3DB& db)
{
	try
	{
		db.execQuery(_T("SELECT clipOrder, clipGroupOrder FROM Main"));
	}
	catch (CppSQLite3Exception& e)
	{
		db.execDML(_T("ALTER TABLE Main ADD clipOrder REAL"));
		db.execDML(_T("ALTER TABLE Main ADD clipGroupOrder REAL"));

		db.execDML(_T("Update Main set clipOrder = lDate, clipGroupOrder = lDate"));

		db.execDML(_T("CREATE INDEX Main_ClipOrder on Main(clipOrder DESC)"));
		db.execDML(_T("CREATE INDEX Main_ClipGroupOrder on Main(clipGroupOrder DESC)"));

		db.execDML(_T("DROP INDEX Main_Date"));

		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::AddGlobalShortCutColumn(CppSQLite3DB& db)
{
	try
	{
		db.execQuery(_T("SELECT globalShortCut FROM Main"));
	}
	catch (CppSQLite3Exception& e)
	{
		db.execDML(_T("ALTER TABLE Main ADD globalShortCut INTEGER"));

		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::AddLastPasteDateColumn(CppSQLite3DB& db)
{
	try
	{
		db.execQuery(_T("SELECT lastPasteDate FROM Main"));
	}
	catch (CppSQLite3Exception& e)
	{
		db.execDML(_T("ALTER TABLE Main ADD lastPasteDate INTEGER"));
		db.execDML(_T("Update Main set lastPasteDate = lDate"));
		db.execDMLEx(_T("Update Main set lastPasteDate = %d where lastPasteDate <= 0"), (int)CTime::GetCurrentTime().GetTime());

		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::AddStickyOrderColumns(CppSQLite3DB& db)
{
	try
	{
		db.execQuery(_T("SELECT stickyClipOrder FROM Main"));
	}
	catch (CppSQLite3Exception& e)
	{
		db.execDML(_T("ALTER TABLE Main ADD stickyClipOrder REAL"));
		db.execDML(_T("ALTER TABLE Main ADD stickyClipGroupOrder REAL"));

		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::AddMoveToGroupColumns(CppSQLite3DB& db)
{
	try
	{
		db.execQuery(_T("SELECT MoveToGroupShortCut FROM Main"));
		db.execQuery(_T("SELECT GlobalMoveToGroupShortCut FROM Main"));
	}
	catch (CppSQLite3Exception& e)
	{
		db.execDML(_T("ALTER TABLE Main ADD MoveToGroupShortCut INTEGER"));
		db.execDML(_T("ALTER TABLE Main ADD GlobalMoveToGroupShortCut INTEGER"));

		e.errorCode();
	}
}

void DatabaseSchemaUpgrader::CreateCurrentIndexes(CppSQLite3DB& db)
{
	db.execDML(_T("DROP INDEX IF EXISTS Main_NoGroup"));
	db.execDML(_T("DROP INDEX IF EXISTS Main_InGroup"));
	db.execDML(_T("DROP INDEX IF EXISTS Main_ShortCut"));

	db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_TopLevelParentID ON Main(lParentId ASC, stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC);"));
	db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_TopLevel ON Main(stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC);"));
	db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_InGroup2 ON Main(lParentId ASC, stickyClipGroupOrder DESC, bIsGroup ASC, clipGroupOrder DESC);"));

	db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_ShortCut2 on Main(lShortCut DESC, globalShortCut DESC)"));
	db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_MoveToGroup on Main(MoveToGroupShortCut DESC, GlobalMoveToGroupShortCut DESC)"));
	db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_CRC on Main(CRC ASC)"));
}

BOOL CDatabaseBackupService::BackupDB(CMultiLanguage& language, CString dbPath, CString backupPath)
{
	CRect r = CMonitorGeometry::DefaultMonitorRect();
	CPopup status((r.right - 500), r.bottom - 100, ::GetForegroundWindow());

	CString msg = language.GetString("BackupDbMsg", "Backing up database");

	status.Show(CStringUtil::Format(_T("Ditto - %s - %s"), msg.GetString(), backupPath.GetString()));

	CLogger::Log(CStringUtil::Format(_T("Start backing up db, from: %s to %s"), dbPath.GetString(), backupPath.GetString()));

	try
	{
		std::ifstream in(dbPath.GetString(), std::ios::binary);
		if (!in)
		{
			throw std::runtime_error("the database cannot be opened for reading");
		}
		std::ofstream out(backupPath.GetString(), std::ios::binary | std::ios::trunc);
		if (!out)
		{
			throw std::runtime_error("the backup file cannot be created");
		}

		const std::uintmax_t fileSize = std::filesystem::file_size(dbPath.GetString());
		int percentageComplete{};
		DittoCore::GzipStream::Compress(in, out, [&](std::uint64_t bytesDone)
		{
			const int percent = fileSize == 0 ? 100 : static_cast<int>((bytesDone * 100) / fileSize);
			if (percent != percentageComplete)
			{
				percentageComplete = percent;
				status.Show(CStringUtil::Format(_T("Ditto - %02d%% %s - %s"), percentageComplete, msg.GetString(), backupPath.GetString()));
			}
		});

		out.close();
		if (!out)
		{
			throw std::runtime_error("the backup file could not be completed");
		}
	}
	catch (const std::exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Backing up the database to %s failed: %s"), backupPath.GetString(), CString(e.what()).GetString()));
		return FALSE;
	}

	CLogger::Log(CStringUtil::Format(_T("Done backing up db, to: %s"), backupPath.GetString()));
	return TRUE;
}

BOOL CDatabaseBackupService::RestoreDB(CGetSetOptions& settings, CMultiLanguage& language, CDittoDb& database, CAppState& state, CAppWindows& windows, CString backupPath)
{
	CRect r = CMonitorGeometry::DefaultMonitorRect();
	CPopup status((r.right - 500), r.bottom - 100, ::GetForegroundWindow());

	CString msg = language.GetString("RestoreDbMsg", "Restoring database");
	status.Show(CStringUtil::Format(_T("Ditto - %s - %s"), msg.GetString(), backupPath.GetString()));

	CLogger::Log(CStringUtil::Format(_T("Start restoring db, from: %s"), backupPath.GetString()));

	using namespace nsPath;
	CPath backupPathPath(backupPath);
	const CString tempPath = settings.GetPath(CGetSetOptions::PathRestoreTemp) + backupPathPath.GetName();

	try
	{
		{
			std::ifstream in(backupPath.GetString(), std::ios::binary);
			if (!in)
			{
				throw std::runtime_error("the backup cannot be opened for reading");
			}
			std::ofstream out(tempPath.GetString(), std::ios::binary | std::ios::trunc);
			if (!out)
			{
				throw std::runtime_error("the temporary file for the unpacked database cannot be created");
			}
			DittoCore::GzipStream::Uncompress(in, out);
			out.close();
			if (!out)
			{
				throw std::runtime_error("the unpacked database could not be completed");
			}
		}

		if (!DatabaseSchemaUpgrader::ValidDB(tempPath, true))
		{
			throw std::runtime_error("the unpacked database is not a valid Ditto database");
		}

		CPath defaultDbPathPath(CDatabaseManager::GetDefaultDBName(settings));
		const CString path(defaultDbPathPath.GetPath());
		backupPathPath.RenameExtension(_T("db"));
		CString newFullPath = path + backupPathPath.GetName();
		for (int i = 1; CFileSystem::FileExists(newFullPath); i++)
		{
			newFullPath.Format(_T("%s%s_%d.db"), path.GetString(), backupPathPath.GetTitle().GetString(), i);
		}

		if (!MoveFile(tempPath, newFullPath))
		{
			throw std::runtime_error("the unpacked database could not be moved next to the current one, error " + std::to_string(::GetLastError()));
		}
		settings.SetDBPath(newFullPath);
		CDatabaseManager::OpenDatabase(settings, database, state, newFullPath);
	}
	catch (const std::exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Restoring the database from %s failed: %s"), backupPath.GetString(), CString(e.what()).GetString()));
		return FALSE;
	}

	CLogger::Log(CStringUtil::Format(_T("Done restoring db, from: %s"), backupPath.GetString()));
	windows.RefreshView();
	return TRUE;
}

BOOL CDatabaseManager::CreateDB(CString csFile)
{
	try
	{
		CppSQLite3DB db;
		db.open(csFile);

		db.execDML(_T("PRAGMA auto_vacuum = 1"));

		db.execDML(_T("CREATE TABLE Main(")
			_T("lID INTEGER PRIMARY KEY AUTOINCREMENT, ")
			_T("lDate INTEGER, ")
			_T("mText TEXT, ")
			_T("lShortCut INTEGER, ")
			_T("lDontAutoDelete INTEGER, ")
			_T("CRC INTEGER, ")
			_T("bIsGroup INTEGER, ")
			_T("lParentID INTEGER, ")
			_T("QuickPasteText TEXT, ")
			_T("clipOrder REAL, ")
			_T("clipGroupOrder REAL, ")
			_T("globalShortCut INTEGER, ")
			_T("lastPasteDate INTEGER, ")
			_T("stickyClipOrder REAL, ")
			_T("stickyClipGroupOrder REAL, ")
			_T("MoveToGroupShortCut INTEGER, ")
			_T("GlobalMoveToGroupShortCut INTEGER);"));

		db.execDML(_T("CREATE TABLE Data(")
			_T("lID INTEGER PRIMARY KEY AUTOINCREMENT, ")
			_T("lParentID INTEGER, ")
			_T("strClipBoardFormat TEXT, ")
			_T("ooData BLOB);"));

		db.execDML(_T("CREATE TABLE Types(")
			_T("lID INTEGER PRIMARY KEY AUTOINCREMENT, ")
			_T("TypeText TEXT);"));

		db.execDML(_T("CREATE UNIQUE INDEX Main_ID on Main(lID ASC)"));
		db.execDML(_T("CREATE UNIQUE INDEX Data_ID on Data(lID ASC)"));
		db.execDML(_T("CREATE INDEX Main_ClipOrder on Main(clipOrder DESC)"));
		db.execDML(_T("CREATE INDEX Main_ClipGroupOrder on Main(clipGroupOrder DESC)"));
		db.execDML(_T("CREATE INDEX Main_ParentId on Main(lParentID DESC)"));
		db.execDML(_T("CREATE INDEX Main_IsGroup on Main(bIsGroup DESC)"));

		db.execDML(_T("CREATE TRIGGER delete_data_trigger BEFORE DELETE ON Main FOR EACH ROW\n")
			_T("BEGIN\n")
			_T("INSERT INTO MainDeletes VALUES(old.lID, datetime('now'));\n")
			_T("END\n"));

		db.execDML(_T("CREATE TABLE CopyBuffers(")
			_T("lID INTEGER PRIMARY KEY AUTOINCREMENT, ")
			_T("lClipID INTEGER, ")
			_T("lCopyBuffer INTEGER)"));

		db.execDML(_T("CREATE TABLE MainDeletes(")
			_T("clipID INTEGER,")
			_T("modifiedDate)"));

		db.execDML(_T("CREATE TRIGGER MainDeletes_delete_data_trigger BEFORE DELETE ON MainDeletes FOR EACH ROW\n")
			_T("BEGIN\n")
			_T("DELETE FROM CopyBuffers WHERE lClipID = old.clipID;\n")
			_T("DELETE FROM Data WHERE lParentID = old.clipID;\n")
			_T("END\n"));

		db.execDML(_T("CREATE INDEX Data_ParentId_Format ON Data(lParentID COLLATE BINARY ASC, strClipBoardFormat COLLATE NOCASE ASC);"));

		db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_TopLevelParentID ON Main(lParentId ASC, stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC);"));
		db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_TopLevel ON Main(stickyClipOrder DESC, bIsGroup ASC, clipOrder DESC);"));
		db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_InGroup2 ON Main(lParentId ASC, stickyClipGroupOrder DESC, bIsGroup ASC, clipGroupOrder DESC);"));

		db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_ShortCut2 on Main(lShortCut DESC, globalShortCut DESC)"));
		db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_MoveToGroup on Main(MoveToGroupShortCut DESC, GlobalMoveToGroupShortCut DESC)"));
		db.execDML(_T("CREATE INDEX IF NOT EXISTS Main_CRC on Main(CRC ASC)"));

		db.close();
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Creating the clip database %s failed: %s"), csFile.GetString(), e.errorMessage()));
		return FALSE;
	}

		return TRUE;
}

BOOL CDatabaseManager::CompactDatabase()
{
	//	if(!theApp.CloseDB())
	//		return FALSE;
	//
	//	CString csDBName = GetDBName();
	//	CString csTempDBName = csDBName;
	//	csTempDBName.Replace(".mdb", "TempDBName.mdb");
	//	
	//	//Compact the database			
	//	try
	//	{
	//		CDaoWorkspace::CompactDatabase(csDBName, csTempDBName);//, dbLangGeneral, 0, "andrew");//DATABASE_PASSWORD);
	//	}
	//	catch(CDaoException* e)
	//	{
	//		AfxMessageBox(e->m_pErrorInfo->m_strDescription);
	//		DeleteFile(csTempDBName);
	//		e->Delete();
	//		return FALSE;
	//	}
	//	catch(CMemoryException* e) 
	//	{
	//		AfxMessageBox("Memory Exception");
	//		DeleteFile(csTempDBName);
	//		e->Delete();
	//		return FALSE;
	//	}
	//	
	//	//Since compacting the database creates a new db delete the old one and replace it
	//	//with the compacted db
	//	if(DeleteFile(csDBName))
	//	{
	//		try
	//		{
	//			CFile::Rename(csTempDBName, csDBName);
	//		}
	//		catch(CFileException *e)
	//		{
	//			e->ReportError();
	//			e->Delete();
	//			return FALSE;
	//		}
	//	}
	//	else
	//		AfxMessageBox("Error Compacting Database");

	return TRUE;
}

BOOL CDatabaseManager::RepairDatabase()
{
	//	if(!theApp.CloseDB())
	//		return FALSE;

	//	try
	//	{
	//		CDaoWorkspace::RepairDatabase(GetDBName());
	//	}
	//	catch(CDaoException *e)
	//	{
	//		AfxMessageBox(e->m_pErrorInfo->m_strDescription);
	//		e->Delete();
	//		return FALSE;
	//	}

	return TRUE;
}

void CClipRetentionPolicy::RemoveClipsOverMaxEntries(CGetSetOptions& settings, CAppWindows& windows, CppSQLite3DB& db)
{
	long lMax{settings.GetMaxEntries()};
	if (lMax >= 0)
	{
		CClipIDs IDs{};
		int clipId{};

		CppSQLite3Query q{db.execQueryEx(_T("SELECT lID, lShortCut, lParentID, lDontAutoDelete, stickyClipOrder, stickyClipGroupOrder FROM Main WHERE bIsGroup = 0 ORDER BY clipOrder DESC LIMIT -1 OFFSET %d"), lMax)};
		while (q.eof() == false)
		{
			int shortcut{q.getIntField(_T("lShortCut"))};
			int dontDelete{q.getIntField(_T("lDontAutoDelete"))};
			int parentId{q.getIntField(_T("lParentID"))};
			double stickyClipOrder{q.getFloatField(_T("stickyClipOrder"))};
			double stickyClipGroupOrder{q.getFloatField(_T("stickyClipGroupOrder"))};

			//Only delete entries that have no shortcut and don't have the flag set and aren't in groups and
			if (shortcut == 0 &&
				dontDelete == 0 &&
				parentId <= 0 &&
				stickyClipOrder == -(2147483647) &&
				stickyClipGroupOrder == -(2147483647))
			{
				clipId = q.getIntField(_T("lID"));
				IDs.Add(clipId);
				CLogger::Log(CStringUtil::Format(_T("From MaxEntries - Deleting Id: %d"), clipId));
			}

			q.nextRow();
		}

		if (IDs.GetCount() > 0)
		{
			IDs.DeleteIDs(windows, false, db);
		}
	}
}

void CClipRetentionPolicy::RemoveExpiredClips(CGetSetOptions& settings, CAppWindows& windows, CppSQLite3DB& db)
{
	long lExpire{settings.GetExpiredEntries()};

	if (lExpire)
	{
		CTime now{CTime::GetCurrentTime()};
		now -= CTimeSpan(lExpire, 0, 0, 0);

		CClipIDs IDs{};

		CppSQLite3Query q{db.execQueryEx(_T("SELECT lID FROM Main ")
			_T("WHERE lastPasteDate < %d AND ")
			_T("bIsGroup = 0 AND lShortCut = 0 AND lParentID <= 0 AND lDontAutoDelete = 0 AND stickyClipOrder = -(2147483647) AND stickyClipGroupOrder = -(2147483647)"), (int)now.GetTime())};

		while (q.eof() == false)
		{
			IDs.Add(q.getIntField(_T("lID")));

			CLogger::Log(CStringUtil::Format(_T("From Clips Expire - Deleting Id: %d"), q.getIntField(_T("lID"))));

			q.nextRow();
		}

		if (IDs.GetCount() > 0)
		{
			IDs.DeleteIDs(windows, false, db);
		}
	}
}

BOOL CClipRetentionPolicy::RemoveOldEntries(CGetSetOptions& settings, CIdleTime& idleTime, CAppWindows& windows, bool checkIdleTime)
{
	CLogger::Log(CStringUtil::Format(_T("Beginning of RemoveOldEntries MaxEntries: %d - Keep days: %d"), settings.GetMaxEntries(), settings.GetExpiredEntries()));

	try
	{
		CppSQLite3DB db;
		CString csDbPath = settings.GetDBPath();
		db.open(csDbPath);

		if (settings.GetCheckForMaxEntries())
		{
			RemoveClipsOverMaxEntries(settings, windows, db);
		}

		if (settings.GetCheckForExpiredEntries())
		{
			RemoveExpiredClips(settings, windows, db);
		}

		int toDeleteCount = db.execScalar(_T("SELECT COUNT(clipID) FROM MainDeletes"));

		CLogger::Log(CStringUtil::Format(_T("Before Deleting emptied out data, count: %d, Idle Seconds: %f"), toDeleteCount, idleTime.IdleSeconds()));

		//Only delete 1 at a time, was finding that it was taking a long time to delete clips, locking the db and causing other queries
		//to lock up
		CppSQLite3Query q = db.execQueryEx(_T("SELECT * FROM MainDeletes LIMIT %d"), settings.GetMainDeletesDeleteCount());
		int deleteCount = 0;

		while (q.eof() == false)
		{
			double idleSeconds = idleTime.IdleSeconds();
			if (checkIdleTime == false || idleSeconds > settings.GetIdleSecondsBeforeDelete())
			{
				//delete any data items sitting out there that the main table data was deleted
				//this was done to speed up deleted from the main table
				deleteCount = db.execDMLEx(_T("DELETE FROM MainDeletes WHERE clipID=%d"), q.getIntField(_T("clipID")));
			}
			else
			{
				CLogger::Log(CStringUtil::Format(_T("Computer has not been idle long enough to delete clips, Min Idle: %d, current Idle: %d"),
					settings.GetIdleSecondsBeforeDelete(), idleSeconds));

				break;
			}
			q.nextRow();
		}

		toDeleteCount = db.execScalar(_T("SELECT COUNT(clipID) FROM MainDeletes"));

		CLogger::Log(CStringUtil::Format(_T("After Deleting emptied out data rows, Count: %d, toDelete: %d"), deleteCount, toDeleteCount));
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Removing old clips failed: %s"), e.errorMessage()));
		return FALSE;
	}

		CLogger::Log(_T("End of RemoveOldEntries"));

	return TRUE;
}

BOOL CClipRetentionPolicy::DeleteNonUsedClips(CDittoDb& database, CAppWindows& windows, bool fromAppWindow)
{
	CLogger::Log(_T("Start of delete all non used clips"));
	CClipIDs IDs;

	CppSQLite3Query q = database.execQueryEx(_T("SELECT lID FROM Main WHERE bIsGroup = 0 AND lShortCut = 0 AND lParentID <= 0 AND lDontAutoDelete = 0 AND stickyClipOrder = -(2147483647) AND stickyClipGroupOrder = -(2147483647)"));

	while (q.eof() == false)
	{
		IDs.Add(q.getIntField(_T("lID")));

		CLogger::Log(CStringUtil::Format(_T("From Clips DeleteNonUsedClips - Deleting Id: %d"), q.getIntField(_T("lID"))));

		q.nextRow();
	}

	int clipsDeleted = (int)IDs.GetCount();
	int deletedTableCount = 0;

	if (IDs.GetCount() > 0)
	{
		IDs.DeleteIDs(windows, fromAppWindow, database);

		deletedTableCount = database.execDMLEx(_T("DELETE FROM MainDeletes"));
	}

	CLogger::Log(CStringUtil::Format(_T("End of delete all non used clips, clips deleted: %d, delete table delted: %d"), clipsDeleted, deletedTableCount));

	return TRUE;
}

BOOL CDatabaseManager::EnsureDirectory(CString csPath)
{
	TCHAR drive[_MAX_DRIVE];
	TCHAR dir[_MAX_DIR];
	TCHAR fname[_MAX_FNAME];
	TCHAR ext[_MAX_EXT];

	_tsplitpath(csPath, drive, dir, fname, ext);

	CString csDir(drive);
	csDir += dir;

	if (CFileSystem::FileExists(csDir) == FALSE)
	{
		if (CreateDirectory(csDir, NULL))
			return TRUE;
	}
	else
		return TRUE;

	return FALSE;
}

// BOOL RunZippApp(CString csCommandLine)
// {
// 	CString csLocalPath = _tgetenv(_T("U3_HOST_EXEC_PATH"));
// 	CFolderPath::AddTrailingSlash(csLocalPath);
// 
// 	CString csZippApp = _tgetenv(_T("U3_DEVICE_EXEC_PATH"));
// 	CFolderPath::AddTrailingSlash(csZippApp);
// 	csZippApp += "7za.exe";
// 
// 	csZippApp += " ";
// 	csZippApp += csCommandLine;
// 
// 	CLogger::Log(csZippApp);
// 
// 	STARTUPINFO			StartupInfo;
// 	PROCESS_INFORMATION	ProcessInformation;
// 
// 	ZeroMemory(&StartupInfo, sizeof(StartupInfo));
// 	StartupInfo.cb = sizeof(StartupInfo);
// 	ZeroMemory(&ProcessInformation, sizeof(ProcessInformation));
// 
// 	StartupInfo.dwFlags = STARTF_USESHOWWINDOW;
// 	StartupInfo.wShowWindow = SW_HIDE;
// 
// 	BOOL bRet = CreateProcess(NULL, csZippApp.GetBuffer(csZippApp.GetLength()), NULL, NULL, FALSE,
// 			CREATE_DEFAULT_ERROR_MODE | NORMAL_PRIORITY_CLASS, NULL, csLocalPath,
// 			&StartupInfo, &ProcessInformation);
// 
// 	if(bRet)
// 	{
// 		WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
// 
// 		DWORD dwExitCode;
// 		GetExitCodeProcess(ProcessInformation.hProcess, &dwExitCode);
// 
// 		CString cs;
// 		cs.Format(_T("Exit code from unzip = %d"), dwExitCode);
// 		CLogger::Log(cs);
// 
// 		if(dwExitCode != 0)
// 		{
// 			bRet = FALSE;
// 		}
// 	}
// 	else
// 	{
// 		bRet = FALSE;
// 		CLogger::Log(_T("Create Process Failed"));
// 	}
// 
// 	csZippApp.ReleaseBuffer();
// 
// 	return bRet;
// }

// BOOL CopyDownDatabase()
// {
// 	BOOL bRet = FALSE;
// 
// 	CString csZippedPath = _tgetenv(_T("U3_APP_DATA_PATH"));
// 	CFolderPath::AddTrailingSlash(csZippedPath);
// 	
// 	CString csUnZippedPath = csZippedPath;
// 	csUnZippedPath += "Ditto.db";
// 
// 	csZippedPath += "Ditto.7z";
// 	
// 	CString csLocalPath = _tgetenv(_T("U3_HOST_EXEC_PATH"));
// 	CFolderPath::AddTrailingSlash(csLocalPath);
// 
// 	if(CFileSystem::FileExists(csZippedPath))
// 	{
// 		CString csCommandLine;
// 
// 		//e = extract
// 		//surround command line arguments with quotes
// 		//-aoa = overight files with extracted files
// 
// 		csCommandLine += "e ";
// 		csCommandLine += "\"";
// 		csCommandLine += csZippedPath;
// 		csCommandLine += "\"";
// 		csCommandLine += " -o";
// 		csCommandLine += "\"";
// 		csCommandLine += csLocalPath;
// 		csCommandLine += "\"";
// 		csCommandLine += " -aoa";
// 
// 		bRet = RunZippApp(csCommandLine);
// 
// 		csLocalPath += "Ditto.db";
// 	}
// 	else if(CFileSystem::FileExists(csUnZippedPath))
// 	{
// 		csLocalPath += "Ditto.db";
// 		bRet = CopyFile(csUnZippedPath, csLocalPath, FALSE);
// 	}
// 
// 	if(CFileSystem::FileExists(csLocalPath) == FALSE)
// 	{
// 		CLogger::Log(_T("Failed to copy files from device zip file"));
// 	}
// 
// 	CGetSetOptions::nLastDbWriteTime = CFileSystem::GetLastWriteTime(csLocalPath);
// 
// 	return bRet;
// }

//BOOL CopyUpDatabase()
//{
//	CStringA csZippedPath = "C:\\";//getenv("U3_APP_DATA_PATH");
//	CFolderPath::AddTrailingSlash(csZippedPath);
//	csZippedPath += "Ditto.zip";

//	CStringA csLocalPath = GetDBName();//getenv("U3_HOST_EXEC_PATH");
//	//CFolderPath::AddTrailingSlash(csLocalPath);
//	//csLocalPath += "Ditto.db";
//
//	CZipper Zip;
//
//	if(Zip.OpenZip(csZippedPath))
//	{
//		Zip.AddFileToZip(csLocalPath);
//	}
//
//	return TRUE;
//}
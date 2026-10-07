/**
 * @file CppSQLite3Tests.cpp
 * @brief Tests of CppSQLite3DB's open and extension loading.
 */
#include "stdafx.h"
#include "sqlite\CppSQLite3.h"

#include <gtest/gtest.h>

// Regression: open() threw on a failed sqlite3_open16 but left the connection handle open.
TEST(CppSQLite3DB, FailedOpenThrowsAndClosesTheConnection)
{
	CppSQLite3DB db;

	// a path below a file that does not exist cannot be opened
	EXPECT_THROW(db.open(_T("Z:\\no such folder\\no such folder\\ditto.db")), CppSQLite3Exception);
	EXPECT_FALSE(db.IsDatabaseOpen());
}

TEST(CppSQLite3DB, OpensWithoutLoadingAnyExtension)
{
	CppSQLite3DB db;

	db.open(_T(":memory:"));

	EXPECT_TRUE(db.IsDatabaseOpen());
	EXPECT_EQ(db.execScalar(_T("SELECT 1 WHERE 'Ditto' REGEXP 'it+o'")), 1);
}

// Regression: open() ignored a failed ICU extension load and never freed SQLite's message.
TEST(CppSQLite3DB, MissingExtensionThrowsWithSqliteMessageAndCloses)
{
	CppSQLite3DB db;
	db.open(_T(":memory:"));

	try
	{
		db.loadExtension("NoSuchDittoExtension.dll", "sqlite3_none_init");
		FAIL() << "loadExtension did not throw";
	}
	catch (CppSQLite3Exception& e)
	{
		EXPECT_NE(CString(e.errorMessage()).Find(_T("NoSuchDittoExtension.dll")), -1);
	}
	EXPECT_FALSE(db.IsDatabaseOpen());
}

TEST(CppSQLite3DB, SqlCannotLoadExtensions)
{
	CppSQLite3DB db;
	db.open(_T(":memory:"));

	EXPECT_THROW(db.execQuery(_T("SELECT load_extension('NoSuchDittoExtension.dll')")), CppSQLite3Exception);
}

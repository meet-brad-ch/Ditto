/**
 * @file CppSQLite3Tests.cpp
 * @brief Tests of CppSQLite3: open and extension loading, the exception, the regexp function and
 *        statement queries.
 */
#include "stdafx.h"
#include "sqlite\CppSQLite3.h"

#include <gtest/gtest.h>

#include <exception>
#include <filesystem>
#include <fstream>
#include <string>

namespace
{
	/** A database file under the temp folder, removed before and after the test. */
	class TempDatabaseFile
	{
	public:
		explicit TempDatabaseFile(const std::wstring& name)
			: m_path{ std::filesystem::temp_directory_path() / (L"DittoAppTests_" + std::to_wstring(::GetCurrentProcessId()) + L"_" + name + L".db") }
		{
			std::filesystem::remove(m_path);
		}
		~TempDatabaseFile() { std::error_code ignored{}; std::filesystem::remove(m_path, ignored); }
		TempDatabaseFile(const TempDatabaseFile&) = delete;
		TempDatabaseFile& operator=(const TempDatabaseFile&) = delete;

		CString Path() const { return CString(m_path.c_str()); }
		const std::filesystem::path& FilePath() const { return m_path; }

	private:
		std::filesystem::path m_path{};
	};

	/** Runs a statement and returns the exception it threw; fails the test when it did not throw. */
	CppSQLite3Exception ErrorOf(CppSQLite3DB& db, const TCHAR* sql)
	{
		try
		{
			db.execQuery(sql);
		}
		catch (CppSQLite3Exception& e)
		{
			return e;
		}
		ADD_FAILURE() << "the statement did not throw";
		return CppSQLite3Exception(SQLITE_OK, _T("no error"));
	}
}

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

// Regression: CppSQLite3Exception was no std::exception, so the start-up boundary (which catches
// std::exception) missed it; its message went into a fixed buffer of 1000 characters, unchecked.
TEST(CppSQLite3Exception, IsAStdExceptionWithTheWholeMessageInUtf8)
{
	const std::wstring longText(5000, L'a');
	try
	{
		throw CppSQLite3Exception(SQLITE_BUSY, (L"locked é " + longText).c_str());
	}
	catch (const std::exception& e)
	{
		EXPECT_EQ(std::string(e.what()), "SQLITE_BUSY[5]: locked \xc3\xa9 " + std::string(5000, 'a'));
		const CppSQLite3Exception* sqliteError = dynamic_cast<const CppSQLite3Exception*>(&e);
		ASSERT_NE(sqliteError, nullptr);
		EXPECT_EQ(std::wstring(sqliteError->errorMessage()), L"SQLITE_BUSY[5]: locked é " + longText);
		EXPECT_EQ(sqliteError->errorCode(), SQLITE_BUSY);
	}
}

// Regression: a database another connection holds was treated like a damaged one (renamed to
// _BAD at start-up). A lock is "unavailable"; a damaged file or a schema error is not.
TEST(CppSQLite3Exception, LockedDatabaseIsUnavailable)
{
	const TempDatabaseFile file(L"Locked");
	CppSQLite3DB holder;
	holder.open(file.Path());
	holder.execDML(_T("CREATE TABLE t(x INTEGER)"));
	holder.execDML(_T("BEGIN EXCLUSIVE"));

	CppSQLite3DB other;
	other.open(file.Path());
	other.setBusyTimeout(0);
	const CppSQLite3Exception error = ErrorOf(other, _T("SELECT x FROM t"));

	EXPECT_EQ(error.errorCode(), SQLITE_BUSY);
	EXPECT_TRUE(error.isUnavailable());
	holder.execDML(_T("ROLLBACK"));
}

TEST(CppSQLite3Exception, DamagedFileIsNotUnavailable)
{
	const TempDatabaseFile file(L"Damaged");
	{
		std::ofstream garbage(file.FilePath(), std::ios::binary);
		garbage << std::string(4096, 'x');
	}
	CppSQLite3DB db;
	db.open(file.Path());

	const CppSQLite3Exception error = ErrorOf(db, _T("SELECT lID FROM Main"));

	EXPECT_EQ(error.errorCode(), SQLITE_NOTADB);
	EXPECT_FALSE(error.isUnavailable());
}

TEST(CppSQLite3Exception, MissingTableIsNotUnavailable)
{
	CppSQLite3DB db;
	db.open(_T(":memory:"));

	const CppSQLite3Exception error = ErrorOf(db, _T("SELECT lID FROM Main"));

	EXPECT_EQ(error.errorCode(), SQLITE_ERROR);
	EXPECT_FALSE(error.isUnavailable());
}

// Regression: an invalid pattern set no result, so SQL saw NULL instead of an error.
TEST(CppSQLite3DB, InvalidRegexpPatternFailsTheStatement)
{
	CppSQLite3DB db;
	db.open(_T(":memory:"));

	EXPECT_THROW(db.execQuery(_T("SELECT 'Ditto' REGEXP '('")), CppSQLite3Exception);
}

// Regression: a failed step of a statement's query finalized the statement's VM, which the
// statement then used and finalized again (use after free).
TEST(CppSQLite3Statement, FailedRowOfItsQueryLeavesTheStatementUsable)
{
	CppSQLite3DB db;
	db.open(_T(":memory:"));
	db.execDML(_T("CREATE TABLE t(x INTEGER)"));
	db.execDML(_T("INSERT INTO t VALUES (1), (2)"));
	// the second row fails: abs() of the smallest 64-bit integer overflows. The operand depends on x:
	// SQLite evaluates a constant expression once before the first row (measured: the first step
	// failed), and ORDER BY rowid needs no sorter, so row 1 is returned before row 2 is computed.
	CppSQLite3Statement select = db.compileStatement(_T("SELECT CASE WHEN x = 2 THEN abs(-9223372036854775806 - x) ELSE x END FROM t ORDER BY rowid"));
	{
		CppSQLite3Query rows = select.execQuery();
		ASSERT_FALSE(rows.eof());
		EXPECT_EQ(rows.getIntField(0), 1);
		EXPECT_THROW(rows.nextRow(), CppSQLite3Exception);
	}

	select.reset();
	CppSQLite3Query again = select.execQuery();
	ASSERT_FALSE(again.eof());
	EXPECT_EQ(again.getIntField(0), 1);
}

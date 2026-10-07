#pragma once

#include "DittoDb.h"

#include <mutex>

// A transaction on CDittoDb (RAII): the constructor locks the connection and begins, Commit()
// commits, and a transaction that was not committed is rolled back when it goes out of scope
// (for example when a statement threw).
// Transactions nest: the outermost one is BEGIN/COMMIT, an inner one a SAVEPOINT, so code that
// opens a transaction may be called from code that already holds one (SQLite cannot nest BEGIN).
class CDittoDbTransaction
{
public:
	explicit CDittoDbTransaction(CDittoDb& db);
	~CDittoDbTransaction();

	CDittoDbTransaction(const CDittoDbTransaction&) = delete;
	CDittoDbTransaction& operator=(const CDittoDbTransaction&) = delete;

	// Commits (or releases the savepoint); throws CppSQLite3Exception when that fails, and the
	// destructor then rolls back
	void Commit();

private:
	// The savepoint name of a nested transaction
	CString SavepointName() const;

	CDittoDb& m_db;
	std::unique_lock<std::recursive_mutex> m_lock;
	// 1 for the outermost transaction
	int m_depth{};
	bool m_committed{};
};

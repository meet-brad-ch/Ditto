#pragma once

#include "sqlite\CppSQLite3.h"

#include <functional>
#include <mutex>

// Ditto's database connection. The clip window, the copy thread and the paste and import paths
// share one connection, so work that spans statements holds Lock(): an insert and its
// lastRowId (InsertReturningId) or a transaction (CDittoDbTransaction). The lock is recursive,
// so a locked sequence may call code that locks again.
class CDittoDb : public CppSQLite3DB
{
public:
	// log: where errors that cannot be thrown are written (a failed rollback during unwinding)
	explicit CDittoDb(std::function<void(const CString&)> log);

	// Holds the connection for one thread until the returned lock is released
	std::unique_lock<std::recursive_mutex> Lock();

	// The statement calls of CppSQLite3DB under the lock (the Ex variants call these), so every
	// call on this connection waits for another thread's transaction to end instead of running
	// inside it, also through a CppSQLite3DB&
	int execDML(const TCHAR* szSQL) override;
	CppSQLite3Query execQuery(const TCHAR* szSQL) override;
	int execScalar(const TCHAR* szSQL) override;

	// Runs an INSERT statement and returns the id of the new row, under one lock so no other
	// thread's insert can come between the two
	sqlite_int64 InsertReturningId(CppSQLite3Statement& insert);

	// The number of CDittoDbTransaction objects open on this connection (call with Lock() held)
	int TransactionDepth() const noexcept;
	// Set by CDittoDbTransaction as transactions open and close
	void SetTransactionDepth(int depth) noexcept;

	// Writes an error that cannot be thrown
	void LogError(const CString& text) const;

protected:
	/**
	 * @brief The lock of this connection, so prepared statements (compileStatement) step and reset
	 * under it like execDML/execQuery (upstream ran them without it, so a statement from another
	 * thread ran inside, and was rolled back with, a transaction it did not belong to).
	 * @return the recursive mutex Lock() holds.
	 */
	std::recursive_mutex* connectionMutex() override;

private:
	std::recursive_mutex m_mutex;
	std::function<void(const CString&)> m_log;
	int m_transactionDepth{};
};

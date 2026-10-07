#pragma once

#include "DittoDb.h"

#include <mutex>

// A transaction on CDittoDb (RAII): the constructor locks the connection and begins, Commit()
// commits, and a transaction that was not committed is rolled back when it goes out of scope
// (for example when a statement threw).
class CDittoDbTransaction
{
public:
	explicit CDittoDbTransaction(CDittoDb& db);
	~CDittoDbTransaction();

	CDittoDbTransaction(const CDittoDbTransaction&) = delete;
	CDittoDbTransaction& operator=(const CDittoDbTransaction&) = delete;

	// Commits; throws CppSQLite3Exception when the commit fails (the destructor then rolls back)
	void Commit();

private:
	CDittoDb& m_db;
	std::unique_lock<std::recursive_mutex> m_lock;
	bool m_committed{};
};

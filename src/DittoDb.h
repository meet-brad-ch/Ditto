#pragma once

#include "sqlite\CppSQLite3.h"

#include <mutex>

// Ditto's database connection. The clip window, the copy thread and the paste and import paths
// share one connection, so work that spans statements holds Lock(): an insert and its
// lastRowId (InsertReturningId) or a transaction (CDittoDbTransaction). The lock is recursive,
// so a locked sequence may call code that locks again.
class CDittoDb : public CppSQLite3DB
{
public:
	// Holds the connection for one thread until the returned lock is released
	std::unique_lock<std::recursive_mutex> Lock();

	// Runs an INSERT statement and returns the id of the new row, under one lock so no other
	// thread's insert can come between the two
	sqlite_int64 InsertReturningId(CppSQLite3Statement& insert);

private:
	std::recursive_mutex m_mutex;
};

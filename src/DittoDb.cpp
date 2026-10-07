#include "stdafx.h"
#include "DittoDb.h"

std::unique_lock<std::recursive_mutex> CDittoDb::Lock()
{
	return std::unique_lock<std::recursive_mutex>(m_mutex);
}

sqlite_int64 CDittoDb::InsertReturningId(CppSQLite3Statement& insert)
{
	const std::unique_lock<std::recursive_mutex> lock = Lock();
	insert.execDML();
	return lastRowId();
}

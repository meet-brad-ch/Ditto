#include "stdafx.h"
#include "DittoDb.h"

#include <stdexcept>
#include <utility>

CDittoDb::CDittoDb(std::function<void(const CString&)> log) :
	m_log(std::move(log))
{
	if (!m_log)
	{
		throw std::invalid_argument("CDittoDb needs a log function");
	}
}

std::unique_lock<std::recursive_mutex> CDittoDb::Lock()
{
	return std::unique_lock<std::recursive_mutex>(m_mutex);
}

int CDittoDb::execDML(const TCHAR* szSQL)
{
	const std::unique_lock<std::recursive_mutex> lock = Lock();
	return CppSQLite3DB::execDML(szSQL);
}

CppSQLite3Query CDittoDb::execQuery(const TCHAR* szSQL)
{
	const std::unique_lock<std::recursive_mutex> lock = Lock();
	return CppSQLite3DB::execQuery(szSQL);
}

int CDittoDb::execScalar(const TCHAR* szSQL)
{
	const std::unique_lock<std::recursive_mutex> lock = Lock();
	return CppSQLite3DB::execScalar(szSQL);
}

sqlite_int64 CDittoDb::InsertReturningId(CppSQLite3Statement& insert)
{
	const std::unique_lock<std::recursive_mutex> lock = Lock();
	insert.execDML();
	return lastRowId();
}

int CDittoDb::TransactionDepth() const noexcept
{
	return m_transactionDepth;
}

void CDittoDb::SetTransactionDepth(int depth) noexcept
{
	m_transactionDepth = depth;
}

void CDittoDb::LogError(const CString& text) const
{
	m_log(text);
}

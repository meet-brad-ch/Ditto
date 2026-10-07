#include "stdafx.h"
#include "DittoDbTransaction.h"

CDittoDbTransaction::CDittoDbTransaction(CDittoDb& db) :
	m_db(db),
	m_lock(db.Lock()),
	m_depth(db.TransactionDepth() + 1)
{
	if (m_depth == 1)
	{
		m_db.execDML(_T("BEGIN IMMEDIATE;"));
	}
	else
	{
		m_db.execDML(_T("SAVEPOINT ") + SavepointName() + _T(";"));
	}
	// counted only once it has begun: a constructor that threw has nothing to undo
	m_db.SetTransactionDepth(m_depth);
}

CDittoDbTransaction::~CDittoDbTransaction()
{
	m_db.SetTransactionDepth(m_depth - 1);
	if (m_committed)
	{
		return;
	}
	try
	{
		if (m_depth == 1)
		{
			m_db.execDML(_T("ROLLBACK;"));
		}
		else
		{
			m_db.execDML(_T("ROLLBACK TO ") + SavepointName() + _T(";"));
			m_db.execDML(_T("RELEASE ") + SavepointName() + _T(";"));
		}
	}
	catch (CppSQLite3Exception& e)
	{
		// a destructor must not throw; the error that ended the transaction is already on its
		// way to the caller, so the failed rollback is logged next to it
		CString text;
		text.Format(_T("Rollback failed: %d - %s"), e.errorCode(), e.errorMessage());
		m_db.LogError(text);
	}
}

void CDittoDbTransaction::Commit()
{
	if (m_depth == 1)
	{
		m_db.execDML(_T("COMMIT;"));
	}
	else
	{
		m_db.execDML(_T("RELEASE ") + SavepointName() + _T(";"));
	}
	m_committed = true;
}

CString CDittoDbTransaction::SavepointName() const
{
	CString name;
	name.Format(_T("ditto_nested_%d"), m_depth);
	return name;
}

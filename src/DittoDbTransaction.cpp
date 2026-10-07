#include "stdafx.h"
#include "DittoDbTransaction.h"
#include "Misc.h"

CDittoDbTransaction::CDittoDbTransaction(CDittoDb& db) :
	m_db(db),
	m_lock(db.Lock())
{
	m_db.execDML(_T("BEGIN IMMEDIATE;"));
}

CDittoDbTransaction::~CDittoDbTransaction()
{
	if (m_committed)
	{
		return;
	}
	try
	{
		m_db.execDML(_T("ROLLBACK;"));
	}
	catch (CppSQLite3Exception& e)
	{
		// a destructor must not throw; the error that ended the transaction is already on its
		// way to the caller, so the failed rollback is logged next to it
		Log(StrF(_T("Rollback failed: %d - %s"), e.errorCode(), e.errorMessage()));
	}
}

void CDittoDbTransaction::Commit()
{
	m_db.execDML(_T("COMMIT;"));
	m_committed = true;
}

////////////////////////////////////////////////////////////////////////////////
// CppSQLite3 - A C++ wrapper around the SQLite3 embedded database library.
//
// Copyright (c) 2004 Rob Groves. All Rights Reserved. rob.groves@btinternet.com
// 
// Permission to use, copy, modify, and distribute this software and its
// documentation for any purpose, without fee, and without a written
// agreement, is hereby granted, provided that the above copyright notice, 
// this paragraph and the following two paragraphs appear in all copies, 
// modifications, and distributions.
//
// IN NO EVENT SHALL THE AUTHOR BE LIABLE TO ANY PARTY FOR DIRECT,
// INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES, INCLUDING LOST
// PROFITS, ARISING OUT OF THE USE OF THIS SOFTWARE AND ITS DOCUMENTATION,
// EVEN IF THE AUTHOR HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// THE AUTHOR SPECIFICALLY DISCLAIMS ANY WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE. THE SOFTWARE AND ACCOMPANYING DOCUMENTATION, IF
// ANY, PROVIDED HEREUNDER IS PROVIDED "AS IS". THE AUTHOR HAS NO OBLIGATION
// TO PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.
//
// V3.0		03/08/2004	-Initial Version for sqlite3
//
// V3.1		16/09/2004	-Implemented getXXXXField using sqlite3 functions
//						-Added CppSQLiteDB3::tableExists()
////////////////////////////////////////////////////////////////////////////////
#include "StdAfx.h"
#include "CppSQLite3.h"
#include <cstdlib>
#include <atlconv.h>
#include <memory>
#include <regex>
#include <algorithm>
#include <array>


// CppSQLite3Exception::DONT_DELETE_MSG: the value passed to CppSQLite3Exception for a string
// that cannot be deleted.
// CppSQLite3Binary::encode/decode: SQLite functions not included in SQLite DLL, but copied below
// from SQLite encode.c

////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////

CppSQLite3Exception::CppSQLite3Exception(const int nErrCode,
									const TCHAR* szErrMess,
									bool /*bDeleteMsg=true*/) :
									mnErrCode(nErrCode)
{
	swprintf(mpszErrMess, _T("%s[%d]: %s"),
								errorCodeAsString(nErrCode),
								nErrCode,
								szErrMess ? szErrMess : _T(""));
}

									
CppSQLite3Exception::CppSQLite3Exception(const CppSQLite3Exception&  e) :
									mnErrCode(e.mnErrCode)
{
	mpszErrMess[0] = 0;

	if(e.mpszErrMess)
	{
		swprintf(mpszErrMess, _T("%s"), e.mpszErrMess);
	}
}


const TCHAR* CppSQLite3Exception::errorCodeAsString(int nErrCode)
{
	for (const ErrorCodeName& entry : m_errorCodeNames)
	{
		if (entry.code == nErrCode)
		{
			return entry.name;
		}
	}
	return _T("UNKNOWN_ERROR");
}


CppSQLite3Exception::~CppSQLite3Exception()
{

}

////////////////////////////////////////////////////////////////////////////////

CppSQLite3Query::CppSQLite3Query()
{
	mpDB = 0;
	mpVM = 0;
	mbEof = true;
	mnCols = 0;
	mbOwnVM = false;
}


CppSQLite3Query::CppSQLite3Query(const CppSQLite3Query& rQuery)
{
	mpDB = rQuery.mpDB;
	mpVM = rQuery.mpVM;
	// Only one object can own the VM
	const_cast<CppSQLite3Query&>(rQuery).mpVM = 0;
	mbEof = rQuery.mbEof;
	mnCols = rQuery.mnCols;
	mbOwnVM = rQuery.mbOwnVM;
}


CppSQLite3Query::CppSQLite3Query(sqlite3* pDB,
							sqlite3_stmt* pVM,
							bool bEof,
							bool bOwnVM/*=true*/)
{
	mpDB = pDB;
	mpVM = pVM;
	mbEof = bEof;
	mnCols = sqlite3_column_count(mpVM);
	mbOwnVM = bOwnVM;
}


CppSQLite3Query::~CppSQLite3Query()
{
	// A destructor must not throw, so this does not call finalize(). sqlite3_finalize always frees
	// the statement, and its result code only repeats the error of the statement's last step:
	// nextRow() and execQuery() finalize and throw on a failed step, so an owned VM that is still
	// here has stepped without error and its finalize returns SQLITE_OK.
	if (mpVM && mbOwnVM)
	{
		static_cast<void>(sqlite3_finalize(mpVM));
		mpVM = 0;
	}
}


CppSQLite3Query& CppSQLite3Query::operator=(const CppSQLite3Query& rQuery)
{
	// a CppSQLite3Exception from finalize() propagates to the caller; finalize() has already
	// cleared mpVM and rQuery still owns its own VM
	finalize();
	mpDB = rQuery.mpDB;
	mpVM = rQuery.mpVM;
	// Only one object can own the VM
	const_cast<CppSQLite3Query&>(rQuery).mpVM = 0;
	mbEof = rQuery.mbEof;
	mnCols = rQuery.mnCols;
	mbOwnVM = rQuery.mbOwnVM;
	return *this;
}


int CppSQLite3Query::numFields()
{
	checkVM();
	return mnCols;
}


const TCHAR* CppSQLite3Query::fieldValue(int nField)
{
	checkVM();

	if (nField < 0 || nField > mnCols-1)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Invalid field index requested"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}

	return (const TCHAR*)sqlite3_column_text16(mpVM, nField);
}


const TCHAR* CppSQLite3Query::fieldValue(const TCHAR* szField)
{
	int nField = fieldIndex(szField);

	return (const TCHAR*)sqlite3_column_text16(mpVM, nField);
}


int CppSQLite3Query::getIntField(int nField, int nNullValue/*=0*/)
{
	if (fieldDataType(nField) == SQLITE_NULL)
	{
		return nNullValue;
	}
	else
	{
		return sqlite3_column_int(mpVM, nField);
	}
}


int CppSQLite3Query::getIntField(const TCHAR* szField, int nNullValue/*=0*/)
{
	int nField = fieldIndex(szField);
	return getIntField(nField, nNullValue);
}

__int64 CppSQLite3Query::getInt64Field(int nField, __int64 nNullValue/*=0*/)
{
	if (fieldDataType(nField) == SQLITE_NULL)
	{
		return nNullValue;
	}
	else
	{
		return sqlite3_column_int64(mpVM, nField);
	}
}


__int64 CppSQLite3Query::getInt64Field(const TCHAR* szField, __int64 nNullValue/*=0*/)
{
	int nField = fieldIndex(szField);
	return getInt64Field(nField, nNullValue);
}


double CppSQLite3Query::getFloatField(int nField, double fNullValue/*=0.0*/)
{
	if (fieldDataType(nField) == SQLITE_NULL)
	{
		return fNullValue;
	}
	else
	{
		return sqlite3_column_double(mpVM, nField);
	}
}


double CppSQLite3Query::getFloatField(const TCHAR* szField, double fNullValue/*=0.0*/)
{
	int nField = fieldIndex(szField);
	return getFloatField(nField, fNullValue);
}


const TCHAR* CppSQLite3Query::getStringField(int nField, const TCHAR* szNullValue/*=""*/)
{
	if (fieldDataType(nField) == SQLITE_NULL)
	{
		return szNullValue;
	}
	else
	{
		return (const TCHAR*)sqlite3_column_text16(mpVM, nField);
	}
}


const TCHAR* CppSQLite3Query::getStringField(const TCHAR* szField, const TCHAR* szNullValue/*=""*/)
{
	int nField = fieldIndex(szField);
	return getStringField(nField, szNullValue);
}

int CppSQLite3Query::getBlobFieldSize(int nField)
{
	checkVM();

	if (nField < 0 || nField > mnCols-1)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Invalid field index requested"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}

	int nLen = sqlite3_column_bytes(mpVM, nField);

	return nLen;
}

const unsigned char* CppSQLite3Query::getBlobField(int nField, int& nLen)
{
	checkVM();

	if (nField < 0 || nField > mnCols-1)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Invalid field index requested"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}

	nLen = sqlite3_column_bytes(mpVM, nField);
	return (const unsigned char*)sqlite3_column_blob(mpVM, nField);
}

int CppSQLite3Query::getBlobFieldSize(const TCHAR* szField)
{
	int nField = fieldIndex(szField);
	return getBlobFieldSize(nField);
}

const unsigned char* CppSQLite3Query::getBlobField(const TCHAR* szField, int& nLen)
{
	int nField = fieldIndex(szField);
	return getBlobField(nField, nLen);
}


bool CppSQLite3Query::fieldIsNull(int nField)
{
	return (fieldDataType(nField) == SQLITE_NULL);
}


bool CppSQLite3Query::fieldIsNull(const TCHAR* szField)
{
	int nField = fieldIndex(szField);
	return (fieldDataType(nField) == SQLITE_NULL);
}


int CppSQLite3Query::fieldIndex(const TCHAR* szField)
{
	checkVM();

	if (szField)
	{
		for (int nField = 0; nField < mnCols; nField++)
		{
			const TCHAR* szTemp = (const TCHAR*)sqlite3_column_name16(mpVM, nField);

			if(_tcscmp(szField, szTemp) == 0)
			{
				return nField;
			}
		}
	}


	throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
							_T("Invalid field name requested"),
							CppSQLite3Exception::DONT_DELETE_MSG);
}


const TCHAR* CppSQLite3Query::fieldName(int nCol)
{
	checkVM();

	if (nCol < 0 || nCol > mnCols-1)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Invalid field index requested"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}

	return (const TCHAR*)sqlite3_column_name16(mpVM, nCol);
}


const TCHAR* CppSQLite3Query::fieldDeclType(int nCol)
{
	checkVM();

	if (nCol < 0 || nCol > mnCols-1)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Invalid field index requested"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}

	return (const TCHAR*)sqlite3_column_decltype16(mpVM, nCol);
}


int CppSQLite3Query::fieldDataType(int nCol)
{
	checkVM();

	if (nCol < 0 || nCol > mnCols-1)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Invalid field index requested"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}

	return sqlite3_column_type(mpVM, nCol);
}


bool CppSQLite3Query::eof()
{
	checkVM();
	return mbEof;
}


void CppSQLite3Query::nextRow()
{
	checkVM();

	int nRet = sqlite3_step(mpVM);

	if (nRet == SQLITE_DONE)
	{
		// no rows
		mbEof = true;
	}
	else if (nRet == SQLITE_ROW)
	{
		// more rows, nothing to do
	}
	else
	{
		nRet = sqlite3_finalize(mpVM);
		mpVM = 0;
		const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
		throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


void CppSQLite3Query::finalize()
{
	if (mpVM && mbOwnVM)
	{
		int nRet = sqlite3_finalize(mpVM);
		mpVM = 0;
		if (nRet != SQLITE_OK)
		{
			const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
			throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
		}
	}
}


void CppSQLite3Query::checkVM()
{
	if (mpVM == 0)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Null Virtual Machine pointer"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}

////////////////////////////////////////////////////////////////////////////////

CppSQLite3Statement::CppSQLite3Statement()
{
	mpDB = 0;
	mpVM = 0;
}


CppSQLite3Statement::CppSQLite3Statement(const CppSQLite3Statement& rStatement)
{
	mpDB = rStatement.mpDB;
	mpVM = rStatement.mpVM;
	// Only one object can own VM
	const_cast<CppSQLite3Statement&>(rStatement).mpVM = 0;
}


CppSQLite3Statement::CppSQLite3Statement(sqlite3* pDB, sqlite3_stmt* pVM)
{
	mpDB = pDB;
	mpVM = pVM;
}


CppSQLite3Statement::~CppSQLite3Statement()
{
	// A destructor must not throw, so this does not call finalize(). sqlite3_finalize always frees
	// the statement, and its result code only repeats the error of the statement's last step,
	// which execDML()/execQuery() already threw to the caller.
	if (mpVM)
	{
		static_cast<void>(sqlite3_finalize(mpVM));
		mpVM = 0;
	}
}


CppSQLite3Statement& CppSQLite3Statement::operator=(const CppSQLite3Statement& rStatement)
{
	mpDB = rStatement.mpDB;
	mpVM = rStatement.mpVM;
	// Only one object can own VM
	const_cast<CppSQLite3Statement&>(rStatement).mpVM = 0;
	return *this;
}


int CppSQLite3Statement::execDML()
{
	checkDB();
	checkVM();

	int nRet = sqlite3_step(mpVM);

	if (nRet == SQLITE_DONE)
	{
		int nRowsChanged = sqlite3_changes(mpDB);

		nRet = sqlite3_reset(mpVM);

		if (nRet != SQLITE_OK)
		{
			const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
			throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
		}

		return nRowsChanged;
	}
	else
	{
		nRet = sqlite3_reset(mpVM);
		const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
		throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


CppSQLite3Query CppSQLite3Statement::execQuery()
{
	checkDB();
	checkVM();

	int nRet = sqlite3_step(mpVM);

	if (nRet == SQLITE_DONE)
	{
		// no rows
		return CppSQLite3Query(mpDB, mpVM, true/*eof*/, false);
	}
	else if (nRet == SQLITE_ROW)
	{
		// at least 1 row
		return CppSQLite3Query(mpDB, mpVM, false/*eof*/, false);
	}
	else
	{
		nRet = sqlite3_reset(mpVM);
		const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
		throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


void CppSQLite3Statement::bind(int nParam, const TCHAR* szValue)
{
	checkVM();

	int nRes = sqlite3_bind_text16(mpVM, nParam, szValue, -1, SQLITE_TRANSIENT);
	if (nRes != SQLITE_OK)
	{
		throw CppSQLite3Exception(nRes,
								_T("Error binding string param"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


void CppSQLite3Statement::bind(int nParam, const int nValue)
{
	checkVM();
	int nRes = sqlite3_bind_int(mpVM, nParam, nValue);

	if (nRes != SQLITE_OK)
	{
		throw CppSQLite3Exception(nRes,
								_T("Error binding int param"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


void CppSQLite3Statement::bind(int nParam, const double dValue)
{
	checkVM();
	int nRes = sqlite3_bind_double(mpVM, nParam, dValue);

	if (nRes != SQLITE_OK)
	{
		throw CppSQLite3Exception(nRes,
								_T("Error binding double param"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


void CppSQLite3Statement::bind(int nParam, const unsigned char* blobValue, int nLen)
{
	checkVM();
	int nRes = sqlite3_bind_blob(mpVM, nParam,
								(const void*)blobValue, nLen, SQLITE_TRANSIENT);

	if (nRes != SQLITE_OK)
	{
		throw CppSQLite3Exception(nRes,
								_T("Error binding blob param"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}

	
void CppSQLite3Statement::bindNull(int nParam)
{
	checkVM();
	int nRes = sqlite3_bind_null(mpVM, nParam);

	if (nRes != SQLITE_OK)
	{
		throw CppSQLite3Exception(nRes,
								_T("Error binding NULL param"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


void CppSQLite3Statement::reset()
{
	if (mpVM)
	{
		int nRet = sqlite3_reset(mpVM);

		if (nRet != SQLITE_OK)
		{
			const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
			throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
		}
	}
}


void CppSQLite3Statement::finalize()
{
	if (mpVM)
	{
		int nRet = sqlite3_finalize(mpVM);
		mpVM = 0;

		if (nRet != SQLITE_OK)
		{
			const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
			throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
		}
	}
}


void CppSQLite3Statement::checkDB()
{
	if (mpDB == 0)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Database not open"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


void CppSQLite3Statement::checkVM()
{
	if (mpVM == 0)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Null Virtual Machine pointer"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


////////////////////////////////////////////////////////////////////////////////

CppSQLite3DB::CppSQLite3DB()
{
	mpDB = 0;
	mnBusyTimeoutMs = 60000; // 60 seconds
}


CppSQLite3DB::CppSQLite3DB(const CppSQLite3DB& db)
{
	mpDB = db.mpDB;
	mnBusyTimeoutMs = 60000; // 60 seconds
}


CppSQLite3DB::~CppSQLite3DB()
{
	close();
}


CppSQLite3DB& CppSQLite3DB::operator=(const CppSQLite3DB& db)
{
	mpDB = db.mpDB;
	mnBusyTimeoutMs = 60000; // 60 seconds
	return *this;
}

void CppSQLite3DB::sqlite_regexp(sqlite3_context* context, int argc, sqlite3_value** values)
{
	char* reg = (char*) sqlite3_value_text(values[0]);
	char* text = (char*) sqlite3_value_text(values[1]);

	if (argc != 2 || reg == 0 || text == 0) 
	{
		//sqlite3_result_error(context, "SQL function regexp() called with invalid arguments.\n", -1);
		sqlite3_result_int(context, 0);
		return;
	}

	try
	{
		if (std::regex_search(text, std::regex(reg, std::regex::flag_type::icase)))
		{
			sqlite3_result_int(context, 1);
		}
		else
		{
			sqlite3_result_int(context, 0);
		}
	}
	catch (std::regex_error& e) 
	{
		CStringA r;
		r.Format("regex_search exception %d, reg: %s, str: %s", e.code(), reg, text);
		OutputDebugStringA(r);
	}
}

bool CppSQLite3DB::DBEncrypted()
{
	bool encrypted = false;
	CFile file;
	CFileException ex;
	if (file.Open(m_dbFile, CFile::modeRead | CFile::shareDenyNone, &ex))
	{
		if (file.GetLength() > 10)
		{
			//an unencrypted database will start with "SQLite format 3\000", if it's encrypted it will start with "Salted__"
			CStringA b;
			file.Read(b.GetBufferSetLength(6), 6);
			b.ReleaseBuffer();

			if (b != "SQLite")
			{
				encrypted = true;
			}
		}
		file.Close();
	}

	return encrypted;
}

void CppSQLite3DB::open(const TCHAR* szFile)
{
	int nRet = sqlite3_open16(szFile, &mpDB);

	//sqlite3_exec(mpDB, "PRAGMA rekey=123456", 0, 0, 0);
	//sqlite3_exec(mpDB, "PRAGMA key=123456", 0, 0, 0);

	//int r = sqlite3_rekey(mpDB, "123456", 6);
	
	//if (encrypted)
	//{
	//	int rr = sqlite3_key(mpDB, "123456", 6);
	//}

	m_dbFile = szFile;

	if (nRet != SQLITE_OK)
	{
		throwAndClose(nRet);
	}

	nRet = sqlite3_create_function(mpDB, "regexp", 2, SQLITE_ANY, 0, &CppSQLite3DB::sqlite_regexp, 0, 0);
	if (nRet != SQLITE_OK)
	{
		throwAndClose(nRet);
	}

	setBusyTimeout(mnBusyTimeoutMs);
}

void CppSQLite3DB::loadExtension(const char* szFile, const char* szEntryPoint)
{
	checkDB();

	// the C API only: SQL cannot call load_extension()
	int nRet = sqlite3_db_config(mpDB, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 1, nullptr);
	if (nRet != SQLITE_OK)
	{
		throwAndClose(nRet);
	}

	char* szLoadError{};
	nRet = sqlite3_load_extension(mpDB, szFile, szEntryPoint, &szLoadError);
	const std::unique_ptr<char, decltype(&sqlite3_free)> loadError{ szLoadError, &sqlite3_free };
	if (nRet != SQLITE_OK)
	{
		CString message;
		message.Format(_T("loading %s failed: %s"), CString(CA2W(szFile, CP_UTF8)).GetString(),
			szLoadError ? CString(CA2W(szLoadError, CP_UTF8)).GetString() : _T("no message"));
		CppSQLite3Exception error(nRet, message.GetBuffer(), CppSQLite3Exception::DONT_DELETE_MSG);
		close();
		throw error;
	}

	nRet = sqlite3_db_config(mpDB, SQLITE_DBCONFIG_ENABLE_LOAD_EXTENSION, 0, nullptr);
	if (nRet != SQLITE_OK)
	{
		throwAndClose(nRet);
	}
}

void CppSQLite3DB::throwAndClose(int nErrCode)
{
	const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
	// the exception copies the message before close() frees it
	CppSQLite3Exception error(nErrCode, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
	close();
	throw error;
}

bool CppSQLite3DB::close()
{
	bool bRet = true;
	if (mpDB)
	{
		//sqlite3_shutdown();
		int nClose = sqlite3_close(mpDB);
		
		if(nClose != SQLITE_OK)
		{
			ASSERT(!"Error closing sqlite db");
			bRet = false;
		}
		mpDB = 0;
	}

	return bRet;
}


CppSQLite3Statement CppSQLite3DB::compileStatement(const TCHAR* szSQL)
{
	checkDB();

	sqlite3_stmt* pVM = compile(szSQL);
	return CppSQLite3Statement(mpDB, pVM);
}


bool CppSQLite3DB::tableExists(const TCHAR* szTable)
{
	TCHAR szSQL[128];

	wsprintf(szSQL,
			_T("select count(*) from sqlite_master where type='table' and name='%s'"),
			szTable);
	int nRet = execScalar(szSQL);
	return (nRet > 0);
}

int CppSQLite3DB::execDMLEx(LPCTSTR szSQL,...)
{
	CString csText;
	va_list vlist;

	ASSERT(AfxIsValidString(szSQL));
	va_start(vlist,szSQL);
	csText.FormatV(szSQL,vlist);
	va_end(vlist);

	return execDML(csText);
}

int CppSQLite3DB::execDML(const TCHAR* szSQL)
{
	checkDB();

	sqlite3_stmt* pVM = compile(szSQL);

	int nRet = sqlite3_step(pVM);

	if (nRet == SQLITE_DONE)
	{
		nRet = sqlite3_changes(mpDB);
		sqlite3_finalize(pVM);
	}	
	else
	{
		nRet = sqlite3_finalize(pVM);
		const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
		throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
	}
	
	return nRet;
}

CppSQLite3Query CppSQLite3DB::execQueryEx(LPCTSTR szSQL,...)
{
	CString csText;
	va_list vlist;

	ASSERT(AfxIsValidString(szSQL));
	va_start(vlist,szSQL);
	csText.FormatV(szSQL,vlist);
	va_end(vlist);

	return execQuery(csText);
}


CppSQLite3Query CppSQLite3DB::execQuery(const TCHAR* szSQL)
{
	checkDB();

	sqlite3_stmt* pVM = compile(szSQL);

	int nRet = sqlite3_step(pVM);

	if (nRet == SQLITE_DONE)
	{
		// no rows
		return CppSQLite3Query(mpDB, pVM, true/*eof*/);
	}
	else if (nRet == SQLITE_ROW)
	{
		// at least 1 row
		return CppSQLite3Query(mpDB, pVM, false/*eof*/);
	}
	else
	{
		nRet = sqlite3_finalize(pVM);
		const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
		throw CppSQLite3Exception(nRet, (TCHAR*)szError, CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


int CppSQLite3DB::execScalarEx(LPCTSTR szSQL,...)
{
	CString csText;
	va_list vlist;

	ASSERT(AfxIsValidString(szSQL));
	va_start(vlist, szSQL);
	csText.FormatV(szSQL,vlist);
	va_end(vlist);

	return execScalar(csText);
}

int CppSQLite3DB::execScalar(const TCHAR* szSQL)
{
	CppSQLite3Query q = execQuery(szSQL);

	if (q.eof() || q.numFields() < 1)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Invalid scalar query"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
	return _ttoi(q.fieldValue(0));
}

sqlite_int64 CppSQLite3DB::lastRowId()
{
	return sqlite3_last_insert_rowid(mpDB);
}


void CppSQLite3DB::setBusyTimeout(int nMillisecs)
{
	mnBusyTimeoutMs = nMillisecs;
	sqlite3_busy_timeout(mpDB, mnBusyTimeoutMs);
}


void CppSQLite3DB::checkDB()
{
	if (!mpDB)
	{
		throw CppSQLite3Exception(CppSQLite3Exception::CppSqliteError,
								_T("Database not open"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}


sqlite3_stmt* CppSQLite3DB::compile(const TCHAR* szSQL)
{
	checkDB();

	const TCHAR* szTail=0;
	sqlite3_stmt* pVM;

	int nRet = sqlite3_prepare16_v2(mpDB, szSQL, -1, &pVM, (const void**)szTail);
	if (nRet != SQLITE_OK)
	{
		const TCHAR* szError = static_cast<const TCHAR*>(sqlite3_errmsg16(mpDB));
		throw CppSQLite3Exception(nRet, (TCHAR*)szError);
	}

	return pVM;
}


////////////////////////////////////////////////////////////////////////////////
// SQLite encode.c reproduced here, containing implementation notes and source
// for sqlite3_encode_binary() and sqlite3_decode_binary() 
////////////////////////////////////////////////////////////////////////////////

/*
** 2002 April 25
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
** This file contains helper routines used to translate binary data into
** a null-terminated string (suitable for use in SQLite) and back again.
** These are convenience routines for use by people who want to store binary
** data in an SQLite database.  The code in this file is not used by any other
** part of the SQLite library.
**
** $Id: CppSQLite3.cpp,v 1.2 2006-09-14 04:56:10 sabrogden Exp $
*/

/*
** How This Encoder Works
**
** The output is allowed to contain any character except 0x27 (') and
** 0x00.  This is accomplished by using an escape character to encode
** 0x27 and 0x00 as a two-byte sequence.  The escape character is always
** 0x01.  An 0x00 is encoded as the two byte sequence 0x01 0x01.  The
** 0x27 character is encoded as the two byte sequence 0x01 0x03.  Finally,
** the escape character itself is encoded as the two-character sequence
** 0x01 0x02.
**
** To summarize, the encoder works by using an escape sequences as follows:
**
**       0x00  ->  0x01 0x01
**       0x01  ->  0x01 0x02
**       0x27  ->  0x01 0x03
**
** If that were all the encoder did, it would work, but in certain cases
** it could double the size of the encoded string.  For example, to
** encode a string of 100 0x27 characters would require 100 instances of
** the 0x01 0x03 escape sequence resulting in a 200-character output.
** We would prefer to keep the size of the encoded string smaller than
** this.
**
** To minimize the encoding size, we first add a fixed offset value to each 
** byte in the sequence.  The addition is modulo 256.  (That is to say, if
** the sum of the original character value and the offset exceeds 256, then
** the higher order bits are truncated.)  The offset is chosen to minimize
** the number of characters in the string that need to be escaped.  For
** example, in the case above where the string was composed of 100 0x27
** characters, the offset might be 0x01.  Each of the 0x27 characters would
** then be converted into an 0x28 character which would not need to be
** escaped at all and so the 100 character input string would be converted
** into just 100 characters of output.  Actually 101 characters of output - 
** we have to record the offset used as the first byte in the sequence so
** that the string can be decoded.  Since the offset value is stored as
** part of the output string and the output string is not allowed to contain
** characters 0x00 or 0x27, the offset cannot be 0x00 or 0x27.
**
** Here, then, are the encoding steps:
**
**     (1)   Choose an offset value and make it the first character of
**           output.
**
**     (2)   Copy each input character into the output buffer, one by
**           one, adding the offset value as you copy.
**
**     (3)   If the value of an input character plus offset is 0x00, replace
**           that one character by the two-character sequence 0x01 0x01.
**           If the sum is 0x01, replace it with 0x01 0x02.  If the sum
**           is 0x27, replace it with 0x01 0x03.
**
**     (4)   Put a 0x00 terminator at the end of the output.
**
** Decoding is obvious:
**
**     (5)   Copy encoded characters except the first into the decode 
**           buffer.  Set the first encoded character aside for use as
**           the offset in step 7 below.
**
**     (6)   Convert each 0x01 0x01 sequence into a single character 0x00.
**           Convert 0x01 0x02 into 0x01.  Convert 0x01 0x03 into 0x27.
**
**     (7)   Subtract the offset value that was the first character of
**           the encoded buffer from all characters in the output buffer.
**
** The only tricky part is step (1) - how to compute an offset value to
** minimize the size of the output buffer.  This is accomplished by testing
** all offset values and picking the one that results in the fewest number
** of escapes.  To do that, we first scan the entire input and count the
** number of occurances of each character value in the input.  Suppose
** the number of 0x00 characters is N(0), the number of occurances of 0x01
** is N(1), and so forth up to the number of occurances of 0xff is N(255).
** An offset of 0 is not allowed so we don't have to test it.  The number
** of escapes required for an offset of 1 is N(1)+N(2)+N(40).  The number
** of escapes required for an offset of 2 is N(2)+N(3)+N(41).  And so forth.
** In this way we find the offset that gives the minimum number of escapes,
** and thus minimizes the length of the output string.
*/

/*
** Encode a binary buffer "in" of size n bytes so that it contains
** no instances of characters '\'' or '\000'.  The output is 
** null-terminated and can be used as a string value in an INSERT
** or UPDATE statement.  Use sqlite3_decode_binary() to convert the
** string back into its original binary.
**
** The result is written into a preallocated output buffer "out".
** "out" must be able to hold at least 2 +(257*n)/254 bytes.
** In other words, the output will be expanded by as much as 3
** bytes for every 254 bytes of input plus 2 bytes of fixed overhead.
** (This is approximately 2 + 1.0118*n or about a 1.2% size increase.)
**
** The return value is the number of characters in the encoded
** string, excluding the "\000" terminator.
*/
int CppSQLite3Binary::encode(const unsigned char *in, int n, unsigned char *out){
  // e: the loop below always picks an offset (its smallest sum is below n); 1 is a valid one
  int i{}, j{}, e{1}, m{};
  int cnt[256];
  if( n<=0 ){
    out[0] = 'x';
    out[1] = 0;
    return 1;
  }
  memset(cnt, 0, sizeof(cnt));
  for(i=n-1; i>=0; i--){ cnt[in[i]]++; }
  m = n;
  for(i=1; i<256; i++){
    int sum;
    if( i=='\'' ) continue;
    sum = cnt[i] + cnt[(i+1)&0xff] + cnt[(i+'\'')&0xff];
    if( sum<m ){
      m = sum;
      e = i;
      if( m==0 ) break;
    }
  }
  out[0] = static_cast<unsigned char>(e); // e is an offset chosen from 1-255
  j = 1;
  // escaped values in escape-code order: 0x00 -> 0x01 0x01, 0x01 -> 0x01 0x02, 0x27 -> 0x01 0x03
  const std::array<int, 3> escaped{ 0, 1, '\'' };
  for(i=0; i<n; i++){
    int c = (in[i] - e)&0xff;
    const auto hit = std::find(escaped.begin(), escaped.end(), c);
    if( hit!=escaped.end() ){
      out[j++] = 1;
      out[j++] = static_cast<unsigned char>(1 + (hit - escaped.begin())); // escape code 1-3
    }else{
      out[j++] = static_cast<unsigned char>(c); // c is masked to 0-255 above
    }
  }
  out[j] = 0;
  return j;
}

/*
** Decode the string "in" into binary data and write it into "out".
** This routine reverses the encoding created by sqlite3_encode_binary().
** The output will always be a few bytes less than the input.  The number
** of bytes of output is returned.  If the input is not a well-formed
** encoding, -1 is returned.
**
** The "in" and "out" parameters may point to the same buffer in order
** to decode a string in place.
*/
int CppSQLite3Binary::decode(const unsigned char *in, unsigned char *out){
  int i, c, e;
  e = *(in++);
  i = 0;
  while( (c = *(in++))!=0 ){
    if( c==1 ){
      c = *(in++);
      if( c==1 ){
        c = 0;
      }else if( c==2 ){
        c = 1;
      }else if( c==3 ){
        c = '\'';
      }else{
        return -1;
      }
    }
    out[i++] = (c + e)&0xff;
  }
  return i;
}

void CppSQLite3Statement::bindInt64(int nParam, const sqlite_int64 nValue)
{
	checkVM();
	int nRes = sqlite3_bind_int64(mpVM, nParam, nValue);

	if (nRes != SQLITE_OK)
	{
		throw CppSQLite3Exception(nRes,
								_T("Error binding int64 param"),
								CppSQLite3Exception::DONT_DELETE_MSG);
	}
}

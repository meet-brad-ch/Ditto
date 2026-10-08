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
#ifndef _CppSQLite3_H_
#define _CppSQLite3_H_

#include "sqlite3mc_amalgamation.h"
#include <cstdio>
#include <cstring>
#include <array>
#include <exception>
#include <mutex>
#include <string>

/**
 * @brief A failed SQLite call or a misuse of the wrapper. A std::exception, so a boundary that
 * catches std::exception (the application start) also catches it; what() is the message in UTF-8.
 */
class CppSQLite3Exception : public std::exception
{
public:
    /** @brief The error code of errors CppSQLite3 itself finds (not SQLite's). */
    static constexpr int CppSqliteError = 1000;

    /** @brief The bDeleteMsg value for a message string that cannot be deleted. */
    static constexpr bool DONT_DELETE_MSG{ false };

    /**
     * @brief Creates the exception; the message is copied as "NAME[code]: text".
     * @param nErrCode the SQLite (or CppSqliteError) error code.
     * @param szErrMess the error text; null for none.
     * @param bDeleteMsg not used (the message is always copied).
     */
    CppSQLite3Exception(const int nErrCode,
                    const TCHAR* szErrMess,
                    bool bDeleteMsg=true);

    /**
     * @brief The error code.
     * @return the SQLite (or CppSqliteError) error code.
     */
    int errorCode() const { return mnErrCode; }

    /**
     * @brief The message.
     * @return "NAME[code]: text"; valid while this exception exists.
     */
    const TCHAR* errorMessage() const { return m_message.c_str(); }

    /**
     * @brief The message for std::exception handlers.
     * @return errorMessage() in UTF-8; valid while this exception exists.
     */
    const char* what() const noexcept override { return m_what.c_str(); }

    /**
     * @brief Tells whether the database could not be used at the moment, as opposed to a damaged
     * file or a schema problem: it is locked or busy (another connection or program holds it), it
     * cannot be opened, read or written (permissions, I/O, a full disk), or SQLite ran out of memory
     * or was interrupted. Such a database must not be treated as corrupt (renamed or deleted).
     * @return true for SQLITE_BUSY, SQLITE_LOCKED, SQLITE_CANTOPEN, SQLITE_IOERR, SQLITE_PERM,
     * SQLITE_READONLY, SQLITE_FULL, SQLITE_NOMEM, SQLITE_INTERRUPT and SQLITE_AUTH (extended codes by
     * their primary code); false for every other code.
     */
    bool isUnavailable() const;

    static const TCHAR* errorCodeAsString(int nErrCode);

private:

    /** @brief One entry of the error code to name table. */
    struct ErrorCodeName
    {
        /** @brief SQLite (or CppSQLite) error code. */
        int code;
        /** @brief Name of the error code, e.g. "SQLITE_BUSY". */
        const TCHAR* name;
    };

    /** @brief Names of the known error codes, used by errorCodeAsString(). */
    static constexpr std::array<ErrorCodeName, 29> m_errorCodeNames
    {{
        { SQLITE_OK,         _T("SQLITE_OK") },
        { SQLITE_ERROR,      _T("SQLITE_ERROR") },
        { SQLITE_INTERNAL,   _T("SQLITE_INTERNAL") },
        { SQLITE_PERM,       _T("SQLITE_PERM") },
        { SQLITE_ABORT,      _T("SQLITE_ABORT") },
        { SQLITE_BUSY,       _T("SQLITE_BUSY") },
        { SQLITE_LOCKED,     _T("SQLITE_LOCKED") },
        { SQLITE_NOMEM,      _T("SQLITE_NOMEM") },
        { SQLITE_READONLY,   _T("SQLITE_READONLY") },
        { SQLITE_INTERRUPT,  _T("SQLITE_INTERRUPT") },
        { SQLITE_IOERR,      _T("SQLITE_IOERR") },
        { SQLITE_CORRUPT,    _T("SQLITE_CORRUPT") },
        { SQLITE_NOTFOUND,   _T("SQLITE_NOTFOUND") },
        { SQLITE_FULL,       _T("SQLITE_FULL") },
        { SQLITE_CANTOPEN,   _T("SQLITE_CANTOPEN") },
        { SQLITE_PROTOCOL,   _T("SQLITE_PROTOCOL") },
        { SQLITE_EMPTY,      _T("SQLITE_EMPTY") },
        { SQLITE_SCHEMA,     _T("SQLITE_SCHEMA") },
        { SQLITE_TOOBIG,     _T("SQLITE_TOOBIG") },
        { SQLITE_CONSTRAINT, _T("SQLITE_CONSTRAINT") },
        { SQLITE_MISMATCH,   _T("SQLITE_MISMATCH") },
        { SQLITE_MISUSE,     _T("SQLITE_MISUSE") },
        { SQLITE_NOLFS,      _T("SQLITE_NOLFS") },
        { SQLITE_AUTH,       _T("SQLITE_AUTH") },
        { SQLITE_FORMAT,     _T("SQLITE_FORMAT") },
        { SQLITE_RANGE,      _T("SQLITE_RANGE") },
        { SQLITE_ROW,        _T("SQLITE_ROW") },
        { SQLITE_DONE,       _T("SQLITE_DONE") },
        { CppSqliteError,    _T("CPPSQLITE_ERROR") },
    }};

    /** @brief The primary codes isUnavailable() answers true for. */
    static constexpr std::array<int, 10> m_unavailableCodes
    {{
        SQLITE_BUSY, SQLITE_LOCKED, SQLITE_CANTOPEN, SQLITE_IOERR, SQLITE_PERM,
        SQLITE_READONLY, SQLITE_FULL, SQLITE_NOMEM, SQLITE_INTERRUPT, SQLITE_AUTH,
    }};

    /** @brief The error code. */
    int mnErrCode{};
    /** @brief "NAME[code]: text" (upstream wrote it into a fixed buffer of 1000 characters, unchecked). */
    std::wstring m_message{};
    /** @brief m_message in UTF-8, for what(). */
    std::string m_what{};
};

class CppSQLite3Query
{
public:

    CppSQLite3Query();

    CppSQLite3Query(const CppSQLite3Query& rQuery);

    CppSQLite3Query(sqlite3* pDB,
				sqlite3_stmt* pVM,
                bool bEof,
                bool bOwnVM=true);

    CppSQLite3Query& operator=(const CppSQLite3Query& rQuery);

    virtual ~CppSQLite3Query();

    int numFields();

    int fieldIndex(const TCHAR* szField);
    const TCHAR* fieldName(int nCol);

    const TCHAR* fieldDeclType(int nCol);
    int fieldDataType(int nCol);

    const TCHAR* fieldValue(int nField);
    const TCHAR* fieldValue(const TCHAR* szField);

    int getIntField(int nField, int nNullValue=0);
    int getIntField(const TCHAR* szField, int nNullValue=0);

    __int64 getInt64Field(int nField, __int64 nNullValue=0);
    __int64 getInt64Field(const TCHAR* szField, __int64 nNullValue=0);

    double getFloatField(int nField, double fNullValue=0.0);
    double getFloatField(const TCHAR* szField, double fNullValue=0.0);

    const TCHAR* getStringField(int nField, const TCHAR* szNullValue=_T(""));
    const TCHAR* getStringField(const TCHAR* szField, const TCHAR* szNullValue=_T(""));

    const unsigned char* getBlobField(int nField, int& nLen);
    const unsigned char* getBlobField(const TCHAR* szField, int& nLen);

	int getBlobFieldSize(const TCHAR* szField);
	int getBlobFieldSize(int nField);

    bool fieldIsNull(int nField);
    bool fieldIsNull(const TCHAR* szField);

    bool eof();

    void nextRow();

    void finalize();

private:

    void checkVM();

	sqlite3* mpDB;
    sqlite3_stmt* mpVM;
    bool mbEof;
    int mnCols;
    bool mbOwnVM;
};

class CppSQLite3Statement
{
public:

    CppSQLite3Statement();

    CppSQLite3Statement(const CppSQLite3Statement& rStatement);

    /**
     * @brief Takes ownership of a compiled statement.
     * @param pDB the connection.
     * @param pVM the compiled statement (finalized by this object).
     * @param pConnectionMutex the connection's lock (CDittoDb), held while the statement steps or
     * resets; null for a connection without one.
     */
    CppSQLite3Statement(sqlite3* pDB, sqlite3_stmt* pVM, std::recursive_mutex* pConnectionMutex);

    virtual ~CppSQLite3Statement();

    CppSQLite3Statement& operator=(const CppSQLite3Statement& rStatement);

    int execDML();

    CppSQLite3Query execQuery();

    void bind(int nParam, const TCHAR* szValue);
    void bind(int nParam, const int nValue);
    void bindInt64(int nParam, const sqlite_int64 nValue);
    void bind(int nParam, const double dwValue);
    void bind(int nParam, const unsigned char* blobValue, int nLen);
    void bindNull(int nParam);

    void reset();

    void finalize();

private:

    void checkDB();
    void checkVM();

    /**
     * @brief Locks the connection for one step or reset, as CDittoDb's execDML/execQuery do.
     * @return the held lock; an empty lock when the connection has none.
     */
    std::unique_lock<std::recursive_mutex> lockConnection() const;

    sqlite3* mpDB;
    sqlite3_stmt* mpVM;
    /** @brief The connection's lock (not owned); null when the connection has none. */
    std::recursive_mutex* mpConnectionMutex{};
};


class CppSQLite3DB
{
public:

    CppSQLite3DB();

    virtual ~CppSQLite3DB();

    // Opens szFile; on failure the connection is closed again and CppSQLite3Exception thrown
    void open(const TCHAR* szFile);

    // Loads a SQLite extension DLL into the open connection (Ditto: ICU_Loader.dll,
    // sqlite3_icu_init). Loading is enabled for the C API only, and only during this call.
    // Throws CppSQLite3Exception with SQLite's message when the extension cannot be loaded.
    void loadExtension(const char* szFile, const char* szEntryPoint);

    bool close();

	bool tableExists(const TCHAR* szTable);

	int execDMLEx(LPCTSTR szSQL,...);
    virtual int execDML(const TCHAR* szSQL);

	CppSQLite3Query execQueryEx(LPCTSTR szSQL,...);
    virtual CppSQLite3Query execQuery(const TCHAR* szSQL);

	int execScalarEx(LPCTSTR szSQL,...);
    virtual int execScalar(const TCHAR* szSQL);

    /**
     * @brief Compiles a statement (under connectionMutex() when there is one); the statement holds
     * that lock while it steps or resets, so it waits for another thread's transaction like
     * execDML/execQuery do.
     * @param szSQL the SQL.
     * @return the statement.
     * @throws CppSQLite3Exception when the connection is closed or the SQL does not compile.
     */
    CppSQLite3Statement compileStatement(const TCHAR* szSQL);

    sqlite_int64 lastRowId();

    void interrupt() { sqlite3_interrupt(mpDB); }

    void setBusyTimeout(int nMillisecs);

    static const TCHAR* SQLiteVersion() { return _T(SQLITE_VERSION); }

    bool IsDatabaseOpen() { return mpDB != NULL; }

    bool DBEncrypted();

protected:
    /**
     * @brief The lock that serializes the work on this connection across threads.
     * @return null: a plain connection has none (CDittoDb overrides this).
     */
    virtual std::recursive_mutex* connectionMutex() { return nullptr; }

private:

    CppSQLite3DB(const CppSQLite3DB& db);
    CppSQLite3DB& operator=(const CppSQLite3DB& db);

    sqlite3_stmt* compile(const TCHAR* szSQL);

    void checkDB();

    // Builds the exception from the connection's last error, closes the connection, throws
    [[noreturn]] void throwAndClose(int nErrCode);

    /**
     * @brief The SQL function regexp(pattern, text) that open() registers: 1 if the case-insensitive
     * std::regex pattern is found in the text, 0 otherwise or for invalid arguments; an invalid pattern
     * is an SQL error (sqlite3_result_error), so the statement fails instead of seeing NULL.
     * @param context the SQLite function context.
     * @param argc the number of arguments.
     * @param values the arguments: pattern, text.
     */
    static void sqlite_regexp(sqlite3_context* context, int argc, sqlite3_value** values);

    sqlite3* mpDB;
    int mnBusyTimeoutMs;
    CString m_dbFile;
};

#endif

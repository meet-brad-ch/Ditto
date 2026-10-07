/**
 * @file TestDatabase.cpp
 * @brief Implements TestDatabase.
 */
#include "stdafx.h"
#include "TestDatabase.h"

TestDatabase::TestDatabase()
	: m_log(std::make_shared<std::vector<CString>>())
{
	const std::shared_ptr<std::vector<CString>> log = m_log;
	m_db = std::make_unique<CDittoDb>([log](const CString& text) { log->push_back(text); });
	m_db->open(_T(":memory:"));
	// the columns of Ditto's CreateDB
	m_db->execDML(_T("CREATE TABLE Main(lID INTEGER PRIMARY KEY AUTOINCREMENT, lDate INTEGER, mText TEXT, ")
		_T("lShortCut INTEGER, lDontAutoDelete INTEGER, CRC INTEGER, bIsGroup INTEGER, lParentID INTEGER, ")
		_T("QuickPasteText TEXT, clipOrder REAL, clipGroupOrder REAL, globalShortCut INTEGER, lastPasteDate INTEGER, ")
		_T("stickyClipOrder REAL, stickyClipGroupOrder REAL, MoveToGroupShortCut INTEGER, GlobalMoveToGroupShortCut INTEGER);"));
	m_db->execDML(_T("CREATE TABLE Data(lID INTEGER PRIMARY KEY AUTOINCREMENT, lParentID INTEGER, strClipBoardFormat TEXT, ooData BLOB);"));
}

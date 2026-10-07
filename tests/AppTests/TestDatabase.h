/**
 * @file TestDatabase.h
 * @brief Declares TestDatabase.
 */
#pragma once

#include "DittoDb.h"

#include <memory>
#include <vector>

/**
 * @brief An in-memory database with Ditto's Main and Data tables, for the app-layer tests.
 */
class TestDatabase
{
public:
	/// Opens the database and creates the tables.
	TestDatabase();

	/**
	 * @brief The connection.
	 * @return The database.
	 */
	CDittoDb& Db() { return *m_db; }

	/**
	 * @brief The errors the database wrote to its log.
	 * @return The log lines.
	 */
	const std::vector<CString>& Log() const { return *m_log; }

private:
	/// The log lines (shared with the log function of m_db).
	std::shared_ptr<std::vector<CString>> m_log;
	/// The connection.
	std::unique_ptr<CDittoDb> m_db;
};

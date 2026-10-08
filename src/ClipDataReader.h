#pragma once

#include "Clip.h"

#include <memory>

class CDittoDb;

/**
 * @brief Reads clip data and the saved clipboard types from the clip database; a failed read is
 *        reported (CErrorReport) and returned as no data.
 *
 * Cheap to create where needed: CClipDataReader(database).GetClipData(...).
 */
class CClipDataReader
{
public:
	/**
	 * @brief Creates the reader.
	 * @param database The clip database; must outlive this object.
	 */
	explicit CClipDataReader(CDittoDb& database);

	/**
	 * @brief Loads the data of one format of a clip into a new global block.
	 * @param parentId The clip.
	 * @param Clip The format to load (m_cfType); m_hgData receives the new block.
	 * @return TRUE when the clip has data in that format; FALSE when not, or when the read failed (reported).
	 */
	BOOL GetClipData(long parentId, CClipFormat& Clip);

	/**
	 * @brief Reads the clipboard types Ditto saves (the Types table); the default set when the table is empty.
	 * @return A new types array; null when the database read failed (reported).
	 */
	std::unique_ptr<CClipTypes> LoadTypesFromDB();

private:
	/** @brief The clip database (not owned). */
	CDittoDb& m_database;
};

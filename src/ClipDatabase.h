#pragma once

#include "..\Shared/ArrayEx.h"

class CClipContext;
class CDittoDb;

/**
 * @brief Group and clip edits on the app's database (CAppServices::Database()): new groups,
 * deleting all clips or some formats of a clip, and a group's path. A database error is shown to
 * the user (CErrorReport) and gives the failure result.
 */
class CClipDatabase
{
public:
	/**
	 * @brief Creates a group.
	 * @param db The clip database.
	 * @param parentID The parent group; 0 for a top level group.
	 * @param text The group name; empty for "NewGroup yy/mm/dd HH:MM:SS".
	 * @return The new group's id; 0 when the insert failed.
	 */
	static long NewGroupID(CDittoDb& db, int parentID = 0, CString text = "");

	/**
	 * @brief Deletes all clips and their data.
	 * @param db The clip database.
	 * @return TRUE on success; FALSE when the delete failed.
	 */
	static BOOL DeleteAllIDs(CDittoDb& db);

	/**
	 * @brief Deletes data formats of a clip and updates the clip's CRC.
	 * @param context The clip services (the database, the clip's save settings for the CRC).
	 * @param parentID The clip.
	 * @param formatIDs The ids of the Data rows to delete; nothing is done when empty.
	 * @return TRUE on success; FALSE when the delete failed.
	 */
	static BOOL DeleteFormats(CClipContext& context, int parentID, ARRAY& formatIDs);

	/**
	 * @brief The path of a group, "Group Path: \\\\top\\...\\group" (at most 100 levels).
	 * @param db The clip database.
	 * @param folderId The group; 0 or less for none.
	 * @return The path; empty for no group or when reading failed.
	 */
	static CString FolderPath(CDittoDb& db, int folderId);
};

#pragma once

#include "IClipAggregator.h"
#include "sqlite\CppSQLite3.h"

class CPopup;
class CAppWindows;
class CClipContext;
class CDittoDb;

class CClipIDs : public CArrayEx<int>
{
public:
// PASTING FUNCTIONS

	/**
	 * @brief Allocates an HGLOBAL of the given format representing the clip ids in this array
	 *        (several clips are joined with the multi-paste separator, reversed when configured).
	 * @param context the clip services (the multi-paste separator and order, the database, the
	 *        registered formats).
	 * @param cfType the clipboard format.
	 * @return the data, or NULL/0 when there is none.
	 */
	HGLOBAL	Render(CClipContext& context, UINT cfType);
	/**
	 * @brief Fills "types" with the Format Types corresponding to the Clip IDs in this array.
	 * @param context the clip services (the database).
	 * @param types receives the formats.
	 */
	void GetTypes(CClipContext& context, CClipTypes& types);
	/**
	 * @brief Adds the cfType data of every clip to Aggregator.
	 * @param context the clip services (the database).
	 * @param Aggregator the aggregator for cfType.
	 * @param cfType the clipboard format.
	 * @param bReverse TRUE to aggregate the clips in reverse order.
	 * @param textOnly a text-only paste (text formats also take file lists).
	 * @return true when a clip added data.
	 */
	bool AggregateData(CClipContext& context, IClipAggregator &Aggregator, UINT cfType, BOOL bReverse, bool textOnly);

// MANAGEMENT FUNCTIONS

	/**
	 * @brief Blindly Moves IDs into the lParentID Group sequentially with the given order
	 *        (i.e. this does not check to see if the IDs' order conflict).
	 * @param context the clip services (the database).
	 * @param lParentID the target group.
	 * @param dFirst not used.
	 * @param dIncrement not used.
	 * @return TRUE on success; FALSE (after showing the error) when a statement failed: the move
	 *         is one transaction, so then no clip was moved.
	 */
	BOOL MoveTo(CClipContext& context, long lParentID, double dFirst = 0, double dIncrement = -1);

	/**
	 * @brief Deletes the clips (a group's children move to the top level), in one transaction.
	 * @param windows the application's windows: told of each deleted clip (after the commit) unless fromClipWindow.
	 * @param fromClipWindow the quick paste window deletes the clips itself.
	 * @param db the database to delete from.
	 * @return TRUE on success; FALSE for no clips, or (after showing the error) when a statement
	 *         failed, and then no clip was deleted.
	 */
	BOOL DeleteIDs(CAppWindows& windows, bool fromClipWindow, CDittoDb& db);

	/**
	 * @brief Exports the clips to a new SQLite file (an existing file is replaced).
	 * @param context the clip services (passed to the exported clips).
	 * @param csFilePath the export file.
	 * @return TRUE on success or when there is nothing to export.
	 */
	BOOL Export(CClipContext& context, CString csFilePath);
	
protected:
	BOOL CreateExportSqliteDB(CppSQLite3DB &db);

private:
	/**
	 * @brief Render's step for several clips: aggregates their cfType data.
	 * @param context the clip services (the database).
	 * @param Aggregator the aggregator for cfType.
	 * @param cfType the clipboard format to render.
	 * @param bReverse TRUE to aggregate the clips in reverse order (the settings' m_bMultiPasteReverse).
	 * @return the aggregated HGLOBAL, or NULL when no clip added data.
	 */
	HGLOBAL RenderAggregated(CClipContext& context, IClipAggregator& Aggregator, UINT cfType, BOOL bReverse);
	/**
	 * @brief GetTypes' step for several clips: fills types with the formats all clips have,
	 * or with the formats of the first clip when they have none in common.
	 * @param context the clip services (the database).
	 * @param types the (empty) list to fill.
	 * @param count the number of clip ids (more than 1).
	 */
	void GetCommonTypes(CClipContext& context, CClipTypes& types, INT_PTR count);
	/**
	 * @brief Tells whether DeleteIDs runs the DELETE of a batch after this index.
	 * @param index the index of the clip id just added.
	 * @param batchCount the batch size.
	 * @return true for every batchCount-th index (index 0 excluded).
	 */
	static bool IsDeleteBatchEnd(INT_PTR index, int batchCount);
	/**
	 * @brief Shows a DeleteIDs progress text when showing is allowed.
	 * @param status the progress popup.
	 * @param bAllowShow true if a Ditto window is in the foreground.
	 * @param text the text to show.
	 */
	static void ShowDeleteStatus(CPopup& status, bool bAllowShow, const CString& text);
	// DeleteIDs' step for one clip: if it is still in Main, moves a group's children to the
	// top level and appends the clip's id to the IN list
	void AddExistingClipToDelete(CppSQLite3DB& db, int clipId, CString& sqlIn);
	/**
	 * @brief Export's work: writes every clip whose Main row and formats load into db.
	 * @param context the clip context (database) to load the clips from.
	 * @param db the open export database.
	 * @return TRUE when every clip was written; FALSE (after showing which clips were skipped)
	 *         when a clip could not be loaded or has no data.
	 * @throws CppSQLite3Exception when writing to db fails.
	 */
	BOOL ExportClips(CClipContext& context, CppSQLite3DB& db);

protected:

};
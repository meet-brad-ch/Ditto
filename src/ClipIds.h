#pragma once

#include "IClipAggregator.h"
#include "sqlite\CppSQLite3.h"

class CPopup;

class CClipIDs : public CArrayEx<int>
{
public:
// PASTING FUNCTIONS

	// allocate an HGLOBAL of the given Format Type representing the Clip IDs in this array.
	HGLOBAL	Render(UINT cfType);
	// Fills "types" with the Format Types corresponding to the Clip IDs in this array.
	void GetTypes(CClipTypes& types);
	bool AggregateData(IClipAggregator &Aggregator, UINT cfType, BOOL bReverse, bool textOnly);

// MANAGEMENT FUNCTIONS

	// Blindly Moves IDs into the lParentID Group sequentially with the given order
	// (i.e. this does not check to see if the IDs' order conflict)
	// if( dIncrement < 0 ), this does not change the order
	BOOL MoveTo(long lParentID, double dFirst = 0, double dIncrement = -1);

	// reorders the "lParentID" Group, inserting before the given id.
	//  if the id cannot be found, this appends the IDs.
//	BOOL ReorderGroupInsert( long lParentID, long lInsertBeforeID = 0 );

	// Empties this array and fills it with the elements of the given group ID
	BOOL LoadElementsOf(int groupId);

	BOOL CopyTo(int parentId);

	BOOL DeleteIDs(bool fromClipWindow, CppSQLite3DB& db);

	BOOL Export(CString csFilePath);
	
protected:
	BOOL CreateExportSqliteDB(CppSQLite3DB &db);

private:
	/**
	 * @brief Render's step for several clips: aggregates their cfType data.
	 * @param Aggregator the aggregator for cfType.
	 * @param cfType the clipboard format to render.
	 * @return the aggregated HGLOBAL, or NULL when no clip added data.
	 */
	HGLOBAL RenderAggregated(IClipAggregator& Aggregator, UINT cfType);
	/**
	 * @brief GetTypes' step for several clips: fills types with the formats all clips have,
	 * or with the formats of the first clip when they have none in common.
	 * @param types the (empty) list to fill.
	 * @param count the number of clip ids (more than 1).
	 */
	void GetCommonTypes(CClipTypes& types, INT_PTR count);
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
	// Export's work: writes every clip whose Main row and formats load into db; TRUE when at
	// least one clip was written
	BOOL ExportClips(CppSQLite3DB& db);

protected:

};
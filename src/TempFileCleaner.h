#pragma once

/** @brief Empties Ditto's own temp folders (received files, drag files, clip compare and edit files). */
class CTempFileCleaner
{
public:
	/**
	 * @brief Deletes the files in the received files, drag files and clip compare folders.
	 * @param checkFileLastAccess TRUE: only files not accessed for an hour; FALSE: all files.
	 */
	static void DeleteDittoTempFiles(BOOL checkFileLastAccess);

	/**
	 * @brief Deletes the files in a folder; does nothing unless the folder is one of Ditto's own
	 * temp folders (\\ReceivedFiles\\, \\DragFiles\\, ClipCompare, EditClips).
	 * @param csDir The folder.
	 * @param checkFileLastAccess TRUE: only files not accessed within lastAccessOffset; FALSE: all files.
	 * @param lastAccessOffset How long a file must be unused before it is deleted.
	 */
	static void DeleteFolderFiles(CString csDir, BOOL checkFileLastAccess, CTimeSpan lastAccessOffset);
};

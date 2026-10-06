/**
 * @file FileDialogPath.h
 * @brief Declares CFileDialogPath.
 */
#pragma once

/**
 * @brief Reads the path a common file dialog (GetOpenFileName / GetSaveFileName) returned.
 *
 * The path is read only up to the dialog's buffer capacity (OPENFILENAME::nMaxFile), so a
 * missing terminator can never lead to a read past the buffer.
 */
class CFileDialogPath
{
public:
	/**
	 * @brief The selected path of a completed file dialog.
	 * @param dialog The OPENFILENAME the dialog filled; lpstrFile holds nMaxFile characters.
	 * @return The path, at most nMaxFile characters long.
	 */
	static CString From(const OPENFILENAME& dialog);
};

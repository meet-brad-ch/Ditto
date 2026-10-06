/**
 * @file FileDialogPath.cpp
 * @brief Implements CFileDialogPath.
 */
#include "stdafx.h"
#include "FileDialogPath.h"

#include <tchar.h>

CString CFileDialogPath::From(const OPENFILENAME& dialog)
{
	const size_t length{ _tcsnlen(dialog.lpstrFile, dialog.nMaxFile) };
	return CString(dialog.lpstrFile, static_cast<int>(length));
}

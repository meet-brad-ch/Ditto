#pragma once

/** @brief File queries and file path text helpers. */
class CFileSystem
{
public:
	/**
	 * @brief Whether a file or folder exists.
	 * @param pszFile The path.
	 * @return TRUE when GetFileAttributes finds it.
	 */
	static BOOL FileExists(LPCTSTR pszFile)
	{
		return (GetFileAttributes(pszFile) != 0xffffffff);
	}

	/**
	 * @brief The size of a file.
	 * @param fileName The file.
	 * @return The size in bytes, or -1 when the file cannot be read.
	 */
	static __int64 FileSize(const TCHAR* fileName);

	/**
	 * @brief The last write time of a file.
	 * @param csFile The file.
	 * @return The FILETIME as a 64 bit number, or 0 when the file is not found.
	 */
	static __int64 GetLastWriteTime(const CString& csFile);

	/**
	 * @brief The folder part of a path, up to and including the last backslash.
	 * @param csFileName The path.
	 * @return The folder; the whole path when it has no backslash.
	 */
	static CString GetFilePath(CString csFileName);

	/**
	 * @brief The file name part of a path, after the last backslash.
	 * @param csFileName The path.
	 * @return The file name; the whole path when it has no backslash.
	 */
	static CString GetFileName(CString csFileName);
};

#include "stdafx.h"
#include "FileSystem.h"
#include <sys/types.h>
#include <sys/stat.h>

__int64 CFileSystem::FileSize(const TCHAR* fileName)
{
	struct _stat64 buf{};
	if (_wstat64((wchar_t const*)fileName, &buf) != 0)
		return -1; // error, could use errno to find out more

	return buf.st_size;
}

__int64 CFileSystem::GetLastWriteTime(const CString& csFile)
{
	__int64 nLastWrite{};
	CFileFind finder;
	const BOOL bResult{ finder.FindFile(csFile) };

	if (bResult)
	{
		finder.FindNextFile();

		FILETIME ft{};
		finder.GetLastWriteTime(&ft);

		memcpy(&nLastWrite, &ft, sizeof(ft));
	}

	return nLastWrite;
}

CString CFileSystem::GetFilePath(CString csFileName)
{
	const long lSlash{ csFileName.ReverseFind('\\') };

	if (lSlash > -1)
	{
		csFileName = csFileName.Left(lSlash + 1);
	}

	return csFileName;
}

CString CFileSystem::GetFileName(CString csFileName)
{
	const long lSlash{ csFileName.ReverseFind('\\') };
	if (lSlash > -1)
	{
		csFileName = csFileName.Right(csFileName.GetLength() - lSlash - 1);
	}

	return csFileName;
}

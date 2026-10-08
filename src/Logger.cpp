#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"

void CLogger::AppendToFile(const TCHAR* fn, const TCHAR* msg)
{
#ifdef _UNICODE
	FILE* file{ _wfopen(fn, _T("a")) };
#else
	FILE* file{ fopen(fn, _T("a")) };
#endif

	ASSERT(file);

	if (file != NULL)
	{
#ifdef _UNICODE
		fwprintf(file, _T("%s"), msg);
#else
		fprintf(file, _T("%s"), msg);
#endif

		fclose(file);
	}
}

void CLogger::Write(const TCHAR* msg, CString csFile, long lLine)
{
	ASSERT(AfxIsValidString(msg));

	SYSTEMTIME st{};
	GetLocalTime(&st);

	CString csText{};
	csText.Format(_T("[%d/%d/%d %02d:%02d:%02d.%03d - "), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	CString csFileLine{};
	csFile = CFileSystem::GetFileName(csFile);
	csFileLine.Format(_T("%s %d] "), csFile.GetString(), lLine);
	csText += csFileLine;

	csText += msg;
	csText += "\n";

	// The documented exception to the access rule: the log is used from every class and thread.
	CGetSetOptions& settings{ theApp.Services().Settings() };

#ifndef _DEBUG
	if (settings.m_outputDebugStringLogging)
#endif
	{
		OutputDebugString(csText);
	}

#ifndef _DEBUG
	if (!settings.m_bEnableDebugLogging)
		return;
#endif

	CString csExeFile{ settings.GetPath(CGetSetOptions::PathLogFile) };
	csExeFile += "Ditto.log";

	// one writer at a time: each line is opened, appended and closed as a whole
	const std::scoped_lock lock{ theApp.Services().State().LogFileLock() };
	AppendToFile(csExeFile, csText);
}

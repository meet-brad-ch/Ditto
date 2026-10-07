#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"

void CLogger::AppendToFile(const TCHAR* fn, const TCHAR* msg)
{
#ifdef _UNICODE
	FILE *file{ _wfopen(fn, _T("a")) };
#else
	FILE *file{ fopen(fn, _T("a")) };
#endif

	ASSERT( file );

	if(file != NULL)
	{
		#ifdef _UNICODE
			fwprintf(file, _T("%s"), msg);
		#else
			fprintf(file, _T("%s"),msg);
		#endif

		fclose(file);
	}
}

void CLogger::Write(const TCHAR* msg, CString csFile, long lLine)
{
	ASSERT(AfxIsValidString(msg));

	SYSTEMTIME st{};
	GetLocalTime(&st);

	CString	csText{};
	csText.Format(_T("[%d/%d/%d %02d:%02d:%02d.%03d - "), st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

	CString csFileLine{};
	csFile = CFileSystem::GetFileName(csFile);
	csFileLine.Format(_T("%s %d] "), csFile.GetString(), lLine);
	csText += csFileLine;

	csText += msg;
	csText += "\n";

#ifndef _DEBUG
	if(CGetSetOptions::m_outputDebugStringLogging)
#endif
	{
		OutputDebugString(csText);
	}

#ifndef _DEBUG
	if(!CGetSetOptions::m_bEnableDebugLogging)
		return;
#endif

	CString csExeFile{ CGetSetOptions::GetPath(CGetSetOptions::PathLogFile) };
	csExeFile += "Ditto.log";

	AppendToFile(csExeFile, csText);
}

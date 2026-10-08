#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"
#include <stdexcept>
#include <string>
#include <vector>

CString CAppVersion::GetVersionString(VersionInfo version)
{
	CString csLine{};
	csLine.Format(_T("%02i.%02i.%02i.%02i"),
		version.Major,
		version.Minor,
		version.Revision,
		version.Build);

	return csLine;
}

VersionInfo CAppVersion::GetRunningVersion(const CString& exeFileName)
{
	// Ditto.exe always carries a version resource: not finding it means a broken build
	const CString csFileName{ exeFileName };

	DWORD dwHandle{};
	const DWORD dwSize{ GetFileVersionInfoSize(csFileName, &dwHandle) };
	if (dwSize == 0)
	{
		throw std::runtime_error("Ditto.exe has no version resource (GetFileVersionInfoSize error " + std::to_string(::GetLastError()) + ")");
	}

	std::vector<BYTE> data(dwSize);
	// The handle parameter of GetFileVersionInfo is ignored and must be 0.
	if (GetFileVersionInfo(csFileName, 0, dwSize, data.data()) == 0)
	{
		throw std::runtime_error("reading Ditto.exe's version resource failed (error " + std::to_string(::GetLastError()) + ")");
	}

	VS_FIXEDFILEINFO* lpFFI{};
	UINT iBuffSize{};
	if (VerQueryValue(data.data(), _T("\\"), reinterpret_cast<LPVOID*>(&lpFFI), &iBuffSize) == 0 || iBuffSize < sizeof(VS_FIXEDFILEINFO))
	{
		throw std::runtime_error("Ditto.exe's version resource has no fixed file info");
	}

	VersionInfo verInfo{};
	verInfo.Major = (lpFFI->dwProductVersionMS >> 16) & 0xffff;
	verInfo.Minor = (lpFFI->dwProductVersionMS >> 0) & 0xffff;
	verInfo.Revision = (lpFFI->dwProductVersionLS >> 16) & 0xffff;
	verInfo.Build = (lpFFI->dwProductVersionLS >> 0) & 0xffff;
	return verInfo;
}

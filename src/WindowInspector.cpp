#include "stdafx.h"
#include "CP_Main.h"
#include "Misc.h"
#include "Path.h"
#include "Tlhelp32.h"

bool CWindowInspector::IsAppWnd( HWND hWnd )
{
	const DWORD dwMyPID{ ::GetCurrentProcessId() };
	DWORD dwTestPID{};
	::GetWindowThreadProcessId( hWnd, &dwTestPID );
	return dwMyPID == dwTestPID;
}

BOOL CALLBACK CWindowInspector::EnumChildWindowsCallback(HWND hWnd, LPARAM lp)
{
	WindowInfo* info{ reinterpret_cast<WindowInfo*>(lp) };
	DWORD pid{};
	GetWindowThreadProcessId(hWnd, &pid);
	if (pid != info->ownerpid)
		info->childpid = pid;
	return TRUE;
}

CString CWindowInspector::UwpAppName(HWND active_window, DWORD ownerpid)
{
	CString uwpAppName{};
	WindowInfo info{};
	info.ownerpid = ownerpid;
	info.childpid = info.ownerpid;
	EnumChildWindows(active_window, EnumChildWindowsCallback, reinterpret_cast<LPARAM>(&info));
	HANDLE active_process{ OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, info.childpid) };
	if (active_process != NULL)
	{
		WCHAR image_name[MAX_PATH] = { 0 };
		DWORD bufsize{ MAX_PATH };
		QueryFullProcessImageName(active_process, 0, image_name, &bufsize);
		CloseHandle(active_process);

		nsPath::CPath path(image_name);
		uwpAppName = path.GetName();
	}

	return uwpAppName;
}

CString CWindowInspector::GetProcessName(HWND hWnd, DWORD processId)
{
	const ULONGLONG startTick{ GetTickCount64() };

	CString	strProcessName{};
	DWORD Id{ processId };
	if (Id == 0)
	{
		GetWindowThreadProcessId(hWnd, &Id);
	}

	HANDLE active_process{ OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, Id) };
	if (active_process != NULL)
	{
		WCHAR image_name[MAX_PATH] = { 0 };
		DWORD bufsize{ MAX_PATH };
		QueryFullProcessImageName(active_process, 0, image_name, &bufsize);
		CloseHandle(active_process);

		nsPath::CPath path(image_name);
		strProcessName = path.GetName();
	}

	if (strProcessName == _T(""))
	{
		CLogger::Log(CStringUtil::Format(_T("failed to get process name from open process, LastError: %d, looping over process names to find process"), GetLastError()));

		PROCESSENTRY32 processEntry = { 0 };

		HANDLE hSnapShot{ CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0) };
		processEntry.dwSize = sizeof(PROCESSENTRY32);

		if (Process32First(hSnapShot, &processEntry))
		{
			do
			{
				if (processEntry.th32ProcessID == Id)
				{
					strProcessName = processEntry.szExeFile;
					break;
				}
			} while (Process32Next(hSnapShot, &processEntry));
		}

		CloseHandle(hSnapShot);
	}

	//uwp apps are wrapped in another app called, if this has focus then try and find the child uwp process
	if (strProcessName == _T("ApplicationFrameHost.exe"))
	{
		strProcessName = UwpAppName(hWnd, Id);
	}

	const ULONGLONG endTick{ GetTickCount64() };
	const ULONGLONG diff{ endTick - startTick };
	if(diff > 5)
	{
		CLogger::Log(CStringUtil::Format(_T("GetProcessName Time (ms): %llu, pid: %d, name: %s"), diff, Id, strProcessName.GetString()));
	}

	return strProcessName;
}

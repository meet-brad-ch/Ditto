#include "stdafx.h"
#include "SystemTheme.h"

BOOL CSystemTheme::DarkAppWindows10Setting()
{
	BOOL darkMode{ false };
	HKEY hkKey{};
	long lResult{ ::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize"), NULL, KEY_READ, &hkKey) };
	if (lResult == ERROR_SUCCESS)
	{
		DWORD buffer{};
		DWORD len{ sizeof(buffer) };
		DWORD type{};

		lResult = ::RegQueryValueEx(hkKey, _T("AppsUseLightTheme"), 0, &type, (LPBYTE)&buffer, &len);

		if (lResult == ERROR_SUCCESS)
		{
			darkMode = (buffer == 0);
		}

		RegCloseKey(hkKey);
	}

	return darkMode;
}

DWORD CSystemTheme::Windows10AccentColor()
{
	DWORD color{ MAXDWORD };
	HKEY hkKey{};
	long lResult{ ::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\DWM"), NULL, KEY_READ, &hkKey) };
	if (lResult == ERROR_SUCCESS)
	{
		DWORD buffer{};
		DWORD len{ sizeof(buffer) };
		DWORD type{};

		lResult = ::RegQueryValueEx(hkKey, _T("ColorizationColor"), 0, &type, (LPBYTE)&buffer, &len);

		if (lResult == ERROR_SUCCESS)
		{
			color = buffer;
		}

		RegCloseKey(hkKey);
	}

	return color;
}

BOOL CSystemTheme::Windows10ColorTitleBar()
{
	BOOL colorTitleBar{ FALSE };
	HKEY hkKey{};
	long lResult{ ::RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\Microsoft\\Windows\\DWM"), NULL, KEY_READ, &hkKey) };
	if (lResult == ERROR_SUCCESS)
	{
		DWORD buffer{};
		DWORD len{ sizeof(buffer) };
		DWORD type{};

		lResult = ::RegQueryValueEx(hkKey, _T("ColorPrevalence"), 0, &type, (LPBYTE)&buffer, &len);

		if (lResult == ERROR_SUCCESS)
		{
			colorTitleBar = (buffer == 1);
		}

		RegCloseKey(hkKey);
	}

	return colorTitleBar;
}

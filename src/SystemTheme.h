#pragma once

/** @brief The Windows 10 personalization settings Ditto follows (dark apps, accent color, colored title bars). */
class CSystemTheme
{
public:
	/**
	 * @brief Whether Windows apps use the dark theme (Personalize\\AppsUseLightTheme is 0).
	 * @return TRUE for dark; FALSE when light or not readable.
	 */
	static BOOL DarkAppWindows10Setting();

	/**
	 * @brief The Windows accent color (DWM\\ColorizationColor).
	 * @return The color as stored (0xAARRGGBB); MAXDWORD when not readable.
	 */
	static DWORD Windows10AccentColor();

	/**
	 * @brief Whether title bars show the accent color (DWM\\ColorPrevalence is 1).
	 * @return TRUE when they do; FALSE otherwise or when not readable.
	 */
	static BOOL Windows10ColorTitleBar();
};

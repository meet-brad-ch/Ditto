#pragma once

#include <vector>

/** @brief Clipboard format ids and the names Ditto stores them under in the database. */
class CClipboardFormats
{
public:
	/**
	 * @brief The standard (system) clipboard formats.
	 * https://learn.microsoft.com/en-us/windows/win32/dataxchg/standard-clipboard-formats
	 * @return The formats.
	 */
	static std::vector<CLIPFORMAT> GetSystemClipFormats();

	/**
	 * @brief The format of a stored name: a standard format's "CF_..." name gives its id, any other
	 * name is registered (RegisterClipboardFormat). Do not change: the names are stored in the database.
	 * @param cbName The name.
	 * @return The format.
	 */
	static CLIPFORMAT GetFormatID(LPCTSTR cbName);

	/**
	 * @brief The stored name of a format: "CF_..." for a standard format, the registered name
	 * otherwise. Do not change: the names are stored in the database.
	 * @param cbType The format.
	 * @return The name; empty when a registered name cannot be read, "ERROR" for format 0.
	 */
	static CString GetFormatName(CLIPFORMAT cbType);
};

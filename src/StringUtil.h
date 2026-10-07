#pragma once

/** @brief Text helpers: printf-style formatting, word counting and GUID text. */
class CStringUtil
{
public:
	/**
	 * @brief Formats a text printf-style (CString::FormatV).
	 * @param pszFormat The format text.
	 * @param ... The values the format text names.
	 * @return The formatted text.
	 */
	static CString Format(const TCHAR* pszFormat, ...);

	/**
	 * @brief Counts the words of a text; spaces, tabs, carriage returns and line feeds separate words.
	 * @param text The text.
	 * @return The number of words.
	 */
	static int WordCount(const CString& text);

	/**
	 * @brief Creates a new GUID as lower case text without braces (8-4-4-4-12 hex digits).
	 * @return The GUID text.
	 * @throws std::runtime_error when CoCreateGuid fails.
	 */
	static CString NewGuidString();
};

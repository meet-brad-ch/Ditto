/**
 * @file TextTransforms.h
 * @brief Declares DittoCore::TextTransforms.
 */
#pragma once

#include <string>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief The special-paste changes of UTF-16 text that need no case mapping.
	 */
	class TextTransforms
	{
	public:
		/**
		 * @brief Drops every character outside ASCII.
		 * @param text The text.
		 * @return The ASCII characters of @p text.
		 */
		static std::wstring AsciiOnly(std::wstring_view text);

		/**
		 * @brief Turns every line break (CRLF, CR or LF) into one space.
		 * @param text The text.
		 * @return The text on one line.
		 */
		static std::wstring RemoveLineFeeds(std::wstring_view text);

		/**
		 * @brief Appends line breaks.
		 * @param text The text.
		 * @param count How many CRLFs to append.
		 * @return The text.
		 */
		static std::wstring AddLineFeeds(std::wstring_view text, int count);

		/**
		 * @brief Appends a line with a date and time.
		 * @param text The text.
		 * @param formattedTime The date and time, already formatted.
		 * @return The text, a CRLF and @p formattedTime.
		 */
		static std::wstring AddDateTime(std::wstring_view text, std::wstring_view formattedTime);

		/**
		 * @brief Removes white space (spaces, tabs, line breaks) at both ends.
		 * @param text The text.
		 * @return The trimmed text.
		 */
		static std::wstring Trim(std::wstring_view text);

		/**
		 * @brief Turns Windows paths into POSIX paths: trims the text, turns each drive "X:\"
		 *        into "/x/" and every backslash into a slash.
		 * @param text The text.
		 * @return The text with POSIX paths.
		 */
		static std::wstring PosixifyPaths(std::wstring_view text);

	private:
		/**
		 * @brief Whether a drive ("C:\") starts at a position.
		 * @param text The text.
		 * @param pos The position.
		 * @return True when a letter, a colon and a backslash start at @p pos.
		 */
		static bool IsDriveAt(std::wstring_view text, std::size_t pos);
	};
}

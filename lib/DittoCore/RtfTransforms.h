/**
 * @file RtfTransforms.h
 * @brief Declares DittoCore::RtfTransforms.
 */
#pragma once

#include <string>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief The special-paste changes that also apply to a clip's RTF.
	 */
	class RtfTransforms
	{
	public:
		/**
		 * @brief Turns paragraph and line breaks (`\par`, `\line`) into spaces.
		 * @param rtf The RTF document.
		 * @return The document.
		 */
		static std::string RemoveLineFeeds(std::string_view rtf);

		/**
		 * @brief Appends empty paragraphs before the document's closing brace.
		 * @param rtf The RTF document.
		 * @param count How many paragraph breaks to add.
		 * @return The document; unchanged when it has no closing brace.
		 */
		static std::string AddLineFeeds(std::string_view rtf, int count);

		/**
		 * @brief Appends an empty paragraph and a paragraph with a date and time before the
		 *        document's closing brace.
		 * @param rtf The RTF document.
		 * @param formattedTime The date and time, already formatted; it is escaped as RTF.
		 * @return The document; unchanged when it has no closing brace.
		 */
		static std::string AddDateTime(std::string_view rtf, std::wstring_view formattedTime);

	private:
		/**
		 * @brief Inserts text before the last closing brace.
		 * @param rtf The RTF document.
		 * @param text The RTF to insert.
		 * @return The document; unchanged when it has no closing brace.
		 */
		static std::string InsertBeforeEnd(std::string_view rtf, std::string_view text);

		/**
		 * @brief Replaces every occurrence of a string.
		 * @param text The text, changed in place.
		 * @param from What to replace.
		 * @param to The replacement.
		 */
		static void ReplaceAll(std::string& text, std::string_view from, std::string_view to);
	};
}

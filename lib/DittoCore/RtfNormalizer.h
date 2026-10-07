/**
 * @file RtfNormalizer.h
 * @brief Declares DittoCore::RtfNormalizer.
 */
#pragma once

#include <string>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief Removes the parts of an RTF document that Word and Outlook change on every copy, so
	 *        two copies of the same text get the same CRC (duplicate detection).
	 *
	 * The rules are upstream Ditto's, kept exactly: CRCs of clips already in a database were
	 * computed with them, and a changed rule would stop those clips from matching new copies.
	 */
	class RtfNormalizer
	{
	public:
		/**
		 * @brief Normalizes an RTF document.
		 *
		 * Removes the first `{\*\datastore ...}` group, every `\rsid<digits>` and
		 * `\insrsid<digits>`, and every `\mdispDef1`.
		 * @param rtf The document.
		 * @return The normalized document.
		 */
		static std::string Normalize(std::string_view rtf);

	private:
		/**
		 * @brief Removes the first group that starts with @p section, up to its matching brace.
		 * @param rtf The document, changed in place.
		 * @param section The start of the group, including its opening brace.
		 */
		static void RemoveSection(std::string& rtf, std::string_view section);

		/**
		 * @brief Removes every occurrence of a control word.
		 *
		 * An occurrence right after a backslash is kept. With @p trailingDigits, only occurrences
		 * followed by digits are removed (with the digits), and the search stops at an occurrence
		 * whose digits run to the end of the document.
		 * @param rtf The document, changed in place.
		 * @param word The control word, including its backslash.
		 * @param trailingDigits Whether the word takes a number.
		 */
		static void DeleteControlWord(std::string& rtf, std::string_view word, bool trailingDigits);

		/**
		 * @brief The end of the control word at @p start.
		 * @param rtf The document.
		 * @param start Where the word starts.
		 * @param word The control word.
		 * @param trailingDigits Whether the word takes a number.
		 * @return The position after the word and its digits; npos when its digits run to the end
		 *         of the document; @p start + the word's length when a number was expected but none follows.
		 */
		static std::size_t ControlWordEnd(const std::string& rtf, std::size_t start, std::string_view word, bool trailingDigits);
	};
}

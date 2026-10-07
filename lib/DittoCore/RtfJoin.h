/**
 * @file RtfJoin.h
 * @brief Declares DittoCore::RtfJoin.
 */
#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace DittoCore
{
	/**
	 * @brief Joins RTF documents into one, with a separator between them (multi-clip paste).
	 *
	 * The result is the first document's outer group, holding the first document's content and
	 * then, for each further document, the separator and that document's content without its
	 * own `{\rtf1` and closing brace.
	 */
	class RtfJoin
	{
	public:
		/**
		 * @brief Starts a join.
		 * @param separator The text put between documents; it is escaped as RTF.
		 */
		explicit RtfJoin(std::wstring_view separator);

		/**
		 * @brief Adds the next document.
		 * @param document An RTF document; anything after its last closing brace is ignored.
		 * @throws ClipboardFormatError When it does not start with `{\rtf1` or has no closing brace.
		 */
		void Add(std::string_view document);

		/**
		 * @brief The joined document.
		 * @return The RTF text.
		 * @throws ClipboardFormatError When no document was added.
		 */
		std::string Result() const;

		/**
		 * @brief Escapes text as RTF: backslash and braces are escaped, line breaks become
		 *        `\par`, tabs `\tab`, and characters outside ASCII `\uN?`.
		 * @param text The text.
		 * @return The RTF.
		 */
		static std::string Escape(std::wstring_view text);

	private:
		/// The start of every RTF document.
		static constexpr std::string_view RtfStart{ "{\\rtf1" };

		/// The escaped separator.
		std::string m_separator;
		/// The content of each document, without `{\rtf1` and the closing brace.
		std::vector<std::string> m_contents;

		/**
		 * @brief Escapes one character that is not part of a line break.
		 * @param c The character.
		 * @return The RTF.
		 */
		static std::string EscapeCharacter(wchar_t c);
	};
}

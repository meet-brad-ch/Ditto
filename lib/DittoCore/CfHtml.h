/**
 * @file CfHtml.h
 * @brief Declares DittoCore::CfHtml.
 */
#pragma once

#include "CfHtmlFragment.h"

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief Reads and writes the CF_HTML clipboard format ("HTML Format").
	 *
	 * A CF_HTML block is a header of "Name:value" lines followed by HTML. StartFragment and
	 * EndFragment are byte offsets into the whole UTF-8 block; the fragment between them is the
	 * copied HTML.
	 */
	class CfHtml
	{
	public:
		/**
		 * @brief Parses a CF_HTML block.
		 * @param block The block's bytes; the text ends at the first null or the end of the block.
		 * @return The version, source URL and fragment.
		 * @throws ClipboardFormatError When StartFragment or EndFragment is missing, not a number,
		 *         reversed or past the end of the text.
		 */
		static CfHtmlFragment Parse(std::span<const std::byte> block);

		/**
		 * @brief Builds a CF_HTML block around a fragment.
		 * @param fragment The fragment, UTF-8.
		 * @param version The Version value; "0.9" when empty.
		 * @param sourceUrl The SourceURL value; the header line is left out when empty.
		 * @return The block without a terminating null; its offsets count bytes.
		 */
		static std::string Build(const std::string& fragment, const std::string& version, const std::string& sourceUrl);

		/**
		 * @brief Turns plain text into HTML for use inside a fragment (e.g. the multi-paste separator).
		 * @param text The text.
		 * @return The text as UTF-8 with &amp;, &lt;, &gt; and &quot; escaped and each line break as &lt;br&gt;.
		 */
		static std::string HtmlFromText(std::wstring_view text);

	private:
		/// The CF_HTML header fields Ditto uses.
		struct Header
		{
			/// The Version value.
			std::string version{};
			/// The SourceURL value.
			std::string sourceUrl{};
			/// The StartFragment byte offset, when the header has one.
			std::optional<std::size_t> startFragment{};
			/// The EndFragment byte offset, when the header has one.
			std::optional<std::size_t> endFragment{};
		};

		/**
		 * @brief Reads the "Name:value" lines before the HTML.
		 * @param text The whole block as text.
		 * @return The fields found.
		 * @throws ClipboardFormatError When an offset field is not a number.
		 */
		static Header ReadHeader(const std::string& text);

		/**
		 * @brief Stores one header field when Ditto uses it.
		 * @param header The header being read.
		 * @param name The field name.
		 * @param value The field value.
		 * @throws ClipboardFormatError When an offset field is not a number.
		 */
		static void ApplyField(Header& header, std::string_view name, std::string_view value);

		/**
		 * @brief Parses a byte offset (decimal digits, trailing spaces allowed).
		 * @param value The field value.
		 * @return The offset.
		 * @throws ClipboardFormatError When the value is not a non-negative number.
		 */
		static std::size_t ParseOffset(std::string_view value);

		/**
		 * @brief Removes spaces, tabs and line breaks at both ends.
		 * @param text The text.
		 * @return The trimmed text.
		 */
		static std::string Trim(std::string_view text);

		/**
		 * @brief The HTML for one character of plain text.
		 * @param c The character.
		 * @return Its entity for &amp; &lt; &gt; &quot;, &lt;br&gt; for a line break, else the character.
		 */
		static std::wstring Escape(wchar_t c);

		/**
		 * @brief Converts UTF-16 to UTF-8.
		 * @param text The text.
		 * @return The UTF-8 bytes.
		 * @throws ClipboardFormatError When Windows cannot convert the text.
		 */
		static std::string ToUtf8(const std::wstring& text);
	};
}

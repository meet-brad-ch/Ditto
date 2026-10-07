/**
 * @file TextJoin.h
 * @brief Declares and implements DittoCore::TextJoin.
 */
#pragma once

#include <span>
#include <string>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief Joins the texts of several clips with a separator between them (multi-clip paste of
	 *        CF_TEXT or CF_UNICODETEXT).
	 * @tparam CharT char for ANSI text, wchar_t for UTF-16 text.
	 */
	template <typename CharT>
	class TextJoin
	{
	public:
		/**
		 * @brief Starts a join.
		 * @param separator The text put between two items.
		 */
		explicit TextJoin(std::basic_string_view<CharT> separator)
			: m_separator(separator)
		{
		}

		/**
		 * @brief Adds an item; the separator goes before every item but the first.
		 * @param text The item's text.
		 */
		void Add(std::basic_string_view<CharT> text)
		{
			if (m_count > 0)
			{
				m_text += m_separator;
			}
			m_text += text;
			m_count++;
		}

		/**
		 * @brief Adds one item made of lines, each followed by CRLF (a file list as text).
		 * @param lines The lines.
		 * @return False, and nothing is added, when there are no lines.
		 */
		bool AddLines(std::span<const std::basic_string<CharT>> lines)
		{
			if (lines.empty())
			{
				return false;
			}
			std::basic_string<CharT> item;
			for (const std::basic_string<CharT>& line : lines)
			{
				item += line;
				item += static_cast<CharT>('\r');
				item += static_cast<CharT>('\n');
			}
			Add(item);
			return true;
		}

		/**
		 * @brief The joined text.
		 * @return The text, without a terminating null.
		 */
		const std::basic_string<CharT>& Result() const noexcept
		{
			return m_text;
		}

	private:
		/// The text put between two items.
		std::basic_string<CharT> m_separator;
		/// The text joined so far.
		std::basic_string<CharT> m_text;
		/// The number of items added.
		std::size_t m_count{};
	};
}

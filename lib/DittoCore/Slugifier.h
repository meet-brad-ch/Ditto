/**
 * @file Slugifier.h
 * @brief Declares DittoCore::Slugifier.
 */
#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace DittoCore
{
	/**
	 * @brief Turns text into a URL slug: accented and non-Latin letters become ASCII, the text is
	 *        lowered, other characters are dropped, and the words are joined with a separator
	 *        ("Ünïcode Text!" gives "unicode-text").
	 */
	class Slugifier
	{
	public:
		/**
		 * @brief Slugifies a text.
		 * @param text The text.
		 * @param separator Put between words; a run of white space and separator characters
		 *        becomes one separator.
		 * @return The slug, with no separator at either end.
		 */
		static std::wstring Slugify(std::wstring_view text, std::wstring_view separator);

	private:
		/**
		 * @brief The ASCII spelling of the letters and symbols the slug keeps.
		 * @return The table, built once.
		 * @throws std::logic_error When a table key is not one character (a programming error).
		 */
		static const std::unordered_map<wchar_t, std::wstring_view>& Transliterations();

		/**
		 * @brief Transliterates, lowers ASCII letters and drops characters a slug cannot hold.
		 * @param text The text.
		 * @return Lower-case ASCII letters, digits, '-' and white space.
		 */
		static std::wstring ToSlugCharacters(std::wstring_view text);

		/**
		 * @brief Whether a slug keeps a character.
		 * @param c A lower-case character.
		 * @return True for a-z, 0-9, '-' and white space.
		 */
		static bool IsSlugCharacter(wchar_t c);
	};
}

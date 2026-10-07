/**
 * @file Typoglycemia.h
 * @brief Declares DittoCore::Typoglycemia.
 */
#pragma once

#include "IRandomRange.h"

#include <string>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief Shuffles the inner letters of each word, keeping its first and last letter and any
	 *        trailing '.', '!' or '?' in place ("typoglycemia" text stays readable).
	 */
	class Typoglycemia
	{
	public:
		/**
		 * @brief Shuffles every space-separated word whose letters, without trailing punctuation,
		 *        are more than 3; the spaces stay as they are.
		 * @param text The text.
		 * @param random The random source.
		 * @return The shuffled text.
		 */
		static std::wstring Scramble(std::wstring_view text, IRandomRange& random);

	private:
		/**
		 * @brief Shuffles the inner letters of one word.
		 * @param word The word, changed in place.
		 * @param random The random source.
		 */
		static void ScrambleWord(std::wstring& word, IRandomRange& random);
	};
}

/**
 * @file Typoglycemia.cpp
 * @brief Implements DittoCore::Typoglycemia.
 */
#include "Typoglycemia.h"

#include <utility>

namespace DittoCore
{
	std::wstring Typoglycemia::Scramble(std::wstring_view text, IRandomRange& random)
	{
		std::wstring result;
		std::wstring word;
		for (const wchar_t c : text)
		{
			if (c == L' ')
			{
				ScrambleWord(word, random);
				result += word;
				result += c;
				word.clear();
			}
			else
			{
				word += c;
			}
		}
		ScrambleWord(word, random);
		result += word;
		return result;
	}

	void Typoglycemia::ScrambleWord(std::wstring& word, IRandomRange& random)
	{
		std::size_t end = word.size();
		while (end > 0 && (word[end - 1] == L'.' || word[end - 1] == L'!' || word[end - 1] == L'?'))
		{
			end--;
		}
		if (end <= 3)
		{
			return;
		}
		const int lastInner = static_cast<int>(end) - 2;
		for (int i = 1; i <= lastInner; i++)
		{
			// at() rejects a value outside the range instead of writing past the word
			std::swap(word[i], word.at(static_cast<std::size_t>(random.Next(1, lastInner))));
		}
	}
}

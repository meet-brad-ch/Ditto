/**
 * @file SearchCondition.cpp
 * @brief Implements DittoCore::SearchCondition.
 */
#include "SearchCondition.h"

#include <cwctype>

namespace DittoCore
{
	std::wstring SearchCondition::Build(const std::wstring& column, const std::wstring& search, const Options& options)
	{
		std::wstring text{ search };
		ReplaceAll(text, L"'", L"''");

		std::wstring where{};
		if (options.mode == Mode::Regex)
		{
			// SQLite's ICU regexp() matches the whole text: (?s:.*) on both sides finds the pattern
			// anywhere, and (?i) makes the match case-insensitive when the option is set
			const std::wstring caseFlag{ options.regexCaseInsensitive ? L"(?i)" : L"" };
			where = column + L" REGEXP '" + caseFlag + L"(?s:.*)(?:" + Trim(text) + L")(?s:.*)'";
		}
		else if (options.mode == Mode::Simple)
		{
			Pending pending{};
			AddLikeTerm(where, column, text, pending);
		}
		else
		{
			AddKeywordSearch(where, column, text);
		}
		return where;
	}

	void SearchCondition::AddKeywordSearch(std::wstring& where, const std::wstring& column, std::wstring text)
	{
		ReplaceAll(text, L"[", L" ");
		ReplaceAll(text, L"]", L" ");
		ReplaceAll(text, L"*", L"%");

		Pending pending{};
		std::wstring word{};
		bool inQuotes{};
		for (const wchar_t c : text)
		{
			if (c == L'"')
			{
				inQuotes = !inQuotes;
			}
			else if (c == L' ' && !inQuotes)
			{
				AddWord(where, column, word, pending);
				word.clear();
			}
			else
			{
				word += c;
			}
		}

		// the last word is always a term, also a keyword
		if (!word.empty())
		{
			AddLikeTerm(where, column, word, pending);
		}
	}

	void SearchCondition::AddWord(std::wstring& where, const std::wstring& column, const std::wstring& word, Pending& pending)
	{
		const Keyword keyword{ ToKeyword(word) };
		if (keyword == Keyword::Not)
		{
			pending.negation = keyword;
		}
		else if (keyword != Keyword::None)
		{
			pending.join = keyword;
		}
		else
		{
			AddLikeTerm(where, column, word, pending);
		}
	}

	void SearchCondition::AddLikeTerm(std::wstring& where, const std::wstring& column, const std::wstring& word, Pending& pending)
	{
		const std::wstring term{ LikeTerm(column, Trim(word), pending.negation) };
		if (where.empty())
		{
			where = term;
		}
		else
		{
			where += KeywordText(pending.join) + term;
		}
		pending = Pending{};
	}

	std::wstring SearchCondition::LikeTerm(const std::wstring& column, const std::wstring& text, Keyword negation)
	{
		const std::wstring start{ column + KeywordText(negation) + L"LIKE '%" };
		if (text.find(L'%') == std::wstring::npos)
		{
			return start + text + L"%'";
		}

		// with ESCAPE '\' a backslash escapes the next character, so the user's backslashes are
		// doubled first (before, "50% C:\temp" searched for "50% C:temp")
		std::wstring escaped{ text };
		ReplaceAll(escaped, L"\\", L"\\\\");
		ReplaceAll(escaped, L"%", L"\\%");
		return start + escaped + L"%' ESCAPE '\\'";
	}

	SearchCondition::Keyword SearchCondition::ToKeyword(const std::wstring& word)
	{
		std::wstring upper{ word };
		for (wchar_t& c : upper)
		{
			c = static_cast<wchar_t>(std::towupper(c));
		}

		if (upper == L"NOT" || upper == L"!")
		{
			return Keyword::Not;
		}
		if (upper == L"OR")
		{
			return Keyword::Or;
		}
		return upper == L"AND" ? Keyword::And : Keyword::None;
	}

	const wchar_t* SearchCondition::KeywordText(Keyword keyword)
	{
		switch (keyword)
		{
		case Keyword::Not:
			return L" NOT ";
		case Keyword::And:
			return L" AND ";
		case Keyword::Or:
			return L" OR ";
		case Keyword::None:
			break;
		}
		return L" ";
	}

	void SearchCondition::ReplaceAll(std::wstring& text, const std::wstring& from, const std::wstring& to)
	{
		std::size_t position{ text.find(from) };
		while (position != std::wstring::npos)
		{
			text.replace(position, from.size(), to);
			position = text.find(from, position + to.size());
		}
	}

	std::wstring SearchCondition::Trim(const std::wstring& text)
	{
		std::size_t first{};
		while (first < text.size() && std::iswspace(text[first]))
		{
			first++;
		}
		std::size_t last{ text.size() };
		while (last > first && std::iswspace(text[last - 1]))
		{
			last--;
		}
		return text.substr(first, last - first);
	}
}

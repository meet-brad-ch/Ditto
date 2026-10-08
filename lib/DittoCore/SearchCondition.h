/**
 * @file SearchCondition.h
 * @brief Declares DittoCore::SearchCondition.
 */
#pragma once

#include <string>

namespace DittoCore
{
	/**
	 * @brief Builds the SQL condition of the clip search for one column (Ditto's search box).
	 *
	 * The search text goes into SQL string literals, so its single quotes are doubled. Three modes:
	 * - Keywords: words separated by spaces (quotes keep spaces in a word), each a LIKE term
	 *   joined with AND; the words NOT (or !), OR and AND set the operator of the next term;
	 *   '[' and ']' become spaces and '*' becomes '%'.
	 * - Simple: the whole text is one LIKE term.
	 * - Regex: the whole text is one REGEXP term, found anywhere in the column; optionally
	 *   case-insensitive.
	 *
	 * A term that contains '%' matches it literally: '%' is escaped with '\\' and the term gets
	 * ESCAPE '\\'; a '\\' in such a term is escaped too, so it still matches itself.
	 */
	class SearchCondition
	{
	public:
		/** @brief How the search text is read. */
		enum class Mode
		{
			/** @brief Words with NOT/OR/AND (the default search). */
			Keywords,
			/** @brief The whole text as one LIKE term. */
			Simple,
			/** @brief The whole text as one regular expression. */
			Regex
		};

		/** @brief The search options from the settings. */
		struct Options
		{
			/** @brief How the search text is read. */
			Mode mode{ Mode::Keywords };
			/** @brief Regex mode: match without regard to case. */
			bool regexCaseInsensitive{};
		};

		/**
		 * @brief The condition for a column.
		 * @param column The SQL column, e.g. Main.mText (inserted as it is).
		 * @param search The search text as typed.
		 * @param options The search options.
		 * @return The terms joined with their operators, without enclosing parentheses; empty when
		 *         the keyword search has no word.
		 */
		static std::wstring Build(const std::wstring& column, const std::wstring& search, const Options& options);

	private:
		/** @brief A search keyword, or none. */
		enum class Keyword
		{
			None,
			Not,
			And,
			Or
		};

		/** @brief The operators waiting for the next term of a keyword search. */
		struct Pending
		{
			/** @brief Not when the next term is negated, else None. */
			Keyword negation{ Keyword::None };
			/** @brief The operator that joins the next term to the ones before. */
			Keyword join{ Keyword::And };
		};

		/**
		 * @brief The keyword search: splits the text into words and adds them.
		 * @param where The condition so far; the terms are appended.
		 * @param column The SQL column.
		 * @param text The search text, quotes already doubled.
		 */
		static void AddKeywordSearch(std::wstring& where, const std::wstring& column, std::wstring text);

		/**
		 * @brief A finished word of the keyword search: a keyword sets the pending operator, any
		 *        other word (also an empty one) is added as a term.
		 * @param where The condition so far.
		 * @param column The SQL column.
		 * @param word The word.
		 * @param pending The pending operators.
		 */
		static void AddWord(std::wstring& where, const std::wstring& column, const std::wstring& word, Pending& pending);

		/**
		 * @brief Appends a LIKE term with the pending operators and resets them.
		 * @param where The condition so far.
		 * @param column The SQL column.
		 * @param word The term's text (surrounding white space is removed).
		 * @param pending The pending operators.
		 */
		static void AddLikeTerm(std::wstring& where, const std::wstring& column, const std::wstring& word, Pending& pending);

		/**
		 * @brief The LIKE term "column[ NOT ]LIKE '%text%'", with ESCAPE '\\' when text has a '%'.
		 * @param column The SQL column.
		 * @param text The term's text.
		 * @param negation Not for NOT LIKE.
		 * @return The term.
		 */
		static std::wstring LikeTerm(const std::wstring& column, const std::wstring& text, Keyword negation);

		/**
		 * @brief The keyword a word stands for (case-insensitive: NOT or !, AND, OR).
		 * @param word The word.
		 * @return The keyword; None for any other word.
		 */
		static Keyword ToKeyword(const std::wstring& word);

		/**
		 * @brief The SQL text of an operator.
		 * @param keyword The operator.
		 * @return " NOT ", " AND ", " OR ", or " " for None.
		 */
		static const wchar_t* KeywordText(Keyword keyword);

		/**
		 * @brief Replaces every occurrence of a text.
		 * @param text The text to change.
		 * @param from The text to replace (not empty).
		 * @param to The replacement.
		 */
		static void ReplaceAll(std::wstring& text, const std::wstring& from, const std::wstring& to);

		/**
		 * @brief The text without leading and trailing white space.
		 * @param text The text.
		 * @return The trimmed text.
		 */
		static std::wstring Trim(const std::wstring& text);
	};
}

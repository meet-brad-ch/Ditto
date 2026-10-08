/**
 * @file SearchConditionTests.cpp
 * @brief Unit tests for DittoCore::SearchCondition (the clip search SQL of CFormatSQL).
 */
#include "SearchCondition.h"

#include <gtest/gtest.h>

#include <string>

using DittoCore::SearchCondition;

namespace
{
	SearchCondition::Options WithMode(SearchCondition::Mode mode)
	{
		SearchCondition::Options options{};
		options.mode = mode;
		return options;
	}

	std::wstring Keywords(const std::wstring& search)
	{
		return SearchCondition::Build(L"Main.mText", search, WithMode(SearchCondition::Mode::Keywords));
	}

	std::wstring Simple(const std::wstring& search)
	{
		return SearchCondition::Build(L"Main.mText", search, WithMode(SearchCondition::Mode::Simple));
	}
}

TEST(SearchCondition, KeywordWordsAreJoinedWithAnd)
{
	EXPECT_EQ(Keywords(L"foo bar"), L"Main.mText LIKE '%foo%' AND Main.mText LIKE '%bar%'");
}

TEST(SearchCondition, KeywordOperatorsApplyToTheNextWord)
{
	EXPECT_EQ(Keywords(L"foo or bar"), L"Main.mText LIKE '%foo%' OR Main.mText LIKE '%bar%'");
	EXPECT_EQ(Keywords(L"not foo"), L"Main.mText NOT LIKE '%foo%'");
	EXPECT_EQ(Keywords(L"foo ! bar"), L"Main.mText LIKE '%foo%' AND Main.mText NOT LIKE '%bar%'");
}

TEST(SearchCondition, QuotesKeepSpacesInAWord)
{
	EXPECT_EQ(Keywords(L"\"foo bar\""), L"Main.mText LIKE '%foo bar%'");
}

TEST(SearchCondition, SingleQuotesAreDoubled)
{
	EXPECT_EQ(Simple(L"it's"), L"Main.mText LIKE '%it''s%'");
	EXPECT_EQ(Keywords(L"it's"), L"Main.mText LIKE '%it''s%'");
}

TEST(SearchCondition, SimpleSearchIsOneTrimmedTerm)
{
	EXPECT_EQ(Simple(L"  not foo or bar "), L"Main.mText LIKE '%not foo or bar%'");
}

TEST(SearchCondition, PercentIsMatchedLiterally)
{
	EXPECT_EQ(Simple(L"50%"), L"Main.mText LIKE '%50\\%%' ESCAPE '\\'");
	// the keyword search turns * into % first
	EXPECT_EQ(Keywords(L"a*b"), L"Main.mText LIKE '%a\\%b%' ESCAPE '\\'");
}

// Regression: with ESCAPE '\' a backslash of the search escaped the next character, so
// "50% C:\temp" searched for "50% C:temp".
TEST(SearchCondition, BackslashIsDoubledWhenTheTermIsEscaped)
{
	EXPECT_EQ(Simple(L"50% C:\\temp"), L"Main.mText LIKE '%50\\% C:\\\\temp%' ESCAPE '\\'");
}

TEST(SearchCondition, BackslashStaysWithoutEscape)
{
	EXPECT_EQ(Simple(L"C:\\temp"), L"Main.mText LIKE '%C:\\temp%'");
}

TEST(SearchCondition, RegexFindsThePatternAnywhere)
{
	SearchCondition::Options options{ WithMode(SearchCondition::Mode::Regex) };
	EXPECT_EQ(SearchCondition::Build(L"Main.mText", L" a+b ", options), L"Main.mText REGEXP '(?s:.*)(?:a+b)(?s:.*)'");

	options.regexCaseInsensitive = true;
	EXPECT_EQ(SearchCondition::Build(L"Main.mText", L"a+b", options), L"Main.mText REGEXP '(?i)(?s:.*)(?:a+b)(?s:.*)'");
}

TEST(SearchCondition, EmptyKeywordSearchHasNoTerm)
{
	EXPECT_EQ(Keywords(L""), L"");
}

/**
 * @file TransformsTests.cpp
 * @brief Unit tests for the special-paste transforms: DittoCore::CaseTransforms,
 *        TextTransforms, RtfTransforms, Typoglycemia and Slugifier.
 */
#include "CaseTransforms.h"
#include "RtfTransforms.h"
#include "Slugifier.h"
#include "TextTransforms.h"
#include "Typoglycemia.h"

#include <gtest/gtest.h>

#include <cwctype>
#include <stdexcept>
#include <string>
#include <vector>

using DittoCore::CaseTransforms;
using DittoCore::RtfTransforms;
using DittoCore::Slugifier;
using DittoCore::TextTransforms;
using DittoCore::Typoglycemia;

namespace
{
	// case mapping by the C runtime, character by character
	class CrtCaseMapper final : public DittoCore::ICaseMapper
	{
	public:
		bool IsUpper(wchar_t c) const override { return std::iswupper(c) != 0; }
		wchar_t ToUpper(wchar_t c) const override { return static_cast<wchar_t>(std::towupper(c)); }
		wchar_t ToLower(wchar_t c) const override { return static_cast<wchar_t>(std::towlower(c)); }
		std::wstring ToUpper(std::wstring_view text) const override { return Map(text, true); }
		std::wstring ToLower(std::wstring_view text) const override { return Map(text, false); }

	private:
		std::wstring Map(std::wstring_view text, bool upper) const
		{
			std::wstring mapped;
			for (const wchar_t c : text)
			{
				mapped += upper ? ToUpper(c) : ToLower(c);
			}
			return mapped;
		}
	};

	// returns the given values in turn
	class ScriptedRandom final : public DittoCore::IRandomRange
	{
	public:
		explicit ScriptedRandom(std::vector<int> values) : m_values(std::move(values)) {}
		int Next(int, int) override { return m_values.at(m_next++ % m_values.size()); }

	private:
		std::vector<int> m_values;
		std::size_t m_next{};
	};

	// always returns the high end of the range
	class HighRandom final : public DittoCore::IRandomRange
	{
	public:
		int Next(int, int high) override { return high; }
	};

	const CrtCaseMapper Crt{};
}

TEST(CaseTransforms, UpperAndLowerUseTheMapper)
{
	const CaseTransforms cases(Crt);

	EXPECT_EQ(cases.Upper(L"Abc d"), L"ABC D");
	EXPECT_EQ(cases.Lower(L"Abc D"), L"abc d");
}

TEST(CaseTransforms, InvertsCase)
{
	EXPECT_EQ(CaseTransforms(Crt).InvertCase(L"Hello World 1"), L"hELLO wORLD 1");
}

// Regression: upstream wrote the camel-case text with the length from before the spaces were
// removed, so the clip held stale characters after the terminator.
TEST(CaseTransforms, CamelCaseJoinsWords)
{
	EXPECT_EQ(CaseTransforms(Crt).CamelCase(L"hello big WORLD"), L"HelloBigWorld");
}

TEST(CaseTransforms, CapitalizesEveryWord)
{
	EXPECT_EQ(CaseTransforms(Crt).Capitalize(L"hello  big WORLD"), L"Hello  Big World");
}

// Regression: upstream raised the first character after '.', '!' or '?' unless it was a space,
// so after a period at the end of a line it raised the line break and missed the next word.
TEST(CaseTransforms, SentenceCaseSkipsWhiteSpaceAfterPeriod)
{
	EXPECT_EQ(CaseTransforms(Crt).SentenceCase(L"first ONE. second!   third?\r\nfourth"), L"First one. Second!   Third?\r\nFourth");
}

TEST(TextTransforms, KeepsAsciiOnly)
{
	EXPECT_EQ(TextTransforms::AsciiOnly(L"a\x00E9-b\x4E2D"), L"a-b");
}

TEST(TextTransforms, RemovesEveryKindOfLineBreak)
{
	EXPECT_EQ(TextTransforms::RemoveLineFeeds(L"a\r\nb\rc\nd"), L"a b c d");
}

TEST(TextTransforms, AddsLineFeeds)
{
	EXPECT_EQ(TextTransforms::AddLineFeeds(L"a", 2), L"a\r\n\r\n");
}

// Regression: upstream appended one CRLF to CF_UNICODETEXT but two to CF_TEXT.
TEST(TextTransforms, AddsDateTimeOnItsOwnLine)
{
	EXPECT_EQ(TextTransforms::AddDateTime(L"a", L"06.10.2026 20:00"), L"a\r\n06.10.2026 20:00");
}

TEST(TextTransforms, TrimsWhiteSpace)
{
	EXPECT_EQ(TextTransforms::Trim(L" \t\r\n a b \n"), L"a b");
	EXPECT_EQ(TextTransforms::Trim(L" \t "), L"");
}

TEST(TextTransforms, PosixifiesPaths)
{
	EXPECT_EQ(TextTransforms::PosixifyPaths(L" C:\\Users\\x d:\\y\\z.txt "), L"/c/Users/x /d/y/z.txt");
	EXPECT_EQ(TextTransforms::PosixifyPaths(L"C:"), L"C:");
}

TEST(RtfTransforms, RemovesParagraphAndLineBreaks)
{
	EXPECT_EQ(RtfTransforms::RemoveLineFeeds("{\\rtf1 a\\par\r\nb\\par c\\line d}"), "{\\rtf1 a b c d}");
}

TEST(RtfTransforms, AddsLineFeedsBeforeClosingBrace)
{
	EXPECT_EQ(RtfTransforms::AddLineFeeds("{\\rtf1 a}", 2), "{\\rtf1 a\\par\r\n\\par\r\n}");
}

// Regression: upstream also appended "\r\n\r\n" after the closing brace, and inserted the time
// in the ANSI code page without escaping.
TEST(RtfTransforms, AddsEscapedDateTimeBeforeClosingBrace)
{
	EXPECT_EQ(RtfTransforms::AddDateTime("{\\rtf1 a}", L"6.10.2026 \x00E9"), "{\\rtf1 a\\par\r\n\\par\r\n6.10.2026 \\u233?}");
}

TEST(RtfTransforms, LeavesDocumentWithoutClosingBraceAlone)
{
	EXPECT_EQ(RtfTransforms::AddLineFeeds("not rtf", 1), "not rtf");
}

TEST(Typoglycemia, ShufflesInnerLettersOnly)
{
	HighRandom random;

	EXPECT_EQ(Typoglycemia::Scramble(L"Hello world.", random), L"Hlelo wlord.");
}

// Regression: upstream appended a space after every word, so the text ended with a space.
TEST(Typoglycemia, KeepsShortWordsAndSpaces)
{
	HighRandom random;

	EXPECT_EQ(Typoglycemia::Scramble(L"a  an the end!!", random), L"a  an the end!!");
}

TEST(Typoglycemia, RejectsRandomValueOutsideTheWord)
{
	ScriptedRandom random({ 99 });

	EXPECT_THROW(Typoglycemia::Scramble(L"abcdef", random), std::out_of_range);
}

TEST(Slugifier, TransliteratesAndJoinsWords)
{
	EXPECT_EQ(Slugifier::Slugify(L"\x00DCn\x00EF" L"code Text!", L"-"), L"unicode-text");
	EXPECT_EQ(Slugifier::Slugify(L"Stra\x00DF" L"e", L"-"), L"strasse");
	EXPECT_EQ(Slugifier::Slugify(L"5\x20AC & 3\x00A3", L"-"), L"5euro-and-3pound");
}

// Regression: upstream called trim() and dropped its result, so leading and trailing spaces
// became separators at both ends.
TEST(Slugifier, HasNoSeparatorAtTheEnds)
{
	EXPECT_EQ(Slugifier::Slugify(L"  Hello   World  ", L"-"), L"hello-world");
}

TEST(Slugifier, UsesTheSeparator)
{
	EXPECT_EQ(Slugifier::Slugify(L"a b", L"_"), L"a_b");
	EXPECT_EQ(Slugifier::Slugify(L"a-b c", L"_"), L"a-b_c");
	EXPECT_EQ(Slugifier::Slugify(L"a - b", L"-"), L"a-b");
}

// Regression: upstream put the separator into a regular expression unescaped, so a separator
// such as "]" threw std::regex_error.
TEST(Slugifier, AcceptsAnySeparator)
{
	EXPECT_EQ(Slugifier::Slugify(L"a b", L"]"), L"a]b");
	EXPECT_EQ(Slugifier::Slugify(L"a b", L""), L"ab");
}

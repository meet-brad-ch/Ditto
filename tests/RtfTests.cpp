/**
 * @file RtfTests.cpp
 * @brief Unit tests for DittoCore::RtfJoin and DittoCore::RtfNormalizer.
 */
#include "ClipboardFormatError.h"
#include "RtfJoin.h"
#include "RtfNormalizer.h"

#include <gtest/gtest.h>

#include <string>

using DittoCore::ClipboardFormatError;
using DittoCore::RtfJoin;
using DittoCore::RtfNormalizer;

namespace
{
	int Depth(const std::string& rtf)
	{
		int depth{};
		for (std::size_t i = 0; i < rtf.size(); i++)
		{
			if (rtf[i] == '\\')
			{
				i++;   // an escaped brace is text
			}
			else if (rtf[i] == '{')
			{
				depth++;
			}
			else if (rtf[i] == '}')
			{
				depth--;
			}
		}
		return depth;
	}
}

TEST(RtfJoin, SingleDocumentIsKept)
{
	RtfJoin join(L"\r\n");
	join.Add("{\\rtf1\\ansi one}");

	EXPECT_EQ(join.Result(), "{\\rtf1\\ansi one}");
}

TEST(RtfJoin, JoinsTwoDocumentsWithSeparator)
{
	RtfJoin join(L"\r\n");
	join.Add("{\\rtf1\\ansi one}");
	join.Add("{\\rtf1\\ansi two}\r\n");

	EXPECT_EQ(join.Result(), "{\\rtf1\\ansi one\\par \\ansi two}");
}

// Regression: upstream removed "{\rtf1" from the last document only, so with three or more
// clips each middle document kept its opening brace without its closing one.
TEST(RtfJoin, ThreeDocumentsStayBraceBalanced)
{
	RtfJoin join(L"");
	join.Add("{\\rtf1 {\\b one}}");
	join.Add("{\\rtf1 {\\i two}}");
	join.Add("{\\rtf1 three}");

	const std::string result = join.Result();

	EXPECT_EQ(Depth(result), 0);
	EXPECT_EQ(result, "{\\rtf1 {\\b one} {\\i two} three}");
}

// Regression: upstream put the separator in unescaped, in the ANSI code page, and turned line
// breaks into "\par" with no delimiter, so "-\r\nx" became the control word "\parx".
TEST(RtfJoin, EscapesSeparator)
{
	EXPECT_EQ(RtfJoin::Escape(L"a{b}\\c\tz"), "a\\{b\\}\\\\c\\tab z");
	EXPECT_EQ(RtfJoin::Escape(L"\r\nx\ny\rz"), "\\par x\\par y\\par z");
	EXPECT_EQ(RtfJoin::Escape(L"\x00E9\x4E2D"), "\\u233?\\u20013?");
	EXPECT_EQ(RtfJoin::Escape(L"\xFFFD"), "\\u-3?");
}

TEST(RtfJoin, RejectsDocumentWithoutRtfStart)
{
	RtfJoin join(L"");

	EXPECT_THROW(join.Add("plain text}"), ClipboardFormatError);
}

TEST(RtfJoin, RejectsDocumentWithoutClosingBrace)
{
	RtfJoin join(L"");

	EXPECT_THROW(join.Add("{\\rtf1 open"), ClipboardFormatError);
}

TEST(RtfJoin, RejectsEmptyJoin)
{
	const RtfJoin join(L"");

	EXPECT_THROW(join.Result(), ClipboardFormatError);
}

TEST(RtfNormalizer, RemovesDatastoreGroup)
{
	EXPECT_EQ(RtfNormalizer::Normalize("{\\rtf1 a{\\*\\datastore 0102{\\x}}b}"), "{\\rtf1 ab}");
}

TEST(RtfNormalizer, KeepsUnclosedDatastoreGroup)
{
	EXPECT_EQ(RtfNormalizer::Normalize("{\\*\\datastore 01{"), "{\\*\\datastore 01{");
}

TEST(RtfNormalizer, RemovesRsidValues)
{
	EXPECT_EQ(RtfNormalizer::Normalize("\\rsid123 a\\insrsid4567 b\\rsid9 c"), " a b c");
}

TEST(RtfNormalizer, KeepsRsidWithoutNumber)
{
	EXPECT_EQ(RtfNormalizer::Normalize("\\rsidtbl a"), "\\rsidtbl a");
}

TEST(RtfNormalizer, KeepsEscapedBackslashBeforeRsid)
{
	EXPECT_EQ(RtfNormalizer::Normalize("\\\\rsid12 x"), "\\\\rsid12 x");
}

TEST(RtfNormalizer, StopsAtRsidDigitsRunningToTheEnd)
{
	// upstream's rule: the search ends there, and that occurrence stays
	EXPECT_EQ(RtfNormalizer::Normalize("\\rsid1 a\\rsid22"), " a\\rsid22");
}

TEST(RtfNormalizer, RemovesMdispDef)
{
	EXPECT_EQ(RtfNormalizer::Normalize("a\\mdispDef1 b\\mdispDef1"), "a b");
}

TEST(RtfNormalizer, LeavesOtherRtfAlone)
{
	const std::string rtf = "{\\rtf1\\ansi{\\fonttbl{\\f0 Arial;}}\\f0 text\\par}";

	EXPECT_EQ(RtfNormalizer::Normalize(rtf), rtf);
}

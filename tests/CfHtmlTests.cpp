/**
 * @file CfHtmlTests.cpp
 * @brief Unit tests for DittoCore::CfHtml, including the byte-offset regression.
 */
#include "CfHtml.h"
#include "ClipboardFormatError.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <span>
#include <string>

using DittoCore::CfHtml;
using DittoCore::CfHtmlFragment;
using DittoCore::ClipboardFormatError;

namespace
{
	std::string Utf8(const char8_t* text)
	{
		return std::string(reinterpret_cast<const char*>(text));
	}

	std::span<const std::byte> Bytes(const std::string& text)
	{
		return std::as_bytes(std::span(text));
	}

	// A CF_HTML block whose offsets are computed here, independently of CfHtml::Build.
	std::string Block(const std::string& before, const std::string& fragment, const std::string& after, const std::string& extraHeader = "")
	{
		const std::string headerTemplate = "Version:1.0\r\nStartHTML:0000000000\r\nEndHTML:0000000000\r\nStartFragment:0000000000\r\nEndFragment:0000000000\r\n" + extraHeader;
		const std::size_t start = headerTemplate.size() + before.size();
		const std::size_t end = start + fragment.size();
		char header[256]{};
		std::snprintf(header, sizeof(header), "Version:1.0\r\nStartHTML:%010zu\r\nEndHTML:%010zu\r\nStartFragment:%010zu\r\nEndFragment:%010zu\r\n",
			headerTemplate.size(), end + after.size(), start, end);
		return std::string(header) + extraHeader + before + fragment + after;
	}
}

// Regression: upstream decoded the UTF-8 block with the ANSI code page and applied the byte
// offsets as UTF-16 indexes, which cut the fragment in the wrong place around non-ASCII text.
TEST(CfHtml, ParsesFragmentAtByteOffsetsAroundMultiByteText)
{
	const std::string fragment = Utf8(u8"<b>Ünïcödé ✓ 😀</b>");
	const std::string block = Block(Utf8(u8"<html><body>éé😀<!--StartFragment-->"), fragment, "<!--EndFragment--></body></html>");

	EXPECT_EQ(CfHtml::Parse(Bytes(block)).fragment, fragment);
}

TEST(CfHtml, ReadsVersionAndSourceUrl)
{
	const std::string block = Block("<html><body><!--StartFragment-->", "<i>x</i>", "<!--EndFragment--></body></html>", "SourceURL:https://example.com/a?b=c\r\n");

	const CfHtmlFragment parsed = CfHtml::Parse(Bytes(block));

	EXPECT_EQ(parsed.version, "1.0");
	EXPECT_EQ(parsed.sourceUrl, "https://example.com/a?b=c");
}

TEST(CfHtml, TrimsWhitespaceAroundFragment)
{
	const std::string block = Block("<html><body>", " \r\n<p>x</p>\t ", "</body></html>");

	EXPECT_EQ(CfHtml::Parse(Bytes(block)).fragment, "<p>x</p>");
}

TEST(CfHtml, TextEndsAtTerminatingNull)
{
	std::string block = Block("<html><body>", "<p>x</p>", "</body></html>");
	block.push_back('\0');
	block += "garbage after the terminator";

	EXPECT_EQ(CfHtml::Parse(Bytes(block)).fragment, "<p>x</p>");
}

TEST(CfHtml, AcceptsLineFeedOnlyHeader)
{
	std::string block = Block("<html>", "<p>x</p>", "</html>");
	// same offsets, but \r\n -> \n would move them: rebuild with LF separators of equal length
	for (std::size_t pos = block.find("\r\n"); pos != std::string::npos && pos < block.find("<html>"); pos = block.find("\r\n", pos + 1))
	{
		block[pos] = ' ';
		block[pos + 1] = '\n';
	}

	EXPECT_EQ(CfHtml::Parse(Bytes(block)).fragment, "<p>x</p>");
}

TEST(CfHtml, RejectsMissingFragmentOffsets)
{
	const std::string block = "Version:1.0\r\nStartHTML:0000000040\r\n<html><body>x</body></html>";

	EXPECT_THROW(CfHtml::Parse(Bytes(block)), ClipboardFormatError);
}

TEST(CfHtml, RejectsFragmentPastEndOfText)
{
	std::string block = Block("<html>", "<p>x</p>", "</html>");
	block.resize(block.size() - 12);

	EXPECT_THROW(CfHtml::Parse(Bytes(block)), ClipboardFormatError);
}

TEST(CfHtml, RejectsReversedOffsets)
{
	const std::string block = "StartFragment:0000000050\r\nEndFragment:0000000040\r\n<html><body>xxxxxxxxxxxxxxxxxxxx</body></html>";

	EXPECT_THROW(CfHtml::Parse(Bytes(block)), ClipboardFormatError);
}

TEST(CfHtml, RejectsNonNumericOffset)
{
	const std::string block = "StartFragment:abc\r\nEndFragment:0000000040\r\n<html><body>xxxxxxxxxxxxxxxxxxxx</body></html>";

	EXPECT_THROW(CfHtml::Parse(Bytes(block)), ClipboardFormatError);
}

TEST(CfHtml, BuiltBlockParsesBackToItsParts)
{
	const std::string fragment = Utf8(u8"<b>Ünïcödé 😀</b>");

	const CfHtmlFragment parsed = CfHtml::Parse(Bytes(CfHtml::Build(fragment, "1.0", "https://example.com/")));

	EXPECT_EQ(parsed.fragment, fragment);
	EXPECT_EQ(parsed.version, "1.0");
	EXPECT_EQ(parsed.sourceUrl, "https://example.com/");
}

TEST(CfHtml, BuiltStartHtmlPointsAtHtmlElement)
{
	const std::string block = CfHtml::Build(Utf8(u8"<p>é</p>"), "1.0", "");
	const std::size_t value = block.find("StartHTML:") + 10;
	const std::size_t startHtml = std::stoul(block.substr(value, 10));

	EXPECT_EQ(block.substr(startHtml, 6), "<html>");
}

TEST(CfHtml, BuildDefaultsVersionAndOmitsEmptySourceUrl)
{
	const std::string block = CfHtml::Build("<p>x</p>", "", "");

	EXPECT_EQ(block.rfind("Version:0.9\r\n", 0), 0u);
	EXPECT_EQ(block.find("SourceURL"), std::string::npos);
}

TEST(CfHtml, HtmlFromTextEscapesMarkupAndBreaksLines)
{
	EXPECT_EQ(CfHtml::HtmlFromText(L"a<b>&\"\r\nc\nd"), "a&lt;b&gt;&amp;&quot;<br>c<br>d");
}

TEST(CfHtml, HtmlFromTextEncodesUtf8)
{
	EXPECT_EQ(CfHtml::HtmlFromText(L"é✓"), Utf8(u8"é✓"));
}

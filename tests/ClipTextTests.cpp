/**
 * @file ClipTextTests.cpp
 * @brief Unit tests for DittoCore::ClipText, including the empty-block regression.
 */
#include "ClipText.h"
#include "ClipboardFormatError.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using DittoCore::ClipboardFormatError;
using DittoCore::ClipText;

namespace
{
	std::vector<std::uint8_t> WideBytes(const std::wstring& text)
	{
		std::vector<std::uint8_t> bytes(text.size() * sizeof(wchar_t));
		std::memcpy(bytes.data(), text.data(), bytes.size());
		return bytes;
	}
}

TEST(ClipText, ReadsAnsiTextUpToTerminator)
{
	const std::string block("hello\0trailing", 14);

	EXPECT_EQ(ClipText::ReadAnsi(block.data(), block.size()), "hello");
}

TEST(ClipText, ReadsWideTextUpToTerminator)
{
	const std::vector<std::uint8_t> block = WideBytes(std::wstring(L"hello\0trailing", 14));

	EXPECT_EQ(ClipText::ReadWide(block.data(), block.size()), L"hello");
}

TEST(ClipText, ReadsEmptyTerminatedText)
{
	const char block[] = { '\0' };

	EXPECT_EQ(ClipText::ReadAnsi(block, sizeof(block)), "");
}

// Regression: the aggregators read text[size - 1], one element before the buffer when size is 0.
TEST(ClipText, RejectsEmptyAnsiBlock)
{
	const char block[] = { 'x' };

	EXPECT_THROW(ClipText::ReadAnsi(block, 0), ClipboardFormatError);
}

TEST(ClipText, RejectsWideBlockShorterThanOneCharacter)
{
	const std::uint8_t block[] = { 0 };

	EXPECT_THROW(ClipText::ReadWide(block, sizeof(block)), ClipboardFormatError);
}

TEST(ClipText, RejectsUnterminatedAnsiText)
{
	const std::string block = "no terminator";

	EXPECT_THROW(ClipText::ReadAnsi(block.data(), block.size()), ClipboardFormatError);
}

TEST(ClipText, RejectsUnterminatedWideText)
{
	const std::vector<std::uint8_t> block = WideBytes(L"no terminator");

	EXPECT_THROW(ClipText::ReadWide(block.data(), block.size()), ClipboardFormatError);
}

TEST(ClipText, RejectsNullData)
{
	EXPECT_THROW(ClipText::ReadAnsi(nullptr, 8), ClipboardFormatError);
	EXPECT_THROW(ClipText::ReadWide(nullptr, 8), ClipboardFormatError);
	EXPECT_THROW(ClipText::ReadAnsiBounded(nullptr, 8), ClipboardFormatError);
}

// CF_HTML and RTF are length-delimited: data sized exactly, without a null, is valid. Upstream
// overwrote its last byte with a null (and wrote before the buffer when it was empty).
TEST(ClipText, BoundedReadKeepsWholeBlockWithoutTerminator)
{
	const std::string block = "<html>full</html>";

	EXPECT_EQ(ClipText::ReadAnsiBounded(block.data(), block.size()), "<html>full</html>");
}

TEST(ClipText, BoundedReadStopsAtTerminator)
{
	const std::string block("<b>x</b>\0\0\0", 11);

	EXPECT_EQ(ClipText::ReadAnsiBounded(block.data(), block.size()), "<b>x</b>");
}

TEST(ClipText, BoundedReadOfEmptyBlockIsEmpty)
{
	const char block[] = { 'x' };

	EXPECT_EQ(ClipText::ReadAnsiBounded(block, 0), "");
}

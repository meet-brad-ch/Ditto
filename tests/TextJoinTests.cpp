/**
 * @file TextJoinTests.cpp
 * @brief Unit tests for DittoCore::TextJoin.
 */
#include "TextJoin.h"

#include <gtest/gtest.h>

#include <string>
#include <vector>

using DittoCore::TextJoin;

TEST(TextJoin, PutsSeparatorBetweenItemsOnly)
{
	TextJoin<char> join(" | ");
	join.Add("a");
	join.Add("b");
	join.Add("c");

	EXPECT_EQ(join.Result(), "a | b | c");
}

TEST(TextJoin, SingleItemHasNoSeparator)
{
	TextJoin<wchar_t> join(L"\r\n");
	join.Add(L"only");

	EXPECT_EQ(join.Result(), L"only");
}

TEST(TextJoin, NothingAddedIsEmpty)
{
	const TextJoin<char> join("-");

	EXPECT_TRUE(join.Result().empty());
}

TEST(TextJoin, AddsFileListAsLines)
{
	TextJoin<wchar_t> join(L"--");
	join.Add(L"text");
	const std::vector<std::wstring> paths{ L"C:\\a.txt", L"C:\\b.txt" };

	EXPECT_TRUE(join.AddLines(paths));
	EXPECT_EQ(join.Result(), L"text--C:\\a.txt\r\nC:\\b.txt\r\n");
}

// Regression: upstream added the separator after every clip except the last by position, so
// when the last clip was an empty file list (skipped), the text ended with a separator.
TEST(TextJoin, EmptyFileListAddsNoSeparator)
{
	TextJoin<char> join("--");
	join.Add("text");

	EXPECT_FALSE(join.AddLines(std::vector<std::string>{}));
	EXPECT_EQ(join.Result(), "text");
}

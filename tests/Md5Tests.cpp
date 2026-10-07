/**
 * @file Md5Tests.cpp
 * @brief Unit tests for DittoCore::Md5.
 */
#include "Md5.h"

#include <gtest/gtest.h>

#include <span>
#include <string>
#include <vector>

using DittoCore::Md5;

namespace
{
	std::string Hex(const std::string& text)
	{
		return Md5::Hex(std::as_bytes(std::span(text.data(), text.size())));
	}
}

// RFC 1321 test suite, in the upper-case hex Ditto stores
TEST(Md5, MatchesTheRfc1321Vectors)
{
	EXPECT_EQ(Hex(""), "D41D8CD98F00B204E9800998ECF8427E");
	EXPECT_EQ(Hex("a"), "0CC175B9C0F1B6A831C399E269772661");
	EXPECT_EQ(Hex("abc"), "900150983CD24FB0D6963F7D28E17F72");
	EXPECT_EQ(Hex("message digest"), "F96B697D7CB7938D525A2F31AAF161D0");
	EXPECT_EQ(Hex("12345678901234567890123456789012345678901234567890123456789012345678901234567890"), "57EDF4A22BE3C955AC49DA2E2107B67A");
}

TEST(Md5, HashesBinaryData)
{
	const std::vector<std::byte> bytes{ std::byte{ 0x00 }, std::byte{ 0xFF }, std::byte{ 0x80 } };

	EXPECT_EQ(Md5::Hex(bytes).size(), 32u);
	EXPECT_NE(Md5::Hex(bytes), Hex(""));
}

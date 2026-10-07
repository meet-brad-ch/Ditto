/**
 * @file Crc32Tests.cpp
 * @brief Unit tests for DittoCore::Crc32.
 */
#include "Crc32.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <span>
#include <string>
#include <vector>

using DittoCore::Crc32;

namespace
{
	std::span<const std::byte> Bytes(const std::string& text)
	{
		return std::as_bytes(std::span(text.data(), text.size()));
	}

	// The CRC code Ditto used before (CCrc32Dynamic): table over 0xEDB88320, start 0xFFFFFFFF,
	// inverted at the end. Kept here only to prove the stored CRCs do not change.
	std::uint32_t OldDittoCrc(const std::vector<std::string>& pieces)
	{
		std::uint32_t table[256]{};
		for (std::uint32_t i = 0; i < 256; i++)
		{
			std::uint32_t crc{ i };
			for (int j = 0; j < 8; j++)
			{
				crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320u : crc >> 1;
			}
			table[i] = crc;
		}
		std::uint32_t crc{ 0xFFFFFFFFu };
		for (const std::string& piece : pieces)
		{
			for (const char c : piece)
			{
				crc = (crc >> 8) ^ table[static_cast<unsigned char>(c) ^ (crc & 0xFFu)];
			}
		}
		return ~crc;
	}
}

// The standard CRC-32 check value
TEST(Crc32, MatchesTheStandardCheckValue)
{
	Crc32 crc;
	crc.Add(Bytes("123456789"));

	EXPECT_EQ(crc.Value(), 0xCBF43926u);
}

TEST(Crc32, NothingAddedIsZero)
{
	Crc32 crc;
	crc.Add({});

	EXPECT_EQ(crc.Value(), 0u);
}

TEST(Crc32, PiecesGiveTheCrcOfTheirConcatenation)
{
	Crc32 pieces;
	pieces.Add(Bytes("Ditto "));
	pieces.Add(Bytes("clip"));
	Crc32 whole;
	whole.Add(Bytes("Ditto clip"));

	EXPECT_EQ(pieces.Value(), whole.Value());
}

// Regression: replacing CCrc32Dynamic must not change the CRCs stored in existing databases
TEST(Crc32, EqualsTheCrcDittoStoredBefore)
{
	const std::vector<std::string> pieces{ "first format", std::string("\0binary\xFF\x80", 9), "" };
	Crc32 crc;
	for (const std::string& piece : pieces)
	{
		crc.Add(Bytes(piece));
	}

	EXPECT_EQ(crc.Value(), OldDittoCrc(pieces));
}

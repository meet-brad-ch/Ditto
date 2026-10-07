/**
 * @file DtoCodecTests.cpp
 * @brief Unit tests for DittoCore::DtoCodec.
 */
#include "ClipboardFormatError.h"
#include "DtoCodec.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using DittoCore::ClipboardFormatError;
using DittoCore::DtoCodec;

namespace
{
	std::vector<std::byte> Bytes(const std::string& text)
	{
		std::vector<std::byte> bytes;
		for (const char c : text)
		{
			bytes.push_back(static_cast<std::byte>(c));
		}
		return bytes;
	}

	std::int64_t Size(const std::vector<std::byte>& bytes)
	{
		return static_cast<std::int64_t>(bytes.size());
	}
}

TEST(DtoCodec, RoundTripsText)
{
	const std::vector<std::byte> data = Bytes("Ditto clip text, Ditto clip text, Ditto clip text");

	const std::vector<std::byte> compressed = DtoCodec::Compress(data);

	EXPECT_EQ(DtoCodec::Uncompress(compressed, Size(data)), data);
}

TEST(DtoCodec, RoundTripsLargeBinaryData)
{
	std::vector<std::byte> data(200000);
	for (std::size_t i = 0; i < data.size(); i++)
	{
		data[i] = static_cast<std::byte>((i * 7919) % 251);
	}

	EXPECT_EQ(DtoCodec::Uncompress(DtoCodec::Compress(data), Size(data)), data);
}

TEST(DtoCodec, RoundTripsEmptyFormat)
{
	const std::vector<std::byte> compressed = DtoCodec::Compress({});

	EXPECT_TRUE(DtoCodec::Uncompress(compressed, 0).empty());
}

TEST(DtoCodec, RejectsNegativeSize)
{
	EXPECT_THROW(DtoCodec::Uncompress(DtoCodec::Compress(Bytes("x")), -1), ClipboardFormatError);
}

TEST(DtoCodec, RejectsSizeAboveMaximum)
{
	EXPECT_THROW(DtoCodec::Uncompress(DtoCodec::Compress(Bytes("x")), DtoCodec::MaxOriginalSize + 1), ClipboardFormatError);
}

// Regression: upstream allocated the size the file declared (new Bytef[lOriginalSize]) before
// looking at the data, so a small file could make Ditto allocate gigabytes.
TEST(DtoCodec, RejectsSizeDeflateCannotReach)
{
	const std::vector<std::byte> compressed = DtoCodec::Compress(Bytes("tiny"));

	EXPECT_THROW(DtoCodec::Uncompress(compressed, 500 * 1024 * 1024), ClipboardFormatError);
}

TEST(DtoCodec, RejectsDataThatIsNotZlib)
{
	EXPECT_THROW(DtoCodec::Uncompress(Bytes("not a zlib stream"), 10), ClipboardFormatError);
}

TEST(DtoCodec, RejectsDeclaredSizeSmallerThanData)
{
	const std::vector<std::byte> data = Bytes("twelve bytes");

	EXPECT_THROW(DtoCodec::Uncompress(DtoCodec::Compress(data), Size(data) - 1), ClipboardFormatError);
}

TEST(DtoCodec, RejectsDeclaredSizeLargerThanData)
{
	const std::vector<std::byte> data = Bytes("twelve bytes");

	EXPECT_THROW(DtoCodec::Uncompress(DtoCodec::Compress(data), Size(data) + 1), ClipboardFormatError);
}

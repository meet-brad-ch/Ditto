/**
 * @file DibHeaderTests.cpp
 * @brief Unit tests for DittoCore::DibHeader, including the pixel-offset regression.
 */
#include "DibHeader.h"
#include "ClipboardFormatError.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <vector>

using DittoCore::ClipboardFormatError;
using DittoCore::DibHeader;
using DittoCore::DibLayout;

namespace
{
	constexpr std::uint32_t BI_RGB_{ 0 };
	constexpr std::uint32_t BI_RLE8_{ 1 };
	constexpr std::uint32_t BI_BITFIELDS_{ 3 };
	constexpr std::uint32_t BI_PNG_{ 5 };
	constexpr std::uint32_t BI_ALPHABITFIELDS_{ 6 };

	struct DibSpec
	{
		std::uint32_t headerSize{ 40 };
		std::int32_t width{ 2 };
		std::int32_t height{ 2 };
		std::uint16_t planes{ 1 };
		std::uint16_t bitCount{ 24 };
		std::uint32_t compression{ BI_RGB_ };
		std::uint32_t sizeImage{};
		std::uint32_t clrUsed{};
		std::size_t tableBytes{};
		std::size_t pixelBytes{ 16 };
	};

	template <typename T>
	void Put(std::vector<std::byte>& block, T value)
	{
		std::byte bytes[sizeof(T)]{};
		std::memcpy(bytes, &value, sizeof(T));
		block.insert(block.end(), bytes, bytes + sizeof(T));
	}

	std::vector<std::byte> Dib(const DibSpec& spec)
	{
		std::vector<std::byte> block;
		Put(block, spec.headerSize);
		Put(block, spec.width);
		Put(block, spec.height);
		Put(block, spec.planes);
		Put(block, spec.bitCount);
		Put(block, spec.compression);
		Put(block, spec.sizeImage);
		Put(block, std::int32_t{ 2835 });
		Put(block, std::int32_t{ 2835 });
		Put(block, spec.clrUsed);
		Put(block, std::uint32_t{ 0 });
		block.resize(spec.headerSize + spec.tableBytes + spec.pixelBytes);
		return block;
	}

	std::uint32_t U32(const std::array<std::byte, DibHeader::FileHeaderSize>& header, std::size_t at)
	{
		std::uint32_t value{};
		std::memcpy(&value, header.data() + at, sizeof(value));
		return value;
	}
}

TEST(DibHeader, Reads24BitImage)
{
	const DibLayout layout = DibHeader::Read(Dib(DibSpec{}));

	EXPECT_EQ(layout.bitsOffset, 40u);
	EXPECT_EQ(layout.imageSize, 16u);   // 2 rows of 6 bytes, padded to 8
	EXPECT_EQ(layout.width, 2);
	EXPECT_EQ(layout.height, 2);
}

TEST(DibHeader, CountsFullPaletteOf8BitImage)
{
	DibSpec spec{};
	spec.bitCount = 8;
	spec.tableBytes = 256 * 4;
	spec.pixelBytes = 8;

	EXPECT_EQ(DibHeader::Read(Dib(spec)).bitsOffset, 40u + 1024u);
}

TEST(DibHeader, CountsUsedColorsOnly)
{
	DibSpec spec{};
	spec.bitCount = 8;
	spec.clrUsed = 16;
	spec.tableBytes = 16 * 4;
	spec.pixelBytes = 8;

	EXPECT_EQ(DibHeader::Read(Dib(spec)).bitsOffset, 40u + 64u);
}

// Regression: upstream put the pixels right after a 40-byte header, ignoring the three bit
// masks a BI_BITFIELDS image stores there, so such images were drawn and saved shifted.
TEST(DibHeader, CountsBitMasksAfterInfoHeader)
{
	DibSpec spec{};
	spec.bitCount = 32;
	spec.compression = BI_BITFIELDS_;
	spec.tableBytes = 12;
	spec.pixelBytes = 16;

	EXPECT_EQ(DibHeader::Read(Dib(spec)).bitsOffset, 52u);
}

// Regression: upstream assumed a 40-byte header; a V5 header is 124 bytes with the masks inside.
TEST(DibHeader, UsesSizeOfV5Header)
{
	DibSpec spec{};
	spec.headerSize = 124;
	spec.bitCount = 32;
	spec.compression = BI_BITFIELDS_;
	spec.pixelBytes = 16;

	EXPECT_EQ(DibHeader::Read(Dib(spec)).bitsOffset, 124u);
}

TEST(DibHeader, AcceptsTopDownImage)
{
	DibSpec spec{};
	spec.height = -2;

	EXPECT_EQ(DibHeader::Read(Dib(spec)).height, -2);
}

TEST(DibHeader, UsesSizeImageForRunLengthImages)
{
	DibSpec spec{};
	spec.bitCount = 8;
	spec.compression = BI_RLE8_;
	spec.tableBytes = 256 * 4;
	spec.sizeImage = 10;
	spec.pixelBytes = 10;

	EXPECT_EQ(DibHeader::Read(Dib(spec)).imageSize, 10u);
}

TEST(DibHeader, FileHeaderPointsAtPixels)
{
	DibSpec spec{};
	spec.bitCount = 32;
	spec.compression = BI_BITFIELDS_;
	spec.tableBytes = 12;
	const std::vector<std::byte> dib = Dib(spec);

	const auto header = DibHeader::FileHeader(DibHeader::Read(dib), dib.size());

	EXPECT_EQ(header[0], std::byte{ 'B' });
	EXPECT_EQ(header[1], std::byte{ 'M' });
	EXPECT_EQ(U32(header, 2), 14u + dib.size());
	EXPECT_EQ(U32(header, 10), 14u + 52u);
}

TEST(DibHeader, RejectsBlockShorterThanHeader)
{
	std::vector<std::byte> dib = Dib(DibSpec{});
	dib.resize(30);

	EXPECT_THROW(DibHeader::Read(dib), ClipboardFormatError);
}

TEST(DibHeader, RejectsUnknownHeaderSize)
{
	DibSpec spec{};
	spec.headerSize = 64;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsWrongPlaneCount)
{
	DibSpec spec{};
	spec.planes = 2;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsUnknownBitCount)
{
	DibSpec spec{};
	spec.bitCount = 7;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsMoreColorsThanBitsAllow)
{
	DibSpec spec{};
	spec.bitCount = 4;
	spec.clrUsed = 17;
	spec.tableBytes = 17 * 4;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsTruncatedPixels)
{
	DibSpec spec{};
	spec.pixelBytes = 15;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsHugeDimensionsWithoutOverflow)
{
	DibSpec spec{};
	spec.width = 0x7FFFFFFF;
	spec.height = 0x7FFFFFFF;
	spec.bitCount = 32;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsZeroWidth)
{
	DibSpec spec{};
	spec.width = 0;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, CountsFourMasksOfAlphaBitFields)
{
	DibSpec spec{};
	spec.bitCount = 32;
	spec.compression = BI_ALPHABITFIELDS_;
	spec.tableBytes = 16;
	spec.pixelBytes = 16;

	EXPECT_EQ(DibHeader::Read(Dib(spec)).bitsOffset, 56u);
}

TEST(DibHeader, RejectsColorTablePastBlock)
{
	DibSpec spec{};
	spec.bitCount = 8;           // 256 colors = 1024 table bytes
	spec.tableBytes = 0;
	spec.pixelBytes = 100;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsUnknownCompression)
{
	DibSpec spec{};
	spec.compression = 4;        // BI_JPEG

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsRunLengthImageLargerThanBlock)
{
	DibSpec spec{};
	spec.bitCount = 8;
	spec.compression = BI_RLE8_;
	spec.tableBytes = 256 * 4;
	spec.sizeImage = 11;
	spec.pixelBytes = 10;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

TEST(DibHeader, RejectsFileLargerThanBmpAllows)
{
	const DibLayout layout = DibHeader::Read(Dib(DibSpec{}));

	EXPECT_THROW(DibHeader::FileHeader(layout, 0xFFFFFFFFu), ClipboardFormatError);
}

TEST(DibHeader, RejectsEmbeddedPng)
{
	DibSpec spec{};
	spec.bitCount = 0;
	spec.compression = BI_PNG_;

	EXPECT_THROW(DibHeader::Read(Dib(spec)), ClipboardFormatError);
}

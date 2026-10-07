/**
 * @file QrBitmapTests.cpp
 * @brief Unit tests for DittoCore::QrBitmap.
 */
#include "QrBitmap.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using DittoCore::QrBitmap;

namespace
{
	std::uint32_t Read32(const std::vector<std::byte>& bytes, std::size_t offset)
	{
		std::uint32_t value{};
		for (int i = 3; i >= 0; i--)
		{
			value = (value << 8) | std::to_integer<std::uint32_t>(bytes[offset + static_cast<std::size_t>(i)]);
		}
		return value;
	}

	// The blue byte of a pixel: 0 for a dark module, 0xFF for a light one
	int Pixel(const std::vector<std::byte>& bitmap, std::uint32_t x, std::uint32_t y)
	{
		const std::uint32_t width = Read32(bitmap, 18);
		const std::uint32_t rowBytes = (width * 3 + 3) / 4 * 4;
		return std::to_integer<int>(bitmap[Read32(bitmap, 10) + static_cast<std::size_t>(rowBytes) * y + static_cast<std::size_t>(x) * 3]);
	}
}

TEST(QrBitmap, WritesA24BitTopDownBmp)
{
	const std::vector<std::byte> bitmap = QrBitmap::Render("Ditto");

	ASSERT_GE(bitmap.size(), 54u);
	EXPECT_EQ(std::to_integer<char>(bitmap[0]), 'B');
	EXPECT_EQ(std::to_integer<char>(bitmap[1]), 'M');
	EXPECT_EQ(Read32(bitmap, 2), bitmap.size());
	EXPECT_EQ(Read32(bitmap, 10), 54u);
	EXPECT_EQ(Read32(bitmap, 14), 40u);
	EXPECT_EQ(static_cast<std::int32_t>(Read32(bitmap, 22)), -static_cast<std::int32_t>(Read32(bitmap, 18)));
	EXPECT_EQ(std::to_integer<int>(bitmap[28]), 24);
}

// "Ditto" fits version 1 (21 modules) at the highest error correction
TEST(QrBitmap, ScalesEachModuleToEightPixels)
{
	const std::vector<std::byte> bitmap = QrBitmap::Render("Ditto");

	EXPECT_EQ(Read32(bitmap, 18), 21u * QrBitmap::PixelsPerModule);
}

TEST(QrBitmap, DrawsTheFinderPatternDarkAndItsSeparatorLight)
{
	const std::vector<std::byte> bitmap = QrBitmap::Render("Ditto");
	const std::uint32_t module = QrBitmap::PixelsPerModule;

	EXPECT_EQ(Pixel(bitmap, 0, 0), 0);
	EXPECT_EQ(Pixel(bitmap, module - 1, module - 1), 0);
	// the finder pattern is 7 modules wide; module 7 is the light separator
	EXPECT_EQ(Pixel(bitmap, 7 * module, 0), 0xFF);
}

TEST(QrBitmap, FileSizeIsHeadersPlusRows)
{
	// a larger version than "Ditto"'s; 8 pixels of 3 bytes per module keep rows 4-byte aligned
	const std::vector<std::byte> bitmap = QrBitmap::Render(std::string(20, 'x'));
	const std::uint32_t width = Read32(bitmap, 18);

	EXPECT_EQ(bitmap.size(), 54u + static_cast<std::size_t>((width * 3 + 3) / 4 * 4) * width);
}

TEST(QrBitmap, RejectsTextTooLongForAnyVersion)
{
	EXPECT_THROW(QrBitmap::Render(std::string(4000, 'x')), std::length_error);
}

/**
 * @file DibFuzzTarget.cpp
 * @brief Implements DibFuzzTarget.
 */
#include "DibFuzzTarget.h"

#include "ClipboardFormatError.h"
#include "DibHeader.h"

#include <cstdlib>
#include <cstring>

void DibFuzzTarget::Run(std::span<const std::byte> input) const
{
	DittoCore::DibLayout layout{};
	try
	{
		layout = DittoCore::DibHeader::Read(input);
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
		return;   // malformed header or sizes: rejected, as it must be
	}

	if (layout.imageSize == 0 || layout.bitsOffset + layout.imageSize > input.size())
	{
		std::abort();   // a finding: a layout the drawing code cannot use
	}
	// the drawing code reads imageSize bytes at bitsOffset; touch each one (volatile keeps the reads)
	volatile std::byte sink{};
	for (const std::byte value : input.subspan(layout.bitsOffset, layout.imageSize))
	{
		sink = value;
	}

	const auto header = DittoCore::DibHeader::FileHeader(layout, input.size());
	std::uint32_t bitsOffset{};
	std::memcpy(&bitsOffset, header.data() + 10, sizeof(bitsOffset));
	if (bitsOffset != DittoCore::DibHeader::FileHeaderSize + layout.bitsOffset)
	{
		std::abort();   // a finding: the .bmp header points somewhere else than the pixels
	}
}

std::vector<std::vector<std::byte>> DibFuzzTarget::Seeds() const
{
	return {
		Dib(24, 0, 0, 16),           // 2 x 2, BI_RGB
		Dib(8, 0, 256 * 4, 8),       // full palette
		Dib(32, 3, 12, 16),          // BI_BITFIELDS with its three masks
	};
}

std::vector<std::byte> DibFuzzTarget::Dib(std::uint16_t bitCount, std::uint32_t compression, std::size_t tableBytes, std::size_t pixelBytes)
{
	const std::uint32_t headerSize{ 40 };
	const std::int32_t width{ 2 };
	const std::int32_t height{ 2 };
	const std::uint16_t planes{ 1 };
	std::vector<std::byte> block(headerSize + tableBytes + pixelBytes);
	std::memcpy(block.data(), &headerSize, 4);
	std::memcpy(block.data() + 4, &width, 4);
	std::memcpy(block.data() + 8, &height, 4);
	std::memcpy(block.data() + 12, &planes, 2);
	std::memcpy(block.data() + 14, &bitCount, 2);
	std::memcpy(block.data() + 16, &compression, 4);
	return block;
}

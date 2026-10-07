/**
 * @file DibHeader.cpp
 * @brief Implements DittoCore::DibHeader.
 */
#include "DibHeader.h"
#include "ClipboardFormatError.h"

#include <algorithm>
#include <climits>
#include <cstring>
#include <limits>
#include <string>

namespace DittoCore
{
	DibLayout DibHeader::Read(std::span<const std::byte> dib)
	{
		const Fields fields = ReadFields(dib);
		ValidateFormat(fields);
		ValidateDimensions(fields);

		const std::uint64_t colorTableSize = ColorTableSize(fields);
		const std::uint64_t bitsOffset = fields.headerSize + colorTableSize;
		if (bitsOffset > dib.size())
		{
			throw ClipboardFormatError("DIB color table ends at byte " + std::to_string(bitsOffset) + ", past the " + std::to_string(dib.size()) + "-byte block");
		}

		DibLayout layout{};
		layout.headerSize = fields.headerSize;
		layout.colorTableSize = static_cast<std::size_t>(colorTableSize);
		layout.bitsOffset = static_cast<std::size_t>(bitsOffset);
		layout.imageSize = ImageSize(fields, dib.size() - layout.bitsOffset);
		layout.width = fields.width;
		layout.height = fields.height;
		layout.bitCount = fields.bitCount;
		layout.compression = fields.compression;
		return layout;
	}

	std::array<std::byte, DibHeader::FileHeaderSize> DibHeader::FileHeader(const DibLayout& layout, std::size_t dibSize)
	{
		if (dibSize > std::numeric_limits<std::uint32_t>::max() - FileHeaderSize)
		{
			throw ClipboardFormatError("DIB of " + std::to_string(dibSize) + " bytes is too large for a .bmp file");
		}
		const std::uint16_t type{ 0x4D42 };   // "BM"
		const std::uint32_t fileSize{ static_cast<std::uint32_t>(FileHeaderSize + dibSize) };
		const std::uint32_t reserved{};
		const std::uint32_t bitsOffset{ static_cast<std::uint32_t>(FileHeaderSize + layout.bitsOffset) };

		std::array<std::byte, FileHeaderSize> header{};
		std::memcpy(header.data(), &type, sizeof(type));
		std::memcpy(header.data() + 2, &fileSize, sizeof(fileSize));
		std::memcpy(header.data() + 6, &reserved, sizeof(reserved));
		std::memcpy(header.data() + 10, &bitsOffset, sizeof(bitsOffset));
		return header;
	}

	DibHeader::Fields DibHeader::ReadFields(std::span<const std::byte> dib)
	{
		if (dib.size() < InfoHeaderSize)
		{
			throw ClipboardFormatError("DIB of " + std::to_string(dib.size()) + " bytes is shorter than a BITMAPINFOHEADER");
		}
		Fields fields{};
		std::memcpy(&fields.headerSize, dib.data(), 4);
		std::memcpy(&fields.width, dib.data() + 4, 4);
		std::memcpy(&fields.height, dib.data() + 8, 4);
		std::memcpy(&fields.planes, dib.data() + 12, 2);
		std::memcpy(&fields.bitCount, dib.data() + 14, 2);
		std::memcpy(&fields.compression, dib.data() + 16, 4);
		std::memcpy(&fields.sizeImage, dib.data() + 20, 4);
		std::memcpy(&fields.clrUsed, dib.data() + 32, 4);

		const std::array<std::uint32_t, 5> knownSizes{ 40, 52, 56, 108, 124 };
		if (std::find(knownSizes.begin(), knownSizes.end(), fields.headerSize) == knownSizes.end() || fields.headerSize > dib.size())
		{
			throw ClipboardFormatError("DIB header size " + std::to_string(fields.headerSize) + " is unknown or past the block");
		}
		return fields;
	}

	void DibHeader::ValidateFormat(const Fields& fields)
	{
		const std::array<std::uint16_t, 6> bitCounts{ 1, 4, 8, 16, 24, 32 };
		if (fields.planes != 1 || std::find(bitCounts.begin(), bitCounts.end(), fields.bitCount) == bitCounts.end())
		{
			throw ClipboardFormatError("DIB has " + std::to_string(fields.planes) + " planes of " + std::to_string(fields.bitCount) + " bits");
		}
		const bool rle = fields.compression == RunLength8 || fields.compression == RunLength4;
		const bool known = rle || fields.compression == Rgb || fields.compression == BitFields || fields.compression == AlphaBitFields;
		if (!known)
		{
			throw ClipboardFormatError("DIB compression " + std::to_string(fields.compression) + " is not supported");
		}
	}

	void DibHeader::ValidateDimensions(const Fields& fields)
	{
		if (fields.width <= 0 || fields.height == 0 || fields.height == INT_MIN)
		{
			throw ClipboardFormatError("DIB is " + std::to_string(fields.width) + " x " + std::to_string(fields.height) + " pixels");
		}
	}

	std::uint64_t DibHeader::ColorTableSize(const Fields& fields)
	{
		std::uint64_t masks{};
		if (fields.headerSize == InfoHeaderSize && fields.compression == BitFields)
		{
			masks = 3 * 4;
		}
		else if (fields.headerSize == InfoHeaderSize && fields.compression == AlphaBitFields)
		{
			masks = 4 * 4;
		}
		const std::uint64_t maxColors = fields.bitCount <= 8 ? (std::uint64_t{ 1 } << fields.bitCount) : 0;
		if (fields.bitCount <= 8 && fields.clrUsed > maxColors)
		{
			throw ClipboardFormatError("DIB of " + std::to_string(fields.bitCount) + " bits claims " + std::to_string(fields.clrUsed) + " colors");
		}
		const std::uint64_t colors = fields.clrUsed != 0 ? fields.clrUsed : maxColors;
		return masks + colors * 4;
	}

	std::size_t DibHeader::ImageSize(const Fields& fields, std::size_t available)
	{
		if (fields.compression == RunLength8 || fields.compression == RunLength4)
		{
			if (fields.sizeImage == 0 || fields.sizeImage > available)
			{
				throw ClipboardFormatError("compressed DIB of " + std::to_string(fields.sizeImage) + " bytes does not fit the " + std::to_string(available) + " bytes after its header");
			}
			return fields.sizeImage;
		}
		const std::uint64_t stride = (static_cast<std::uint64_t>(fields.width) * fields.bitCount + 31) / 32 * 4;
		const std::uint64_t rows = fields.height < 0 ? -static_cast<std::int64_t>(fields.height) : fields.height;
		if (rows > available / stride)
		{
			throw ClipboardFormatError("DIB pixels (" + std::to_string(rows) + " rows of " + std::to_string(stride) + " bytes) exceed the " + std::to_string(available) + " bytes after its header");
		}
		return static_cast<std::size_t>(stride * rows);
	}
}

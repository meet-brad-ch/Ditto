/**
 * @file DibLayout.h
 * @brief Declares DittoCore::DibLayout.
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace DittoCore
{
	/**
	 * @brief Where the parts of a validated CF_DIB block are, all offsets from its start.
	 */
	struct DibLayout
	{
		/// The header size (biSize): 40, 52, 56, 108 or 124.
		std::size_t headerSize{};
		/// Bytes between the header and the pixels: color table plus, for a 40-byte header, bit masks.
		std::size_t colorTableSize{};
		/// Where the pixels start: headerSize + colorTableSize.
		std::size_t bitsOffset{};
		/// The number of pixel bytes the header describes.
		std::size_t imageSize{};
		/// The width in pixels.
		std::int32_t width{};
		/// The height in pixels; negative for a top-down image.
		std::int32_t height{};
		/// Bits per pixel.
		std::uint16_t bitCount{};
		/// The compression (BI_RGB, BI_RLE8, BI_RLE4, BI_BITFIELDS or BI_ALPHABITFIELDS).
		std::uint32_t compression{};
	};
}

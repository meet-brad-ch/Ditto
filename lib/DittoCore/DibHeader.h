/**
 * @file DibHeader.h
 * @brief Declares DittoCore::DibHeader.
 */
#pragma once

#include "DibLayout.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace DittoCore
{
	/**
	 * @brief Validates CF_DIB blocks (a BITMAPINFO header, a color table and the pixels).
	 *
	 * Every size in the header is checked against the block, so code that draws or saves the
	 * image can use the offsets without reading past it.
	 */
	class DibHeader
	{
	public:
		/// Size of a BITMAPFILEHEADER, which a .bmp file or stream puts before the DIB.
		static constexpr std::size_t FileHeaderSize{ 14 };

		/**
		 * @brief Validates a DIB and locates its parts.
		 * @param dib The CF_DIB block.
		 * @return The layout.
		 * @throws ClipboardFormatError When the header, color table or pixels do not fit the
		 *         block, or the header describes an unsupported or impossible image.
		 */
		static DibLayout Read(std::span<const std::byte> dib);

		/**
		 * @brief The BITMAPFILEHEADER that turns the DIB into a .bmp file.
		 * @param layout The DIB's layout from Read.
		 * @param dibSize The size of the whole DIB block.
		 * @return The 14 header bytes ("BM", file size, pixel offset).
		 */
		static std::array<std::byte, FileHeaderSize> FileHeader(const DibLayout& layout, std::size_t dibSize);

	private:
		/// Size of BITMAPINFOHEADER, the smallest header Ditto accepts.
		static constexpr std::uint32_t InfoHeaderSize{ 40 };
		/// BI_RGB: uncompressed.
		static constexpr std::uint32_t Rgb{ 0 };
		/// BI_RLE8: run-length encoded, 8 bits per pixel.
		static constexpr std::uint32_t RunLength8{ 1 };
		/// BI_RLE4: run-length encoded, 4 bits per pixel.
		static constexpr std::uint32_t RunLength4{ 2 };
		/// BI_BITFIELDS: uncompressed with red, green and blue masks.
		static constexpr std::uint32_t BitFields{ 3 };
		/// BI_ALPHABITFIELDS: uncompressed with red, green, blue and alpha masks.
		static constexpr std::uint32_t AlphaBitFields{ 6 };

		/// The BITMAPINFOHEADER fields the layout depends on.
		struct Fields
		{
			/// biSize.
			std::uint32_t headerSize{};
			/// biWidth.
			std::int32_t width{};
			/// biHeight.
			std::int32_t height{};
			/// biPlanes.
			std::uint16_t planes{};
			/// biBitCount.
			std::uint16_t bitCount{};
			/// biCompression.
			std::uint32_t compression{};
			/// biSizeImage.
			std::uint32_t sizeImage{};
			/// biClrUsed.
			std::uint32_t clrUsed{};
		};

		/**
		 * @brief Reads the header fields.
		 * @param dib The CF_DIB block.
		 * @return The fields.
		 * @throws ClipboardFormatError When the block is shorter than its header or the header size is unknown.
		 */
		static Fields ReadFields(std::span<const std::byte> dib);

		/**
		 * @brief Checks planes, bit count and compression.
		 * @param fields The header fields.
		 * @throws ClipboardFormatError When one of them is not a supported value.
		 */
		static void ValidateFormat(const Fields& fields);

		/**
		 * @brief Checks that the image has a positive width and a non-zero height.
		 * @param fields The header fields.
		 * @throws ClipboardFormatError When it does not.
		 */
		static void ValidateDimensions(const Fields& fields);

		/**
		 * @brief The bytes between the header and the pixels: bit masks (40-byte header only) and colors.
		 * @param fields The header fields.
		 * @return The size, in 64 bits so that no claimed size can overflow.
		 * @throws ClipboardFormatError When more colors are claimed than the bit count allows.
		 */
		static std::uint64_t ColorTableSize(const Fields& fields);

		/**
		 * @brief The pixel bytes the header describes.
		 * @param fields The header fields.
		 * @param available The bytes after the color table.
		 * @return The size.
		 * @throws ClipboardFormatError When the pixels do not fit in @p available.
		 */
		static std::size_t ImageSize(const Fields& fields, std::size_t available);
	};
}

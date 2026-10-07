/**
 * @file QrBitmap.h
 * @brief Declares DittoCore::QrBitmap.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DittoCore
{
	/**
	 * @brief Renders text as a QR code in a BMP file image, the image the QR code viewer shows.
	 *
	 * The text is encoded as bytes (byte mode) with the highest error correction, in the smallest
	 * QR version that holds it. The bitmap is 24-bit, top-down, black modules on white, with no
	 * quiet zone (the viewer draws its own border).
	 */
	class QrBitmap
	{
	public:
		/// Pixels per QR module, on each axis.
		static constexpr int PixelsPerModule{ 8 };

		/**
		 * @brief Renders the text as a QR code bitmap.
		 * @param utf8Text The text, as UTF-8 bytes.
		 * @return A complete BMP file: file header, info header, pixel rows.
		 * @throws std::length_error When the text is too long for any QR version.
		 */
		static std::vector<std::byte> Render(const std::string& utf8Text);

	private:
		/// The size of the BMP file header (BITMAPFILEHEADER).
		static constexpr std::uint32_t FileHeaderSize{ 14 };
		/// The size of the BMP info header (BITMAPINFOHEADER).
		static constexpr std::uint32_t InfoHeaderSize{ 40 };

		/**
		 * @brief Appends an unsigned value in little-endian byte order.
		 * @param out The buffer.
		 * @param value The value.
		 * @param byteCount How many bytes to write (2 or 4).
		 */
		static void AppendLittleEndian(std::vector<std::byte>& out, std::uint32_t value, int byteCount);

		/**
		 * @brief Appends the BMP file and info headers of a 24-bit top-down image.
		 * @param out The buffer.
		 * @param width The width in pixels.
		 * @param pixelBytes The size of the pixel rows in bytes.
		 */
		static void AppendHeaders(std::vector<std::byte>& out, std::uint32_t width, std::uint32_t pixelBytes);
	};
}

/**
 * @file QrBitmap.cpp
 * @brief Implements DittoCore::QrBitmap.
 */
#include "QrBitmap.h"

#include <qrcodegen.hpp>

#include <cstdint>
#include <vector>

namespace DittoCore
{
	std::vector<std::byte> QrBitmap::Render(const std::string& utf8Text)
	{
		const std::vector<std::uint8_t> bytes(utf8Text.begin(), utf8Text.end());
		// throws qrcodegen::data_too_long, a std::length_error, when no version holds the text
		const qrcodegen::QrCode qr = qrcodegen::QrCode::encodeBinary(bytes, qrcodegen::QrCode::Ecc::HIGH);

		const std::uint32_t modules{ static_cast<std::uint32_t>(qr.getSize()) };
		const std::uint32_t width{ modules * PixelsPerModule };
		// each row of 3-byte pixels is padded to a multiple of 4 bytes
		const std::uint32_t rowBytes{ (width * 3 + 3) / 4 * 4 };
		const std::uint32_t pixelBytes{ rowBytes * width };

		std::vector<std::byte> bitmap;
		bitmap.reserve(FileHeaderSize + InfoHeaderSize + pixelBytes);
		AppendHeaders(bitmap, width, pixelBytes);

		const std::size_t pixelsStart{ bitmap.size() };
		bitmap.resize(pixelsStart + pixelBytes, std::byte{ 0xFF });
		for (std::uint32_t y = 0; y < width; y++)
		{
			const std::size_t row{ pixelsStart + static_cast<std::size_t>(rowBytes) * y };
			for (std::uint32_t x = 0; x < width; x++)
			{
				if (qr.getModule(static_cast<int>(x / PixelsPerModule), static_cast<int>(y / PixelsPerModule)))
				{
					const std::size_t pixel{ row + static_cast<std::size_t>(x) * 3 };
					bitmap[pixel] = bitmap[pixel + 1] = bitmap[pixel + 2] = std::byte{ 0 };
				}
			}
		}
		return bitmap;
	}

	void QrBitmap::AppendLittleEndian(std::vector<std::byte>& out, std::uint32_t value, int byteCount)
	{
		for (int i = 0; i < byteCount; i++)
		{
			out.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFF));
		}
	}

	void QrBitmap::AppendHeaders(std::vector<std::byte>& out, std::uint32_t width, std::uint32_t pixelBytes)
	{
		// BITMAPFILEHEADER: "BM", file size, two reserved words, offset of the pixels
		out.push_back(std::byte{ 'B' });
		out.push_back(std::byte{ 'M' });
		AppendLittleEndian(out, FileHeaderSize + InfoHeaderSize + pixelBytes, 4);
		AppendLittleEndian(out, 0, 4);
		AppendLittleEndian(out, FileHeaderSize + InfoHeaderSize, 4);

		// BITMAPINFOHEADER: a negative height means the rows run top-down
		AppendLittleEndian(out, InfoHeaderSize, 4);
		AppendLittleEndian(out, width, 4);
		AppendLittleEndian(out, static_cast<std::uint32_t>(-static_cast<std::int32_t>(width)), 4);
		AppendLittleEndian(out, 1, 2);     // planes
		AppendLittleEndian(out, 24, 2);    // bits per pixel
		AppendLittleEndian(out, 0, 4);     // BI_RGB
		AppendLittleEndian(out, 0, 4);     // image size: may be 0 for BI_RGB
		AppendLittleEndian(out, 0, 4);     // horizontal resolution
		AppendLittleEndian(out, 0, 4);     // vertical resolution
		AppendLittleEndian(out, 0, 4);     // colours used
		AppendLittleEndian(out, 0, 4);     // important colours
	}
}

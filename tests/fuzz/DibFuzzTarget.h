/**
 * @file DibFuzzTarget.h
 * @brief Declares DibFuzzTarget.
 */
#pragma once

#include "FuzzTarget.h"

#include <cstdint>

/**
 * @brief Fuzzes DittoCore::DibHeader::Read, and reads every byte of the pixels it locates, so
 *        AddressSanitizer reports any layout that reaches past the block.
 */
class DibFuzzTarget final : public FuzzTarget
{
public:
	std::string_view Name() const override { return "dib"; }
	void Run(std::span<const std::byte> input) const override;
	std::vector<std::vector<std::byte>> Seeds() const override;

private:
	/**
	 * @brief A valid DIB: a 40-byte header, a color table and the pixels.
	 * @param bitCount Bits per pixel.
	 * @param compression The biCompression value.
	 * @param tableBytes Size of the color table or bit masks.
	 * @param pixelBytes Size of the pixels.
	 * @return The block.
	 */
	static std::vector<std::byte> Dib(std::uint16_t bitCount, std::uint32_t compression, std::size_t tableBytes, std::size_t pixelBytes);
};

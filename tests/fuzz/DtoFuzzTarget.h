/**
 * @file DtoFuzzTarget.h
 * @brief Declares DtoFuzzTarget.
 */
#pragma once

#include "FuzzTarget.h"

/**
 * @brief Fuzzes DittoCore::DtoCodec::Uncompress: the first 4 input bytes are the declared
 *        original size (as an exported file stores it), the rest the compressed data. Checks
 *        that an accepted result has exactly the declared size and compresses back to itself.
 */
class DtoFuzzTarget final : public FuzzTarget
{
public:
	std::string_view Name() const override { return "dto"; }
	void Run(std::span<const std::byte> input) const override;
	std::vector<std::vector<std::byte>> Seeds() const override;
};

/**
 * @file FileDataFuzzTarget.h
 * @brief Declares FileDataFuzzTarget.
 */
#pragma once

#include "FuzzTarget.h"

/**
 * @brief Fuzzes DittoCore::FileDataRecord::Parse, and checks that every parsed file list
 *        survives FileDataRecord::Build and a second Parse unchanged (round trip).
 */
class FileDataFuzzTarget final : public FuzzTarget
{
public:
	std::string_view Name() const override { return "filedata"; }
	void Run(std::span<const std::byte> input) const override;
	std::vector<std::vector<std::byte>> Seeds() const override;
};

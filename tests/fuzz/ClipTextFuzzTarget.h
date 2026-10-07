/**
 * @file ClipTextFuzzTarget.h
 * @brief Declares ClipTextFuzzTarget.
 */
#pragma once

#include "FuzzTarget.h"

/**
 * @brief Fuzzes every DittoCore::ClipText read (terminated and bounded, 8-bit and UTF-16).
 */
class ClipTextFuzzTarget final : public FuzzTarget
{
public:
	std::string_view Name() const override { return "cliptext"; }
	void Run(std::span<const std::byte> input) const override;
	std::vector<std::vector<std::byte>> Seeds() const override;
};

/**
 * @file RtfFuzzTarget.h
 * @brief Declares RtfFuzzTarget.
 */
#pragma once

#include "FuzzTarget.h"

/**
 * @brief Fuzzes DittoCore::RtfNormalizer::Normalize and DittoCore::RtfJoin with the input as
 *        RTF; checks that normalizing only removes text and that a join of two copies keeps
 *        the outer group.
 */
class RtfFuzzTarget final : public FuzzTarget
{
public:
	std::string_view Name() const override { return "rtf"; }
	void Run(std::span<const std::byte> input) const override;
	std::vector<std::vector<std::byte>> Seeds() const override;
};

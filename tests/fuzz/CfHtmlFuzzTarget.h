/**
 * @file CfHtmlFuzzTarget.h
 * @brief Declares CfHtmlFuzzTarget.
 */
#pragma once

#include "FuzzTarget.h"

/**
 * @brief Fuzzes DittoCore::CfHtml::Parse, and checks that every parsed fragment survives
 *        CfHtml::Build and a second Parse unchanged (round trip).
 */
class CfHtmlFuzzTarget final : public FuzzTarget
{
public:
	std::string_view Name() const override { return "cfhtml"; }
	void Run(std::span<const std::byte> input) const override;
	std::vector<std::vector<std::byte>> Seeds() const override;
};

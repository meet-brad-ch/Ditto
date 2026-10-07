/**
 * @file HdropFuzzTarget.h
 * @brief Declares HdropFuzzTarget.
 */
#pragma once

#include "FuzzTarget.h"

#include <string>

/**
 * @brief Fuzzes CF_HDROP parsing: DittoCore::FileDropList::Parse on the raw bytes and
 *        DittoCore::GlobalFileDrop::Read on the same bytes in a global memory block.
 */
class HdropFuzzTarget final : public FuzzTarget
{
public:
	std::string_view Name() const override { return "hdrop"; }
	void Run(std::span<const std::byte> input) const override;
	std::vector<std::vector<std::byte>> Seeds() const override;

private:
	/**
	 * @brief Builds a CF_HDROP block: a 20-byte DROPFILES header and a double-null-terminated list.
	 * @param pathBytes The encoded paths, each with its terminator.
	 * @param wide Whether the paths are UTF-16.
	 * @return The block.
	 */
	static std::vector<std::byte> Block(const std::string& pathBytes, bool wide);
};

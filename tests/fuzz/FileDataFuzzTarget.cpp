/**
 * @file FileDataFuzzTarget.cpp
 * @brief Implements FileDataFuzzTarget.
 */
#include "FileDataFuzzTarget.h"

#include "ClipboardFormatError.h"
#include "FileDataRecord.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <string>

void FileDataFuzzTarget::Run(std::span<const std::byte> input) const
{
	std::vector<DittoCore::FileDataEntry> files;
	try
	{
		files = DittoCore::FileDataRecord::Parse(input);
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
		return; // malformed lengths or terminators: rejected, as it must be
	}

	// Build must always produce a block that parses back to the same files
	const std::vector<std::byte> built = DittoCore::FileDataRecord::Build(files);
	const std::vector<DittoCore::FileDataEntry> again = DittoCore::FileDataRecord::Parse(built);
	const auto same = [](const DittoCore::FileDataEntry& a, const DittoCore::FileDataEntry& b)
	{
		return a.path == b.path && a.md5 == b.md5 && std::ranges::equal(a.data, b.data);
	};
	if (!std::ranges::equal(files, again, same))
	{
		std::abort(); // a finding: Build and Parse disagree
	}
}

std::vector<std::vector<std::byte>> FileDataFuzzTarget::Seeds() const
{
	const std::string md5(DittoCore::FileDataRecord::Md5Length, 'a');
	const std::string version1 = std::string("C:\\a.txt") + '\0' + md5 + '\0' + "contents";
	std::vector<std::byte> first(version1.size());
	std::memcpy(first.data(), version1.data(), version1.size());

	const std::string text = "two";
	const std::vector<std::byte> data(reinterpret_cast<const std::byte*>(text.data()), reinterpret_cast<const std::byte*>(text.data()) + text.size());
	const std::vector<DittoCore::FileDataEntry> files{ { "C:\\a.txt", md5, data }, { "C:\\b.bin", md5, {} } };
	return { first, DittoCore::FileDataRecord::Build(files) };
}

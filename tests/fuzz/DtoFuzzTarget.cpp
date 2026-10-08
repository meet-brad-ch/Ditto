/**
 * @file DtoFuzzTarget.cpp
 * @brief Implements DtoFuzzTarget.
 */
#include "DtoFuzzTarget.h"

#include "ClipboardFormatError.h"
#include "DtoCodec.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <string>

void DtoFuzzTarget::Run(std::span<const std::byte> input) const
{
	if (input.size() < sizeof(std::int32_t))
	{
		return;
	}
	std::int32_t declared{};
	std::memcpy(&declared, input.data(), sizeof(declared));
	const std::span<const std::byte> compressed = input.subspan(sizeof(declared));

	std::vector<std::byte> data;
	try
	{
		data = DittoCore::DtoCodec::Uncompress(compressed, declared);
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
		return; // a lying size or a broken stream: rejected, as it must be
	}

	const std::vector<std::byte> again = DittoCore::DtoCodec::Uncompress(DittoCore::DtoCodec::Compress(data), declared);
	if (static_cast<std::int64_t>(data.size()) != declared || again != data)
	{
		std::abort(); // a finding: the size check or the round trip failed
	}
}

std::vector<std::vector<std::byte>> DtoFuzzTarget::Seeds() const
{
	const std::string text = "Ditto clip text, Ditto clip text";
	std::vector<std::byte> data(text.size());
	std::memcpy(data.data(), text.data(), text.size());
	const std::vector<std::byte> compressed = DittoCore::DtoCodec::Compress(data);

	const std::int32_t declared{ static_cast<std::int32_t>(data.size()) };
	std::vector<std::byte> seed(sizeof(declared));
	std::memcpy(seed.data(), &declared, sizeof(declared));
	seed.insert(seed.end(), compressed.begin(), compressed.end());
	return { seed };
}

/**
 * @file RtfFuzzTarget.cpp
 * @brief Implements RtfFuzzTarget.
 */
#include "RtfFuzzTarget.h"

#include "ClipboardFormatError.h"
#include "RtfJoin.h"
#include "RtfNormalizer.h"

#include <cstdlib>
#include <cstring>
#include <string>

void RtfFuzzTarget::Run(std::span<const std::byte> input) const
{
	const std::string_view rtf(reinterpret_cast<const char*>(input.data()), input.size());

	if (DittoCore::RtfNormalizer::Normalize(rtf).size() > rtf.size())
	{
		std::abort(); // a finding: normalizing must only remove text
	}

	DittoCore::RtfJoin join(L"-\r\n");
	try
	{
		join.Add(rtf);
		join.Add(rtf);
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
		return; // not an RTF document: rejected, as it must be
	}
	const std::string joined = join.Result();
	if (!joined.starts_with("{\\rtf1") || !joined.ends_with('}'))
	{
		std::abort(); // a finding: the join lost its outer group
	}
}

std::vector<std::vector<std::byte>> RtfFuzzTarget::Seeds() const
{
	const std::string rtf = "{\\rtf1\\ansi{\\fonttbl{\\f0 Arial;}}{\\*\\datastore 01{\\x}}\\rsid12\\insrsid3\\mdispDef1 \\f0 text\\par}";
	std::vector<std::byte> seed(rtf.size());
	std::memcpy(seed.data(), rtf.data(), rtf.size());
	return { seed };
}

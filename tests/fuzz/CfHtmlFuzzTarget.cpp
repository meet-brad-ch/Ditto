/**
 * @file CfHtmlFuzzTarget.cpp
 * @brief Implements CfHtmlFuzzTarget.
 */
#include "CfHtmlFuzzTarget.h"

#include "CfHtml.h"
#include "ClipboardFormatError.h"

#include <cstdlib>
#include <cstring>
#include <string>

void CfHtmlFuzzTarget::Run(std::span<const std::byte> input) const
{
	DittoCore::CfHtmlFragment parsed{};
	try
	{
		parsed = DittoCore::CfHtml::Parse(input);
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
		return;   // malformed header or offsets: rejected, as it must be
	}

	// Build must always produce a block that parses back to the same fragment
	const std::string built = DittoCore::CfHtml::Build(parsed.fragment, parsed.version, parsed.sourceUrl);
	const DittoCore::CfHtmlFragment again = DittoCore::CfHtml::Parse(std::as_bytes(std::span(built)));
	if (again.fragment != parsed.fragment)
	{
		std::abort();   // a finding: Build and Parse disagree
	}
}

std::vector<std::vector<std::byte>> CfHtmlFuzzTarget::Seeds() const
{
	const std::string html = DittoCore::CfHtml::Build("<b>bold</b> \xC3\xA9", "1.0", "https://example.com/");
	std::vector<std::byte> seed(html.size());
	std::memcpy(seed.data(), html.data(), html.size());
	return { seed };
}

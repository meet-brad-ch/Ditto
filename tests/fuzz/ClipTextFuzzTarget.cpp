/**
 * @file ClipTextFuzzTarget.cpp
 * @brief Implements ClipTextFuzzTarget.
 */
#include "ClipTextFuzzTarget.h"

#include "ClipText.h"
#include "ClipboardFormatError.h"

#include <cstring>
#include <string>

void ClipTextFuzzTarget::Run(std::span<const std::byte> input) const
{
	static_cast<void>(DittoCore::ClipText::ReadAnsiBounded(input).size());
	static_cast<void>(DittoCore::ClipText::ReadWideBounded(input).size());
	try
	{
		static_cast<void>(DittoCore::ClipText::ReadAnsi(input.data(), input.size()).size());
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
		// no terminator: rejected, as it must be
	}
	try
	{
		static_cast<void>(DittoCore::ClipText::ReadWide(input.data(), input.size()).size());
	}
	catch (const DittoCore::ClipboardFormatError&)
	{
	}
}

std::vector<std::vector<std::byte>> ClipTextFuzzTarget::Seeds() const
{
	const std::string ansi("hello\0", 6);
	const std::wstring wide(L"hello\0", 6);
	std::vector<std::byte> ansiBytes(ansi.size());
	std::memcpy(ansiBytes.data(), ansi.data(), ansi.size());
	std::vector<std::byte> wideBytes(wide.size() * sizeof(wchar_t));
	std::memcpy(wideBytes.data(), wide.data(), wideBytes.size());
	return { ansiBytes, wideBytes };
}

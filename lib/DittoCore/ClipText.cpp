/**
 * @file ClipText.cpp
 * @brief Implements DittoCore::ClipText.
 */
#include "ClipText.h"
#include "ClipboardFormatError.h"

#include <cstring>

namespace DittoCore
{
	std::string ClipText::ReadAnsi(const void* data, std::size_t size)
	{
		if (data == nullptr)
		{
			throw ClipboardFormatError("CF_TEXT block has no data");
		}
		const char* text = static_cast<const char*>(data);
		const void* terminator = std::memchr(text, '\0', size);
		if (terminator == nullptr)
		{
			throw ClipboardFormatError("CF_TEXT block of " + std::to_string(size) + " bytes has no terminating null");
		}
		return std::string(text, static_cast<const char*>(terminator));
	}

	std::string ClipText::ReadAnsiBounded(const void* data, std::size_t size)
	{
		if (data == nullptr)
		{
			throw ClipboardFormatError("text block has no data");
		}
		const char* text = static_cast<const char*>(data);
		const void* terminator = std::memchr(text, '\0', size);
		const char* end = terminator != nullptr ? static_cast<const char*>(terminator) : text + size;
		return std::string(text, end);
	}

	std::string ClipText::ReadAnsiBounded(std::span<const std::byte> block)
	{
		return ReadAnsiBounded(block.data(), block.size());
	}

	std::wstring ClipText::ReadWideBounded(std::span<const std::byte> block)
	{
		const std::size_t characters = block.size() / sizeof(wchar_t);
		std::wstring text;
		text.reserve(characters);
		for (std::size_t i = 0; i < characters; ++i)
		{
			wchar_t c{};
			std::memcpy(&c, block.data() + i * sizeof(wchar_t), sizeof(c));
			if (c == L'\0')
			{
				break;
			}
			text.push_back(c);
		}
		return text;
	}

	std::wstring ClipText::ReadWide(const void* data, std::size_t size)
	{
		if (data == nullptr)
		{
			throw ClipboardFormatError("CF_UNICODETEXT block has no data");
		}
		const std::size_t characters = size / sizeof(wchar_t);
		const char* bytes = static_cast<const char*>(data);
		std::wstring text;
		for (std::size_t i = 0; i < characters; ++i)
		{
			wchar_t c = 0;
			std::memcpy(&c, bytes + i * sizeof(wchar_t), sizeof(c));
			if (c == L'\0')
			{
				return text;
			}
			text.push_back(c);
		}
		throw ClipboardFormatError("CF_UNICODETEXT block of " + std::to_string(size) + " bytes has no terminating null");
	}
}

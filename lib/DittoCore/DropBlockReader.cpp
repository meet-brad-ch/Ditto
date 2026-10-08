/**
 * @file DropBlockReader.cpp
 * @brief Implements DittoCore::DropBlockReader.
 */
#include "DropBlockReader.h"
#include "ClipboardFormatError.h"

#include <windows.h>

#include <cstring>
#include <utility>

namespace DittoCore
{
	DropBlockReader::DropBlockReader(const void* data, std::size_t size) :
		m_bytes(static_cast<const std::uint8_t*>(data)),
		m_size(size)
	{
		if (m_bytes == nullptr)
		{
			throw ClipboardFormatError("CF_HDROP block has no data");
		}
		if (m_size < HeaderSize)
		{
			throw ClipboardFormatError("CF_HDROP block of " + std::to_string(m_size) +
									   " bytes is smaller than the 20-byte DROPFILES header");
		}
	}

	std::vector<std::wstring> DropBlockReader::ReadPaths() const
	{
		const std::size_t listStart = ReadUint32(FilesOffsetPosition);
		if (listStart < HeaderSize || listStart >= m_size)
		{
			throw ClipboardFormatError("CF_HDROP path list offset " + std::to_string(listStart) +
									   " is outside the block (header 20 bytes, block " + std::to_string(m_size) + " bytes)");
		}
		return ReadUint32(WideFlagPosition) != 0 ? ReadWideList(listStart) : ReadAnsiList(listStart);
	}

	std::uint32_t DropBlockReader::ReadUint32(std::size_t position) const
	{
		std::uint32_t value = 0;
		std::memcpy(&value, m_bytes + position, sizeof(value));
		return value;
	}

	std::vector<std::wstring> DropBlockReader::ReadWideList(std::size_t position) const
	{
		std::vector<std::wstring> paths;
		std::wstring current;
		while (position + sizeof(wchar_t) <= m_size)
		{
			wchar_t c = 0;
			std::memcpy(&c, m_bytes + position, sizeof(c));
			position += sizeof(c);
			if (c != L'\0')
			{
				current.push_back(c);
				continue;
			}
			if (current.empty())
			{
				return paths; // the empty entry ends the list
			}
			paths.push_back(std::move(current));
			current.clear();
		}
		throw ClipboardFormatError("CF_HDROP wide path list is not terminated within the block");
	}

	std::vector<std::wstring> DropBlockReader::ReadAnsiList(std::size_t position) const
	{
		std::vector<std::wstring> paths;
		std::string current;
		while (position < m_size)
		{
			const char c = static_cast<char>(m_bytes[position]);
			++position;
			if (c != '\0')
			{
				current.push_back(c);
				continue;
			}
			if (current.empty())
			{
				return paths;
			}
			paths.push_back(FromAnsiCodePage(current));
			current.clear();
		}
		throw ClipboardFormatError("CF_HDROP ANSI path list is not terminated within the block");
	}

	std::wstring DropBlockReader::FromAnsiCodePage(const std::string& text)
	{
		const int length = static_cast<int>(text.size());
		const int wideLength = ::MultiByteToWideChar(CP_ACP, 0, text.data(), length, nullptr, 0);
		if (wideLength <= 0)
		{
			throw ClipboardFormatError("CF_HDROP ANSI path cannot be converted from the ANSI code page");
		}
		std::wstring wide(static_cast<std::size_t>(wideLength), L'\0');
		::MultiByteToWideChar(CP_ACP, 0, text.data(), length, wide.data(), wideLength);
		return wide;
	}
}

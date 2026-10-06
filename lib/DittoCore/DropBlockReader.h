#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DittoCore
{
	// Reads the CF_HDROP layout from raw bytes: a 20-byte DROPFILES header (DWORD pFiles,
	// POINT pt, BOOL fNC, BOOL fWide) and the path list it points to. Every read checks the
	// position against the block size first. Used by FileDropList.
	class DropBlockReader
	{
	public:
		static constexpr std::size_t HeaderSize = 20;

		// Throws ClipboardFormatError when data is null or shorter than the header.
		DropBlockReader(const void* data, std::size_t size);

		// Throws ClipboardFormatError when the list starts outside the block or is not
		// terminated within it.
		std::vector<std::wstring> ReadPaths() const;

	private:
		static constexpr std::size_t FilesOffsetPosition = 0;
		static constexpr std::size_t WideFlagPosition = 16;

		std::uint32_t ReadUint32(std::size_t position) const;
		std::vector<std::wstring> ReadWideList(std::size_t position) const;
		std::vector<std::wstring> ReadAnsiList(std::size_t position) const;
		static std::wstring FromAnsiCodePage(const std::string& text);

		const std::uint8_t* m_bytes;
		std::size_t m_size;
	};
}

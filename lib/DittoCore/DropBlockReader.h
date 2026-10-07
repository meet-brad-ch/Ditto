/**
 * @file DropBlockReader.h
 * @brief Declares DittoCore::DropBlockReader.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace DittoCore
{
	/**
	 * @brief Reads the CF_HDROP layout from raw bytes, checking every position against the block size.
	 *
	 * The layout is a 20-byte DROPFILES header (DWORD pFiles, POINT pt, BOOL fNC, BOOL fWide)
	 * followed by null-terminated paths and an empty entry that ends the list. Used by FileDropList.
	 */
	class DropBlockReader
	{
	public:
		/// Size of the DROPFILES header in bytes.
		static constexpr std::size_t HeaderSize{ 20 };
		/// Offset of DROPFILES::pFiles in the header.
		static constexpr std::size_t FilesOffsetPosition{ 0 };
		/// Offset of DROPFILES::fWide in the header.
		static constexpr std::size_t WideFlagPosition{ 16 };

		/**
		 * @brief Prepares reading a CF_HDROP block.
		 * @param data Start of the block.
		 * @param size Size of the block in bytes.
		 * @throws ClipboardFormatError When @p data is null or shorter than the header.
		 */
		DropBlockReader(const void* data, std::size_t size);

		/**
		 * @brief Reads the path list.
		 * @return The paths, converted to UTF-16 when the block holds ANSI paths.
		 * @throws ClipboardFormatError When the list starts outside the block, is not terminated
		 *         within it, or an ANSI path cannot be converted.
		 */
		std::vector<std::wstring> ReadPaths() const;

	private:
		/**
		 * @brief Reads a 32-bit value from the header.
		 * @param position Byte offset, inside the header.
		 * @return The value at @p position.
		 */
		std::uint32_t ReadUint32(std::size_t position) const;

		/**
		 * @brief Reads a list of UTF-16 paths.
		 * @param position Byte offset of the first path.
		 * @return The paths before the empty entry.
		 * @throws ClipboardFormatError When the list is not terminated within the block.
		 */
		std::vector<std::wstring> ReadWideList(std::size_t position) const;

		/**
		 * @brief Reads a list of ANSI paths.
		 * @param position Byte offset of the first path.
		 * @return The paths before the empty entry, converted to UTF-16.
		 * @throws ClipboardFormatError When the list is not terminated within the block or a
		 *         path cannot be converted.
		 */
		std::vector<std::wstring> ReadAnsiList(std::size_t position) const;

		/**
		 * @brief Converts a path from the ANSI code page.
		 * @param text The ANSI path.
		 * @return The path as UTF-16.
		 * @throws ClipboardFormatError When the conversion fails.
		 */
		static std::wstring FromAnsiCodePage(const std::string& text);

		/// Start of the block.
		const std::uint8_t* m_bytes{};
		/// Size of the block in bytes.
		std::size_t m_size{};
	};
}

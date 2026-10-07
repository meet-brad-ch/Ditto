/**
 * @file FileDataRecord.h
 * @brief Declares DittoCore::FileDataRecord.
 */
#pragma once

#include "FileDataEntry.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace DittoCore
{
	/**
	 * @brief Reads and writes the "Ditto File Data" clipboard format, which stores the contents
	 *        of copied files in the clip.
	 *
	 * Version 1 (written by upstream Ditto) holds one file:
	 * `path \0 md5 \0 contents`, with the contents running to the end of the block.
	 *
	 * Version 2 holds any number of files: the magic "DFD" 0x02, a 32-bit file count, then per
	 * file a 32-bit path length, the path, the 32 MD5 characters, a 64-bit content length and
	 * the contents. All numbers are little-endian. A version-1 block cannot start with the magic,
	 * because 0x02 is not allowed in a Windows path.
	 */
	class FileDataRecord
	{
	public:
		/// Length of an MD5 written as hexadecimal characters.
		static constexpr std::size_t Md5Length{ 32 };

		/**
		 * @brief Parses a block of either version.
		 * @param block The block's bytes.
		 * @return The files; the entries view @p block.
		 * @throws ClipboardFormatError When a length, terminator or MD5 does not fit the block,
		 *         or bytes are left over after the last file of a version-2 block.
		 */
		static std::vector<FileDataEntry> Parse(std::span<const std::byte> block);

		/**
		 * @brief Builds a version-2 block.
		 * @param files The files; each MD5 must be Md5Length characters.
		 * @return The block.
		 * @throws ClipboardFormatError When an MD5 has the wrong length or a path is longer than
		 *         a 32-bit length can describe.
		 */
		static std::vector<std::byte> Build(std::span<const FileDataEntry> files);

	private:
		/// The first bytes of a version-2 block: "DFD" and the version.
		static constexpr std::array<std::byte, 4> Magic{ std::byte{ 'D' }, std::byte{ 'F' }, std::byte{ 'D' }, std::byte{ 2 } };

		/**
		 * @brief Parses a version-1 block.
		 * @param block The block's bytes.
		 * @return The one file.
		 * @throws ClipboardFormatError When the path or MD5 has no terminator or the MD5 has the wrong length.
		 */
		static FileDataEntry ParseVersion1(std::span<const std::byte> block);

		/**
		 * @brief Parses a version-2 block.
		 * @param block The block's bytes, starting with the magic.
		 * @return The files.
		 * @throws ClipboardFormatError When a length runs past the block or bytes are left over.
		 */
		static std::vector<FileDataEntry> ParseVersion2(std::span<const std::byte> block);

		/**
		 * @brief Checks that a file has a path and an MD5 of Md5Length characters.
		 * @param file The file.
		 * @throws ClipboardFormatError When it does not.
		 */
		static void ValidateEntry(const FileDataEntry& file);

		/**
		 * @brief Appends a 32-bit little-endian value.
		 * @param block The block being built.
		 * @param value The value.
		 */
		static void AppendUInt32(std::vector<std::byte>& block, std::uint32_t value);

		/**
		 * @brief Appends a 64-bit little-endian value.
		 * @param block The block being built.
		 * @param value The value.
		 */
		static void AppendUInt64(std::vector<std::byte>& block, std::uint64_t value);

		/**
		 * @brief Appends text as bytes.
		 * @param block The block being built.
		 * @param text The text.
		 */
		static void AppendText(std::vector<std::byte>& block, std::string_view text);
	};
}

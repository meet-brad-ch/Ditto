/**
 * @file FileDataEntry.h
 * @brief Declares DittoCore::FileDataEntry.
 */
#pragma once

#include <cstddef>
#include <span>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief One file stored in a "Ditto File Data" clip: its path, its MD5 and its contents.
	 *
	 * The members view memory owned by someone else (the parsed block, or the caller's buffers
	 * for FileDataRecord::Build); an entry is valid only while that memory is.
	 */
	struct FileDataEntry
	{
		/// The original path of the file, UTF-8.
		std::string_view path{};
		/// The MD5 of the contents as 32 hexadecimal characters, computed when the file was copied.
		std::string_view md5{};
		/// The file's contents.
		std::span<const std::byte> data{};
	};
}

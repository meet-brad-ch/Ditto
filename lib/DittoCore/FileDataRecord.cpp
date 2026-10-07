/**
 * @file FileDataRecord.cpp
 * @brief Implements DittoCore::FileDataRecord.
 */
#include "FileDataRecord.h"
#include "ByteCursor.h"
#include "ClipboardFormatError.h"

#include <algorithm>
#include <limits>
#include <string>

namespace DittoCore
{
	std::vector<FileDataEntry> FileDataRecord::Parse(std::span<const std::byte> block)
	{
		if (block.size() >= Magic.size() && std::equal(Magic.begin(), Magic.end(), block.begin()))
		{
			return ParseVersion2(block);
		}
		return { ParseVersion1(block) };
	}

	std::vector<std::byte> FileDataRecord::Build(std::span<const FileDataEntry> files)
	{
		if (files.size() > std::numeric_limits<std::uint32_t>::max())
		{
			throw ClipboardFormatError("file data of " + std::to_string(files.size()) + " files is too large");
		}
		std::vector<std::byte> block(Magic.begin(), Magic.end());
		AppendUInt32(block, static_cast<std::uint32_t>(files.size()));
		for (const FileDataEntry& file : files)
		{
			ValidateEntry(file);
			if (file.path.size() > std::numeric_limits<std::uint32_t>::max())
			{
				throw ClipboardFormatError("file data path of " + std::to_string(file.path.size()) + " bytes is too long");
			}
			AppendUInt32(block, static_cast<std::uint32_t>(file.path.size()));
			AppendText(block, file.path);
			AppendText(block, file.md5);
			AppendUInt64(block, file.data.size());
			block.insert(block.end(), file.data.begin(), file.data.end());
		}
		return block;
	}

	FileDataEntry FileDataRecord::ParseVersion1(std::span<const std::byte> block)
	{
		const auto pathEnd = std::find(block.begin(), block.end(), std::byte{ 0 });
		if (pathEnd == block.end())
		{
			throw ClipboardFormatError("version-1 file data has no terminator after its path");
		}
		const auto md5Begin = pathEnd + 1;
		const auto md5End = std::find(md5Begin, block.end(), std::byte{ 0 });
		if (md5End == block.end())
		{
			throw ClipboardFormatError("version-1 file data has no terminator after its MD5");
		}

		FileDataEntry file{};
		file.path = std::string_view(reinterpret_cast<const char*>(block.data()), static_cast<std::size_t>(pathEnd - block.begin()));
		file.md5 = std::string_view(reinterpret_cast<const char*>(block.data()) + (md5Begin - block.begin()), static_cast<std::size_t>(md5End - md5Begin));
		file.data = block.subspan(static_cast<std::size_t>(md5End - block.begin()) + 1);
		ValidateEntry(file);
		return file;
	}

	std::vector<FileDataEntry> FileDataRecord::ParseVersion2(std::span<const std::byte> block)
	{
		ByteCursor cursor(block);
		cursor.Take(Magic.size());
		const std::uint32_t count = cursor.ReadUInt32();

		std::vector<FileDataEntry> files;
		for (std::uint32_t i = 0; i < count; i++)
		{
			FileDataEntry file{};
			file.path = cursor.TakeText(cursor.ReadUInt32());
			file.md5 = cursor.TakeText(Md5Length);
			file.data = cursor.Take(cursor.ReadUInt64());
			ValidateEntry(file);
			files.push_back(file);
		}
		if (cursor.Remaining() != 0)
		{
			throw ClipboardFormatError("file data has " + std::to_string(cursor.Remaining()) + " bytes after its last file");
		}
		return files;
	}

	void FileDataRecord::ValidateEntry(const FileDataEntry& file)
	{
		if (file.path.empty())
		{
			throw ClipboardFormatError("file data has an empty path");
		}
		if (file.md5.size() != Md5Length)
		{
			throw ClipboardFormatError("file data MD5 has " + std::to_string(file.md5.size()) + " characters, not " + std::to_string(Md5Length));
		}
	}

	void FileDataRecord::AppendUInt32(std::vector<std::byte>& block, std::uint32_t value)
	{
		for (int shift = 0; shift < 32; shift += 8)
		{
			block.push_back(static_cast<std::byte>((value >> shift) & 0xFF));
		}
	}

	void FileDataRecord::AppendUInt64(std::vector<std::byte>& block, std::uint64_t value)
	{
		AppendUInt32(block, static_cast<std::uint32_t>(value & 0xFFFFFFFF));
		AppendUInt32(block, static_cast<std::uint32_t>(value >> 32));
	}

	void FileDataRecord::AppendText(std::vector<std::byte>& block, std::string_view text)
	{
		const auto* bytes = reinterpret_cast<const std::byte*>(text.data());
		block.insert(block.end(), bytes, bytes + text.size());
	}
}

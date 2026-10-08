/**
 * @file FileDropList.cpp
 * @brief Implements DittoCore::FileDropList.
 */
#include "FileDropList.h"
#include "ClipboardFormatError.h"
#include "DropBlockReader.h"

#include <cstdint>
#include <cstring>
#include <utility>

namespace DittoCore
{
	FileDropList FileDropList::Parse(const void* data, std::size_t size)
	{
		return FileDropList(DropBlockReader(data, size).ReadPaths());
	}

	std::vector<std::byte> FileDropList::Build(std::span<const std::wstring> paths)
	{
		// DROPFILES: pFiles (where the list starts), pt.x, pt.y, fNC, fWide
		std::vector<std::byte> block(DropBlockReader::HeaderSize);
		const std::uint32_t filesOffset{ static_cast<std::uint32_t>(DropBlockReader::HeaderSize) };
		const std::uint32_t wide{ 1 };
		std::memcpy(block.data() + DropBlockReader::FilesOffsetPosition, &filesOffset, sizeof(filesOffset));
		std::memcpy(block.data() + DropBlockReader::WideFlagPosition, &wide, sizeof(wide));

		for (const std::wstring& path : paths)
		{
			if (path.empty() || path.find(L'\0') != std::wstring::npos)
			{
				throw ClipboardFormatError("a CF_HDROP path is empty or contains a null character");
			}
			AppendWide(block, path);
		}
		AppendWide(block, L""); // the empty path that ends the list
		return block;
	}

	FileDropList::FileDropList(std::vector<std::wstring> paths) :
		m_paths(std::move(paths))
	{
	}

	void FileDropList::AppendWide(std::vector<std::byte>& block, std::wstring_view text)
	{
		const std::size_t start = block.size();
		block.resize(start + (text.size() + 1) * sizeof(wchar_t));
		std::memcpy(block.data() + start, text.data(), text.size() * sizeof(wchar_t));
	}
}

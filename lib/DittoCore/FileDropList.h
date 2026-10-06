#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace DittoCore
{
	// The file paths of a CF_HDROP block: a DROPFILES header followed by a list of
	// null-terminated paths that ends with an empty string. The block is read from its
	// raw bytes with every offset and length checked against the block size, so a path
	// of any length is read without a fixed-size buffer.
	class FileDropList
	{
	public:
		// Parses a CF_HDROP block of 'size' bytes at 'data'.
		// Throws ClipboardFormatError when the block is shorter than its header, the path
		// list starts outside the block, or the list is not terminated within the block.
		static FileDropList Parse(const void* data, std::size_t size);

		const std::vector<std::wstring>& Paths() const & { return m_paths; }

		// On a temporary list the paths are moved out, so
		// 'for (auto& p : FileDropList::Parse(...).Paths())' never refers to a destroyed list.
		std::vector<std::wstring> Paths() && { return std::move(m_paths); }

	private:
		explicit FileDropList(std::vector<std::wstring> paths);

		std::vector<std::wstring> m_paths;
	};
}

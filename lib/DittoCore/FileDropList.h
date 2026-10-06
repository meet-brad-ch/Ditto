/**
 * @file FileDropList.h
 * @brief Declares DittoCore::FileDropList.
 */
#pragma once

#include <cstddef>
#include <string>
#include <utility>
#include <vector>

namespace DittoCore
{
	/**
	 * @brief The file paths of a CF_HDROP block.
	 *
	 * Read from the raw bytes with every offset and length checked against the block size, so
	 * a path of any length is read without a fixed-size buffer.
	 */
	class FileDropList
	{
	public:
		/**
		 * @brief Parses a CF_HDROP block.
		 * @param data Start of the block.
		 * @param size Size of the block in bytes.
		 * @return The paths the block lists.
		 * @throws ClipboardFormatError When the block is shorter than its header, the path list
		 *         starts outside the block, or the list is not terminated within the block.
		 */
		static FileDropList Parse(const void* data, std::size_t size);

		/**
		 * @brief The paths of this list.
		 * @return The paths, valid while the list lives.
		 */
		const std::vector<std::wstring>& Paths() const & { return m_paths; }

		/**
		 * @brief The paths of a temporary list, moved out.
		 *
		 * Makes `for (auto& p : FileDropList::Parse(...).Paths())` safe: the loop owns the
		 * paths instead of referring to a destroyed list.
		 * @return The paths.
		 */
		std::vector<std::wstring> Paths() && { return std::move(m_paths); }

	private:
		/**
		 * @brief Creates the list.
		 * @param paths The parsed paths.
		 */
		explicit FileDropList(std::vector<std::wstring> paths);

		/// The parsed paths.
		std::vector<std::wstring> m_paths{};
	};
}

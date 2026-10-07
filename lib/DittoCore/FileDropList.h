/**
 * @file FileDropList.h
 * @brief Declares DittoCore::FileDropList.
 */
#pragma once

#include <cstddef>
#include <span>
#include <string_view>
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
		 * @brief Builds a CF_HDROP block: a DROPFILES header, then each path as UTF-16 with its
		 *        terminator, then an empty path that ends the list.
		 * @param paths The paths.
		 * @return The block.
		 * @throws ClipboardFormatError When a path is empty or contains a null character; either
		 *         would end the list early.
		 */
		static std::vector<std::byte> Build(std::span<const std::wstring> paths);

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

		/**
		 * @brief Appends UTF-16 text and its terminator.
		 * @param block The block being built.
		 * @param text The text.
		 */
		static void AppendWide(std::vector<std::byte>& block, std::wstring_view text);

		/// The parsed paths.
		std::vector<std::wstring> m_paths{};
	};
}

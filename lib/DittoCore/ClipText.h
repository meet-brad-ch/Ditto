/**
 * @file ClipText.h
 * @brief Declares DittoCore::ClipText.
 */
#pragma once

#include <cstddef>
#include <span>
#include <string>

namespace DittoCore
{
	/**
	 * @brief Reads the text of a clipboard text block with every access inside the block.
	 *
	 * CF_TEXT and CF_UNICODETEXT are null-terminated by definition; CF_HTML and RTF are
	 * length-delimited, so for them the terminator is optional.
	 */
	class ClipText
	{
	public:
		/**
		 * @brief Reads CF_TEXT: the characters before the first null.
		 * @param data Start of the block.
		 * @param size Size of the block in bytes.
		 * @return The text before the terminating null; bytes after it are ignored.
		 * @throws ClipboardFormatError When @p data is null or no null lies within @p size bytes.
		 */
		static std::string ReadAnsi(const void* data, std::size_t size);

		/**
		 * @brief Reads CF_UNICODETEXT: the UTF-16 characters before the first null.
		 * @param data Start of the block.
		 * @param size Size of the block in bytes; a trailing odd byte is not part of any character.
		 * @return The text before the terminating null; characters after it are ignored.
		 * @throws ClipboardFormatError When @p data is null or no null character lies within the block.
		 */
		static std::wstring ReadWide(const void* data, std::size_t size);

		/**
		 * @brief Reads a length-delimited text format (CF_HTML, RTF).
		 * @param data Start of the block.
		 * @param size Size of the block in bytes.
		 * @return The characters before the first null, or all @p size bytes when there is none.
		 * @throws ClipboardFormatError When @p data is null.
		 */
		static std::string ReadAnsiBounded(const void* data, std::size_t size);

		/**
		 * @brief Reads 8-bit text up to the first null, or the whole block when there is none.
		 * @param block The block's bytes.
		 * @return The characters before the first null.
		 */
		static std::string ReadAnsiBounded(std::span<const std::byte> block);

		/**
		 * @brief Reads UTF-16 text up to the first null character, or the whole block when there is none.
		 * @param block The block's bytes; a trailing odd byte is not part of any character.
		 * @return The characters before the first null character.
		 */
		static std::wstring ReadWideBounded(std::span<const std::byte> block);
	};
}

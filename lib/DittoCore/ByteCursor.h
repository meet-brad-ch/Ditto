/**
 * @file ByteCursor.h
 * @brief Declares DittoCore::ByteCursor.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace DittoCore
{
	/**
	 * @brief Reads little-endian values and byte runs from a block front to back, checking every
	 *        read against the bytes left.
	 */
	class ByteCursor
	{
	public:
		/**
		 * @brief Starts reading at the first byte.
		 * @param block The bytes; they must outlive the cursor and every view it returns.
		 */
		explicit ByteCursor(std::span<const std::byte> block) noexcept;

		/**
		 * @brief Reads a 32-bit little-endian value.
		 * @return The value.
		 * @throws ClipboardFormatError When fewer than 4 bytes are left.
		 */
		std::uint32_t ReadUInt32();

		/**
		 * @brief Reads a 64-bit little-endian value.
		 * @return The value.
		 * @throws ClipboardFormatError When fewer than 8 bytes are left.
		 */
		std::uint64_t ReadUInt64();

		/**
		 * @brief Takes the next bytes.
		 * @param count How many.
		 * @return A view of them.
		 * @throws ClipboardFormatError When fewer than @p count bytes are left.
		 */
		std::span<const std::byte> Take(std::uint64_t count);

		/**
		 * @brief Takes the next bytes as text.
		 * @param count How many.
		 * @return A view of them.
		 * @throws ClipboardFormatError When fewer than @p count bytes are left.
		 */
		std::string_view TakeText(std::uint64_t count);

		/**
		 * @brief The bytes not read yet.
		 * @return The count.
		 */
		std::size_t Remaining() const noexcept;

	private:
		/// The bytes not read yet.
		std::span<const std::byte> m_rest;
	};
}

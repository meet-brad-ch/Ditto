/**
 * @file Crc32.h
 * @brief Declares DittoCore::Crc32.
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <span>

namespace DittoCore
{
	/**
	 * @brief The CRC-32 (PKZip/zlib polynomial) of data added in pieces; Ditto stores it per clip
	 *        to find duplicates.
	 *
	 * The value equals one CRC-32 over all the pieces in order: the same value the CRC code that
	 * Ditto used before (table, start 0xFFFFFFFF, inverted at the end) gave, so the CRCs already
	 * stored in a database keep matching.
	 */
	class Crc32
	{
	public:
		/**
		 * @brief Adds the next piece of data.
		 * @param bytes The data; may be empty.
		 */
		void Add(std::span<const std::byte> bytes);

		/**
		 * @brief The CRC-32 of everything added so far (0 for nothing).
		 * @return The CRC.
		 */
		std::uint32_t Value() const;

	private:
		/// The running CRC, as zlib's crc32() returns it after each piece.
		std::uint32_t m_crc{};
	};
}

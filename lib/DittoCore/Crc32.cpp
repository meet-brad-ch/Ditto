/**
 * @file Crc32.cpp
 * @brief Implements DittoCore::Crc32.
 */
#include "Crc32.h"

#include <zlib.h>

#include <algorithm>
#include <limits>

namespace DittoCore
{
	void Crc32::Add(std::span<const std::byte> bytes)
	{
		// zlib takes the length as uInt: feed blocks larger than that in parts
		while (!bytes.empty())
		{
			const std::size_t part{ (std::min)(bytes.size(), static_cast<std::size_t>((std::numeric_limits<uInt>::max)())) };
			m_crc = static_cast<std::uint32_t>(crc32(m_crc, reinterpret_cast<const Bytef*>(bytes.data()), static_cast<uInt>(part)));
			bytes = bytes.subspan(part);
		}
	}

	std::uint32_t Crc32::Value() const
	{
		return m_crc;
	}
}

/**
 * @file ByteCursor.cpp
 * @brief Implements DittoCore::ByteCursor.
 */
#include "ByteCursor.h"
#include "ClipboardFormatError.h"

#include <string>

namespace DittoCore
{
	ByteCursor::ByteCursor(std::span<const std::byte> block) noexcept
		:
		m_rest(block)
	{
	}

	std::uint32_t ByteCursor::ReadUInt32()
	{
		const std::span<const std::byte> bytes = Take(4);
		std::uint32_t value{};
		for (std::size_t i = 0; i < bytes.size(); i++)
		{
			value |= static_cast<std::uint32_t>(bytes[i]) << (8 * i);
		}
		return value;
	}

	std::uint64_t ByteCursor::ReadUInt64()
	{
		const std::uint64_t low = ReadUInt32();
		const std::uint64_t high = ReadUInt32();
		return low | (high << 32);
	}

	std::span<const std::byte> ByteCursor::Take(std::uint64_t count)
	{
		if (count > m_rest.size())
		{
			throw ClipboardFormatError("a field of " + std::to_string(count) + " bytes runs past the " + std::to_string(m_rest.size()) + " bytes left in the block");
		}
		const std::span<const std::byte> taken = m_rest.first(static_cast<std::size_t>(count));
		m_rest = m_rest.subspan(static_cast<std::size_t>(count));
		return taken;
	}

	std::string_view ByteCursor::TakeText(std::uint64_t count)
	{
		const std::span<const std::byte> bytes = Take(count);
		return std::string_view(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	}

	std::size_t ByteCursor::Remaining() const noexcept
	{
		return m_rest.size();
	}
}

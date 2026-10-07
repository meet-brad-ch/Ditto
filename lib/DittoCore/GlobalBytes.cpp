/**
 * @file GlobalBytes.cpp
 * @brief Implements DittoCore::GlobalBytes.
 */
#include "GlobalBytes.h"
#include "ClipboardFormatError.h"

#include <string>

namespace DittoCore
{
	HGLOBAL GlobalBytes::NotNull(HGLOBAL block)
	{
		if (block == nullptr)
		{
			throw ClipboardFormatError("global memory block is null");
		}
		return block;
	}

	GlobalBytes::GlobalBytes(HGLOBAL block)
		: m_lock(NotNull(block))
	{
		void* data = m_lock.MutableData();
		if (data == nullptr)
		{
			throw ClipboardFormatError("global memory block cannot be locked (error " + std::to_string(::GetLastError()) + ")");
		}
		const SIZE_T size = ::GlobalSize(block);
		if (size == 0)
		{
			throw ClipboardFormatError("global memory block is empty");
		}
		m_bytes = std::span<std::byte>(static_cast<std::byte*>(data), size);
	}

	std::span<const std::byte> GlobalBytes::Bytes() const noexcept
	{
		return m_bytes;
	}

	std::span<std::byte> GlobalBytes::WritableBytes() noexcept
	{
		return m_bytes;
	}
}

/**
 * @file GlobalBytes.h
 * @brief Declares DittoCore::GlobalBytes.
 */
#pragma once

#include "GlobalLockGuard.h"

#include <cstddef>
#include <span>

#include <windows.h>

namespace DittoCore
{
	/**
	 * @brief The bytes of a global memory block, locked for one scope (RAII).
	 *
	 * Clipboard and database blocks reach Ditto as HGLOBALs. Every read and write goes through a
	 * span bounded by GlobalSize, so code that parses or copies them cannot reach past the block.
	 */
	class GlobalBytes
	{
	public:
		/**
		 * @brief Locks the block and measures it.
		 * @param block The global memory handle.
		 * @throws ClipboardFormatError When @p block is null, cannot be locked or is empty.
		 */
		explicit GlobalBytes(HGLOBAL block);

		GlobalBytes(const GlobalBytes&) = delete;
		GlobalBytes& operator=(const GlobalBytes&) = delete;

		/**
		 * @brief The block's bytes, read-only.
		 * @return All GlobalSize bytes of the block.
		 */
		std::span<const std::byte> Bytes() const noexcept;

		/**
		 * @brief The block's bytes, writable.
		 * @return All GlobalSize bytes of the block.
		 */
		std::span<std::byte> WritableBytes() noexcept;

	private:
		/**
		 * @brief Rejects a null handle before it is locked (GlobalLock must never see null).
		 * @param block The handle to check.
		 * @return @p block.
		 * @throws ClipboardFormatError When @p block is null.
		 */
		static HGLOBAL NotNull(HGLOBAL block);

		/// Holds the lock for the lifetime of this object.
		GlobalLockGuard m_lock;
		/// The locked bytes.
		std::span<std::byte> m_bytes{};
	};
}

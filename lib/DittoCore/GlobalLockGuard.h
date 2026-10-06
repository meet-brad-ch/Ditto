/**
 * @file GlobalLockGuard.h
 * @brief Declares DittoCore::GlobalLockGuard.
 */
#pragma once

#include <windows.h>

namespace DittoCore
{
	/**
	 * @brief Holds GlobalLock on a movable global memory block for one scope (RAII).
	 *
	 * The block is unlocked on every exit path, exceptions included.
	 */
	class GlobalLockGuard
	{
	public:
		/**
		 * @brief Locks the block.
		 * @param block The global memory handle to lock.
		 */
		explicit GlobalLockGuard(HGLOBAL block);

		/// @brief Unlocks the block if the lock succeeded.
		~GlobalLockGuard();

		GlobalLockGuard(const GlobalLockGuard&) = delete;
		GlobalLockGuard& operator=(const GlobalLockGuard&) = delete;

		/**
		 * @brief The locked memory.
		 * @return The start of the block, or null when GlobalLock failed.
		 */
		const void* Data() const { return m_data; }

	private:
		/// The locked handle.
		HGLOBAL m_block{};
		/// The pointer GlobalLock returned.
		void* m_data{};
	};
}

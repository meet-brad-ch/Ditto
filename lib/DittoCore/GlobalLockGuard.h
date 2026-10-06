#pragma once

#include <windows.h>

namespace DittoCore
{
	// Holds GlobalLock on a movable global memory block for one scope and unlocks it on every
	// exit path, exceptions included. Data() is null when the lock failed.
	class GlobalLockGuard
	{
	public:
		explicit GlobalLockGuard(HGLOBAL block);
		~GlobalLockGuard();

		GlobalLockGuard(const GlobalLockGuard&) = delete;
		GlobalLockGuard& operator=(const GlobalLockGuard&) = delete;

		const void* Data() const { return m_data; }

	private:
		HGLOBAL m_block;
		void* m_data;
	};
}

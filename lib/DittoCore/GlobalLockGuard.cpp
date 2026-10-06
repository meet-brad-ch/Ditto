#include "GlobalLockGuard.h"

namespace DittoCore
{
	GlobalLockGuard::GlobalLockGuard(HGLOBAL block)
		: m_block(block), m_data(::GlobalLock(block))
	{
	}

	GlobalLockGuard::~GlobalLockGuard()
	{
		if (m_data != nullptr)
		{
			::GlobalUnlock(m_block);
		}
	}
}

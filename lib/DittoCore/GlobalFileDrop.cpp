/**
 * @file GlobalFileDrop.cpp
 * @brief Implements DittoCore::GlobalFileDrop.
 */
#include "GlobalFileDrop.h"
#include "ClipboardFormatError.h"
#include "GlobalLockGuard.h"

#include <string>

namespace DittoCore
{
	FileDropList GlobalFileDrop::Read(HGLOBAL block)
	{
		if (block == nullptr)
		{
			throw ClipboardFormatError("CF_HDROP global memory handle is null");
		}
		GlobalLockGuard lock(block);
		if (lock.Data() == nullptr)
		{
			throw ClipboardFormatError("CF_HDROP global memory block cannot be locked, GetLastError " +
				std::to_string(::GetLastError()));
		}
		return FileDropList::Parse(lock.Data(), ::GlobalSize(block));
	}
}

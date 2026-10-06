/**
 * @file GlobalFileDrop.h
 * @brief Declares DittoCore::GlobalFileDrop.
 */
#pragma once

#include "FileDropList.h"

#include <windows.h>

namespace DittoCore
{
	/**
	 * @brief Reads a CF_HDROP held in movable global memory (clipboard or OLE data).
	 *
	 * The block stays locked only for the duration of the read, and exactly GlobalSize()
	 * bytes are parsed.
	 */
	class GlobalFileDrop
	{
	public:
		/**
		 * @brief Reads the file list of a global memory block.
		 * @param block The global memory handle holding the CF_HDROP.
		 * @return The paths the block lists.
		 * @throws ClipboardFormatError When the handle is null, cannot be locked, or holds a
		 *         malformed CF_HDROP (see FileDropList::Parse).
		 */
		static FileDropList Read(HGLOBAL block);
	};
}

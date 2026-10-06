#pragma once

#include "FileDropList.h"

#include <windows.h>

namespace DittoCore
{
	// Reads a CF_HDROP held in movable global memory (clipboard or OLE data): the block stays
	// locked only for the duration of the read, and exactly GlobalSize() bytes are parsed.
	class GlobalFileDrop
	{
	public:
		// Throws ClipboardFormatError when the handle is null, cannot be locked, or holds a
		// malformed CF_HDROP (see FileDropList::Parse).
		static FileDropList Read(HGLOBAL block);
	};
}

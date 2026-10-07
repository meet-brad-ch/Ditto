#pragma once

#include "..\..\Shared\DittoDefines.h"
#include "..\..\Shared\IClip.h"

#include "GlobalBytes.h"

#include <cstddef>

class CRemoveLineFeeds
{
public:
	CRemoveLineFeeds(void);
	~CRemoveLineFeeds(void);

	bool RemoveLineFeeds(const CDittoInfo &DittoInfo, IClip *pClip);


private:
	bool Handle_CF_TEXT(IClipFormats *pFormats);
	bool Handle_CF_UNICODETEXT(IClipFormats *pFormats);
	bool Handle_RichText(IClipFormats *pFormats);

	// Writes text and a zero terminator of terminatorBytes back into its block; throws
	// DittoCore::ClipboardFormatError when they do not fit
	static void WriteBack(DittoCore::GlobalBytes& bytes, const void* text, std::size_t textBytes, std::size_t terminatorBytes);
};


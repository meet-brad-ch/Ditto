#pragma once
#include "..\..\Shared\DittoDefines.h"
#include "..\..\Shared\IClip.h"

#include <cstddef>
#include <span>


class PasteAnyAsText
{
public:
	PasteAnyAsText(void);
	~PasteAnyAsText(void);

	static bool SelectClipToPasteAsText(const CDittoInfo& DittoInfo, IClip* pClip);

private:
	// The bytes of a format as text of Char: trailing nulls (the terminator and any padding) are
	// dropped, other nulls become spaces, and one terminator is added
	template <typename Char>
	static HGLOBAL TextBlock(std::span<const std::byte> bytes);
};

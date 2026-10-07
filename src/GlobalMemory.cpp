#include "stdafx.h"
#include "GlobalMemory.h"
#include "GlobalBytes.h"
#include "ClipboardFormatError.h"
#include <new>
#include <string>

// make sure the given HGLOBAL is valid.
BOOL CGlobalMemory::IsValid(HGLOBAL hGlobal)
{
	void* pvData{ ::GlobalLock(hGlobal) };
	::GlobalUnlock(hGlobal);
	return (pvData != NULL);
}

// Copies ulBufLen bytes into hDest; throws when hDest is not a lockable block of at least that size
void CGlobalMemory::CopyToGlobalHP(HGLOBAL hDest, const void* pBuf, SIZE_T ulBufLen)
{
	DittoCore::GlobalBytes dest(hDest);
	if (pBuf == nullptr || ulBufLen > dest.WritableBytes().size())
	{
		throw DittoCore::ClipboardFormatError("copy of " + std::to_string(ulBufLen) + " bytes does not fit a block of " + std::to_string(dest.WritableBytes().size()));
	}
	memcpy(dest.WritableBytes().data(), pBuf, ulBufLen);
}

void CGlobalMemory::CopyToGlobalHH(HGLOBAL hDest, HGLOBAL hSource, SIZE_T ulBufLen)
{
	const DittoCore::GlobalBytes source(hSource);
	if (ulBufLen > source.Bytes().size())
	{
		throw DittoCore::ClipboardFormatError("copy of " + std::to_string(ulBufLen) + " bytes reads past a block of " + std::to_string(source.Bytes().size()));
	}
	CopyToGlobalHP(hDest, source.Bytes().data(), ulBufLen);
}

HGLOBAL CGlobalMemory::NewGlobalP(const void* pBuf, SIZE_T nLen)
{
	HGLOBAL hDest{ GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, nLen) };
	if (hDest == nullptr)
	{
		throw std::bad_alloc();
	}
	CopyToGlobalHP(hDest, pBuf, nLen);
	return hDest;
}

HGLOBAL CGlobalMemory::NewGlobal(SIZE_T nLen)
{
	ASSERT(nLen);
	HGLOBAL hDest{ GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, nLen) };
	return hDest;
}

HGLOBAL CGlobalMemory::NewGlobalH(HGLOBAL hSource, SIZE_T nLen)
{
	const DittoCore::GlobalBytes source(hSource);
	if (nLen > source.Bytes().size())
	{
		throw DittoCore::ClipboardFormatError("copy of " + std::to_string(nLen) + " bytes reads past a block of " + std::to_string(source.Bytes().size()));
	}
	return NewGlobalP(source.Bytes().data(), nLen);
}

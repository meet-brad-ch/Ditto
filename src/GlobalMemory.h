#pragma once

/** @brief Helpers for movable global memory blocks (HGLOBAL), the clipboard's data blocks. */
class CGlobalMemory
{
public:
	/**
	 * @brief Whether a global memory block can be locked.
	 * @param hGlobal The block.
	 * @return TRUE when GlobalLock gives a pointer.
	 */
	static BOOL IsValid(HGLOBAL hGlobal);

	/**
	 * @brief Copies bytes from a buffer into a block.
	 * @param hDest The block; at least ulBufLen bytes.
	 * @param pBuf The bytes.
	 * @param ulBufLen The number of bytes.
	 * @throws DittoCore::ClipboardFormatError when pBuf is null or the bytes do not fit the block.
	 */
	static void CopyToGlobalHP(HGLOBAL hDest, const void* pBuf, SIZE_T ulBufLen);

	/**
	 * @brief Copies bytes from one block into another.
	 * @param hDest The target block; at least ulBufLen bytes.
	 * @param hSource The source block; at least ulBufLen bytes.
	 * @param ulBufLen The number of bytes.
	 * @throws DittoCore::ClipboardFormatError when a block is too small.
	 */
	static void CopyToGlobalHH(HGLOBAL hDest, HGLOBAL hSource, SIZE_T ulBufLen);

	/**
	 * @brief Allocates a movable, shared block holding a copy of a buffer.
	 * @param pBuf The bytes.
	 * @param nLen The number of bytes.
	 * @return The new block; the caller owns it.
	 * @throws std::bad_alloc when GlobalAlloc fails.
	 * @throws DittoCore::ClipboardFormatError when pBuf is null.
	 */
	static HGLOBAL NewGlobalP(const void* pBuf, SIZE_T nLen);

	/**
	 * @brief Allocates a movable, shared block holding a copy of the first bytes of another block.
	 * @param hSource The source block; at least nLen bytes.
	 * @param nLen The number of bytes.
	 * @return The new block; the caller owns it.
	 * @throws std::bad_alloc when GlobalAlloc fails.
	 * @throws DittoCore::ClipboardFormatError when the source block is too small.
	 */
	static HGLOBAL NewGlobalH(HGLOBAL hSource, SIZE_T nLen);

	/**
	 * @brief Allocates a movable, shared block.
	 * @param nLen The number of bytes; not 0.
	 * @return The new block (the caller owns it), or NULL when GlobalAlloc fails.
	 */
	static HGLOBAL NewGlobal(SIZE_T nLen);
};

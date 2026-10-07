/**
 * @file GlobalBytesTests.cpp
 * @brief Unit tests for DittoCore::GlobalBytes.
 */
#include "GlobalBytes.h"
#include "ClipboardFormatError.h"

#include <gtest/gtest.h>

#include <cstring>
#include <memory>
#include <stdexcept>
#include <type_traits>

using DittoCore::ClipboardFormatError;
using DittoCore::GlobalBytes;

namespace
{
	struct GlobalFreeDeleter
	{
		void operator()(void* block) const { ::GlobalFree(block); }
	};
	using OwnedGlobal = std::unique_ptr<std::remove_pointer_t<HGLOBAL>, GlobalFreeDeleter>;

	OwnedGlobal MakeGlobal(const char* text, std::size_t size)
	{
		OwnedGlobal block(::GlobalAlloc(GMEM_MOVEABLE, size));
		void* data = ::GlobalLock(block.get());
		if (data == nullptr)
		{
			throw std::runtime_error("test setup: GlobalLock failed");
		}
		std::memcpy(data, text, size);
		::GlobalUnlock(block.get());
		return block;
	}

	UINT LockCount(HGLOBAL block)
	{
		return ::GlobalFlags(block) & GMEM_LOCKCOUNT;
	}
}

TEST(GlobalBytes, ExposesEveryByteOfTheBlock)
{
	const OwnedGlobal block = MakeGlobal("abc", 4);

	const GlobalBytes bytes(block.get());

	ASSERT_EQ(bytes.Bytes().size(), 4u);
	EXPECT_EQ(std::memcmp(bytes.Bytes().data(), "abc", 4), 0);
}

TEST(GlobalBytes, WritesThroughWritableBytes)
{
	const OwnedGlobal block = MakeGlobal("abc", 4);
	{
		GlobalBytes bytes(block.get());
		bytes.WritableBytes()[0] = std::byte{ 'x' };
	}

	const GlobalBytes check(block.get());
	EXPECT_EQ(std::memcmp(check.Bytes().data(), "xbc", 4), 0);
}

TEST(GlobalBytes, UnlocksTheBlockAtScopeExit)
{
	const OwnedGlobal block = MakeGlobal("abc", 4);
	{
		const GlobalBytes bytes(block.get());
		EXPECT_EQ(LockCount(block.get()), 1u);
	}
	EXPECT_EQ(LockCount(block.get()), 0u);
}

TEST(GlobalBytes, NullHandleThrows)
{
	EXPECT_THROW(GlobalBytes(nullptr), ClipboardFormatError);
}

TEST(GlobalBytes, DiscardedEmptyBlockThrows)
{
	// a movable block of size 0 is allocated as discarded: it cannot be locked
	const OwnedGlobal block(::GlobalAlloc(GMEM_MOVEABLE, 0));
	ASSERT_NE(block.get(), nullptr);

	EXPECT_THROW(GlobalBytes(block.get()), ClipboardFormatError);
}

/**
 * @file GlobalFileDropTests.cpp
 * @brief Unit tests for DittoCore::GlobalFileDrop and its lock handling.
 */
#include "GlobalFileDrop.h"
#include "ClipboardFormatError.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

using DittoCore::ClipboardFormatError;
using DittoCore::FileDropList;
using DittoCore::GlobalFileDrop;

namespace
{
	// Owns a movable global memory block for one test; freed by GlobalFree.
	class OwnedGlobal
	{
	public:
		explicit OwnedGlobal(const std::vector<std::uint8_t>& bytes)
			: m_block(::GlobalAlloc(GMEM_MOVEABLE, bytes.size()), &::GlobalFree)
		{
			if (!m_block)
			{
				throw std::runtime_error("GlobalAlloc failed in test setup");
			}
			void* target{ ::GlobalLock(m_block.get()) };
			if (target == nullptr)
			{
				throw std::runtime_error("GlobalLock failed in test setup");
			}
			std::memcpy(target, bytes.data(), bytes.size());
			::GlobalUnlock(m_block.get());
		}

		HGLOBAL Get() const { return m_block.get(); }

		bool IsUnlocked() const { return (::GlobalFlags(m_block.get()) & GMEM_LOCKCOUNT) == 0; }

	private:
		std::unique_ptr<std::remove_pointer_t<HGLOBAL>, decltype(&::GlobalFree)> m_block;
	};

	std::vector<std::uint8_t> WideBlock(const std::wstring& path)
	{
		std::vector<std::uint8_t> block(20, 0);
		const std::uint32_t filesOffset = 20;
		const std::uint32_t wide = 1;
		std::memcpy(block.data(), &filesOffset, sizeof(filesOffset));
		std::memcpy(block.data() + 16, &wide, sizeof(wide));
		std::wstring list = path;
		list.push_back(L'\0');
		list.push_back(L'\0');
		const auto* bytes = reinterpret_cast<const std::uint8_t*>(list.data());
		block.insert(block.end(), bytes, bytes + list.size() * sizeof(wchar_t));
		return block;
	}
}

TEST(GlobalFileDrop, ReadsLongPathFromGlobalMemoryAndUnlocks)
{
	const std::wstring longPath = L"C:\\" + std::wstring(500, L'g') + L".txt";
	OwnedGlobal block(WideBlock(longPath));

	FileDropList list = GlobalFileDrop::Read(block.Get());

	ASSERT_EQ(list.Paths().size(), 1u);
	EXPECT_EQ(list.Paths()[0], longPath);
	EXPECT_TRUE(block.IsUnlocked());
}

TEST(GlobalFileDrop, UnlocksWhenDataIsMalformed)
{
	OwnedGlobal block(std::vector<std::uint8_t>(10, 0));  // shorter than the DROPFILES header

	EXPECT_THROW(GlobalFileDrop::Read(block.Get()), ClipboardFormatError);
	EXPECT_TRUE(block.IsUnlocked());
}

TEST(GlobalFileDrop, RejectsNullHandle)
{
	EXPECT_THROW(GlobalFileDrop::Read(nullptr), ClipboardFormatError);
}

/**
 * @file FileDropListTests.cpp
 * @brief Unit tests for DittoCore::FileDropList, including the over-MAX_PATH regression.
 */
#include "FileDropList.h"
#include "ClipboardFormatError.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

using DittoCore::ClipboardFormatError;
using DittoCore::FileDropList;

namespace
{
	// Builds CF_HDROP blocks byte by byte: a 20-byte DROPFILES header
	// (pFiles, pt.x, pt.y, fNC, fWide as 32-bit values) and a path list.
	class DropBlockBuilder
	{
	public:
		static constexpr std::uint32_t HeaderSize = 20;

		static std::vector<std::uint8_t> Wide(const std::vector<std::wstring>& paths)
		{
			std::vector<std::uint8_t> block = Header(HeaderSize, true);
			for (const std::wstring& path : paths)
			{
				AppendWide(block, path);
			}
			AppendWide(block, L"");
			return block;
		}

		static std::vector<std::uint8_t> Ansi(const std::vector<std::string>& paths)
		{
			std::vector<std::uint8_t> block = Header(HeaderSize, false);
			for (const std::string& path : paths)
			{
				block.insert(block.end(), path.begin(), path.end());
				block.push_back(0);
			}
			block.push_back(0);
			return block;
		}

		static std::vector<std::uint8_t> Header(std::uint32_t filesOffset, bool wide)
		{
			std::vector<std::uint8_t> block;
			AppendUint32(block, filesOffset);
			AppendUint32(block, 0);
			AppendUint32(block, 0);
			AppendUint32(block, 0);
			AppendUint32(block, wide ? 1u : 0u);
			return block;
		}

		static void AppendWide(std::vector<std::uint8_t>& block, const std::wstring& text)
		{
			for (wchar_t c : text)
			{
				AppendWchar(block, c);
			}
			AppendWchar(block, L'\0');
		}

	private:
		static void AppendUint32(std::vector<std::uint8_t>& block, std::uint32_t value)
		{
			std::uint8_t bytes[sizeof(value)];
			std::memcpy(bytes, &value, sizeof(value));
			block.insert(block.end(), bytes, bytes + sizeof(value));
		}

		static void AppendWchar(std::vector<std::uint8_t>& block, wchar_t value)
		{
			std::uint8_t bytes[sizeof(value)];
			std::memcpy(bytes, &value, sizeof(value));
			block.insert(block.end(), bytes, bytes + sizeof(value));
		}
	};

	FileDropList ParseBlock(const std::vector<std::uint8_t>& block)
	{
		return FileDropList::Parse(block.data(), block.size());
	}
}

TEST(FileDropList, ReadsWidePaths)
{
	FileDropList list = ParseBlock(DropBlockBuilder::Wide({ L"C:\\a.txt", L"D:\\dir\\b.png" }));

	ASSERT_EQ(list.Paths().size(), 2u);
	EXPECT_EQ(list.Paths()[0], L"C:\\a.txt");
	EXPECT_EQ(list.Paths()[1], L"D:\\dir\\b.png");
}

// Regression: Ditto read paths into TCHAR[MAX_PATH] while passing sizeof() (bytes) as the
// character count, so a path over 260 characters overflowed the stack buffer.
TEST(FileDropList, ReadsPathLongerThanMaxPath)
{
	const std::wstring longPath = L"\\\\?\\C:\\" + std::wstring(400, L'x') + L"\\file.txt";

	FileDropList list = ParseBlock(DropBlockBuilder::Wide({ longPath }));

	ASSERT_EQ(list.Paths().size(), 1u);
	EXPECT_EQ(list.Paths()[0], longPath);
}

TEST(FileDropList, ReadsPathOfWindowsMaximumLength)
{
	const std::wstring longestPath(32767, L'p');

	FileDropList list = ParseBlock(DropBlockBuilder::Wide({ longestPath }));

	ASSERT_EQ(list.Paths().size(), 1u);
	EXPECT_EQ(list.Paths()[0].size(), 32767u);
}

// Range-for over the paths of a temporary list: the call sites in Ditto use this form. With a
// reference-returning Paths() the list would be destroyed before the loop (ASan: use after free).
TEST(FileDropList, PathsOfTemporaryListStayValidInRangeFor)
{
	const std::vector<std::uint8_t> block = DropBlockBuilder::Wide({ L"C:\\first.txt", L"C:\\second.txt" });
	std::vector<std::wstring> seen;

	for (const std::wstring& path : FileDropList::Parse(block.data(), block.size()).Paths())
	{
		seen.push_back(path);
	}

	ASSERT_EQ(seen.size(), 2u);
	EXPECT_EQ(seen[1], L"C:\\second.txt");
}

TEST(FileDropList, ReadsAnsiPaths)
{
	FileDropList list = ParseBlock(DropBlockBuilder::Ansi({ "C:\\ansi.txt", "C:\\second.doc" }));

	ASSERT_EQ(list.Paths().size(), 2u);
	EXPECT_EQ(list.Paths()[0], L"C:\\ansi.txt");
	EXPECT_EQ(list.Paths()[1], L"C:\\second.doc");
}

TEST(FileDropList, EmptyListGivesNoPaths)
{
	FileDropList list = ParseBlock(DropBlockBuilder::Wide({}));

	EXPECT_TRUE(list.Paths().empty());
}

TEST(FileDropList, IgnoresBytesAfterTheTerminator)
{
	std::vector<std::uint8_t> block = DropBlockBuilder::Wide({ L"C:\\a.txt" });
	block.insert(block.end(), 16, 0xCD); // GlobalSize rounds allocations up

	FileDropList list = ParseBlock(block);

	ASSERT_EQ(list.Paths().size(), 1u);
	EXPECT_EQ(list.Paths()[0], L"C:\\a.txt");
}

TEST(FileDropList, RejectsNullData)
{
	EXPECT_THROW(FileDropList::Parse(nullptr, 40), ClipboardFormatError);
}

TEST(FileDropList, RejectsBlockShorterThanHeader)
{
	std::vector<std::uint8_t> block = DropBlockBuilder::Header(DropBlockBuilder::HeaderSize, true);
	block.resize(DropBlockBuilder::HeaderSize - 1);

	EXPECT_THROW(ParseBlock(block), ClipboardFormatError);
}

TEST(FileDropList, RejectsPathListStartingInsideHeader)
{
	std::vector<std::uint8_t> block = DropBlockBuilder::Header(4, true);
	DropBlockBuilder::AppendWide(block, L"C:\\a.txt");
	DropBlockBuilder::AppendWide(block, L"");

	EXPECT_THROW(ParseBlock(block), ClipboardFormatError);
}

TEST(FileDropList, RejectsPathListStartingOutsideBlock)
{
	std::vector<std::uint8_t> block = DropBlockBuilder::Header(1000, true);
	DropBlockBuilder::AppendWide(block, L"");

	EXPECT_THROW(ParseBlock(block), ClipboardFormatError);
}

TEST(FileDropList, RejectsWideListWithoutFinalTerminator)
{
	std::vector<std::uint8_t> block = DropBlockBuilder::Header(DropBlockBuilder::HeaderSize, true);
	DropBlockBuilder::AppendWide(block, L"C:\\a.txt"); // path terminator, but no empty final entry

	EXPECT_THROW(ParseBlock(block), ClipboardFormatError);
}

TEST(FileDropList, RejectsWidePathWithoutTerminator)
{
	std::vector<std::uint8_t> block = DropBlockBuilder::Header(DropBlockBuilder::HeaderSize, true);
	for (wchar_t c : std::wstring(L"C:\\unterminated"))
	{
		block.push_back(static_cast<std::uint8_t>(c & 0xFF));
		block.push_back(static_cast<std::uint8_t>((c >> 8) & 0xFF));
	}

	EXPECT_THROW(ParseBlock(block), ClipboardFormatError);
}

TEST(FileDropList, RejectsAnsiListWithoutFinalTerminator)
{
	std::vector<std::uint8_t> block = DropBlockBuilder::Header(DropBlockBuilder::HeaderSize, false);
	const std::string path = "C:\\a.txt";
	block.insert(block.end(), path.begin(), path.end());
	block.push_back(0);

	EXPECT_THROW(ParseBlock(block), ClipboardFormatError);
}

TEST(FileDropList, RejectsWideListWithOddTrailingByte)
{
	std::vector<std::uint8_t> block = DropBlockBuilder::Header(DropBlockBuilder::HeaderSize, true);
	block.push_back(0); // half a wide character: no complete terminator fits

	EXPECT_THROW(ParseBlock(block), ClipboardFormatError);
}

TEST(FileDropList, BuildMatchesTheHandMadeBlock)
{
	const std::vector<std::wstring> paths{ L"C:\\a.txt", L"C:\\long folder\\b.bin" };

	const std::vector<std::byte> built = FileDropList::Build(paths);
	const std::vector<std::uint8_t> expected = DropBlockBuilder::Wide(paths);

	ASSERT_EQ(built.size(), expected.size());
	EXPECT_EQ(std::memcmp(built.data(), expected.data(), built.size()), 0);
}

TEST(FileDropList, BuiltBlockParsesBackToItsPaths)
{
	const std::vector<std::wstring> paths{ L"C:\\a.txt", std::wstring(400, L'x') };
	const std::vector<std::byte> block = FileDropList::Build(paths);

	EXPECT_EQ(FileDropList::Parse(block.data(), block.size()).Paths(), paths);
}

TEST(FileDropList, BuildsEmptyList)
{
	const std::vector<std::byte> block = FileDropList::Build({});

	EXPECT_TRUE(FileDropList::Parse(block.data(), block.size()).Paths().empty());
}

TEST(FileDropList, BuildRejectsEmptyPath)
{
	const std::vector<std::wstring> paths{ L"C:\\a.txt", L"" };

	EXPECT_THROW(FileDropList::Build(paths), ClipboardFormatError);
}

TEST(FileDropList, BuildRejectsPathWithNull)
{
	const std::vector<std::wstring> paths{ std::wstring(L"C:\\a\0b", 6) };

	EXPECT_THROW(FileDropList::Build(paths), ClipboardFormatError);
}

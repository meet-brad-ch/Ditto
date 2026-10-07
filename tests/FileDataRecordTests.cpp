/**
 * @file FileDataRecordTests.cpp
 * @brief Unit tests for DittoCore::FileDataRecord and DittoCore::ByteCursor.
 */
#include "ByteCursor.h"
#include "ClipboardFormatError.h"
#include "FileDataRecord.h"

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

using DittoCore::ByteCursor;
using DittoCore::ClipboardFormatError;
using DittoCore::FileDataEntry;
using DittoCore::FileDataRecord;

namespace
{
	const std::string Md5A(32, 'a');
	const std::string Md5B(32, 'b');

	std::vector<std::byte> Bytes(const std::string& text)
	{
		std::vector<std::byte> bytes(text.size());
		std::memcpy(bytes.data(), text.data(), text.size());
		return bytes;
	}

	std::string Text(std::span<const std::byte> bytes)
	{
		return std::string(reinterpret_cast<const char*>(bytes.data()), bytes.size());
	}

	// upstream's layout: path \0 md5 \0 contents
	std::vector<std::byte> Version1(const std::string& path, const std::string& md5, const std::string& contents)
	{
		return Bytes(path + std::string(1, '\0') + md5 + std::string(1, '\0') + contents);
	}
}

TEST(FileDataRecord, ReadsUpstreamVersion1Block)
{
	const std::vector<std::byte> block = Version1("C:\\a.txt", Md5A, "hello");

	const std::vector<FileDataEntry> files = FileDataRecord::Parse(block);

	ASSERT_EQ(files.size(), 1u);
	EXPECT_EQ(files[0].path, "C:\\a.txt");
	EXPECT_EQ(files[0].md5, Md5A);
	EXPECT_EQ(Text(files[0].data), "hello");
}

TEST(FileDataRecord, ReadsVersion1BlockWithEmptyContents)
{
	const std::vector<FileDataEntry> files = FileDataRecord::Parse(Version1("C:\\empty.txt", Md5A, ""));

	ASSERT_EQ(files.size(), 1u);
	EXPECT_TRUE(files[0].data.empty());
}

TEST(FileDataRecord, RejectsVersion1WithoutPathTerminator)
{
	EXPECT_THROW(FileDataRecord::Parse(Bytes("C:\\a.txt")), ClipboardFormatError);
}

TEST(FileDataRecord, RejectsVersion1WithoutMd5Terminator)
{
	EXPECT_THROW(FileDataRecord::Parse(Bytes(std::string("C:\\a.txt") + '\0' + Md5A)), ClipboardFormatError);
}

TEST(FileDataRecord, RejectsVersion1WithShortMd5)
{
	EXPECT_THROW(FileDataRecord::Parse(Version1("C:\\a.txt", "abc", "x")), ClipboardFormatError);
}

TEST(FileDataRecord, RejectsEmptyPath)
{
	EXPECT_THROW(FileDataRecord::Parse(Version1("", Md5A, "x")), ClipboardFormatError);
}

// Regression: upstream kept one format per type, so copying several files kept only the last
// file's contents. Version 2 keeps every file in one block.
TEST(FileDataRecord, KeepsEveryFileOfAVersion2Block)
{
	const std::vector<std::byte> a = Bytes("first");
	const std::vector<std::byte> b = Bytes(std::string("sec\0nd", 6));
	const std::vector<FileDataEntry> files{ { "C:\\a.txt", Md5A, a }, { "C:\\\xC3\xA9.bin", Md5B, b } };

	const std::vector<std::byte> block = FileDataRecord::Build(files);
	const std::vector<FileDataEntry> parsed = FileDataRecord::Parse(block);

	ASSERT_EQ(parsed.size(), 2u);
	EXPECT_EQ(parsed[0].path, "C:\\a.txt");
	EXPECT_EQ(Text(parsed[0].data), "first");
	EXPECT_EQ(parsed[1].path, "C:\\\xC3\xA9.bin");
	EXPECT_EQ(parsed[1].md5, Md5B);
	EXPECT_EQ(Text(parsed[1].data), std::string("sec\0nd", 6));
}

TEST(FileDataRecord, BuildsEmptyList)
{
	EXPECT_TRUE(FileDataRecord::Parse(FileDataRecord::Build({})).empty());
}

TEST(FileDataRecord, BuildRejectsWrongMd5Length)
{
	const std::vector<FileDataEntry> files{ { "C:\\a.txt", "abc", {} } };

	EXPECT_THROW(FileDataRecord::Build(files), ClipboardFormatError);
}

TEST(FileDataRecord, RejectsVersion2CountLargerThanBlock)
{
	std::vector<std::byte> block = FileDataRecord::Build({});
	block[4] = std::byte{ 0xFF };   // file count 255, no files follow

	EXPECT_THROW(FileDataRecord::Parse(block), ClipboardFormatError);
}

TEST(FileDataRecord, RejectsVersion2ContentLengthPastBlock)
{
	const std::vector<std::byte> data = Bytes("abc");
	const std::vector<FileDataEntry> files{ { "C:\\a.txt", Md5A, data } };
	std::vector<std::byte> block = FileDataRecord::Build(files);
	block.pop_back();

	EXPECT_THROW(FileDataRecord::Parse(block), ClipboardFormatError);
}

TEST(FileDataRecord, RejectsBytesAfterLastFile)
{
	std::vector<std::byte> block = FileDataRecord::Build({});
	block.push_back(std::byte{ 0 });

	EXPECT_THROW(FileDataRecord::Parse(block), ClipboardFormatError);
}

TEST(ByteCursor, ReadsLittleEndianValues)
{
	const std::vector<std::byte> block{ std::byte{ 0x01 }, std::byte{ 0x02 }, std::byte{ 0x03 }, std::byte{ 0x04 },
		std::byte{ 0x05 }, std::byte{ 0 }, std::byte{ 0 }, std::byte{ 0 }, std::byte{ 0x01 }, std::byte{ 0 }, std::byte{ 0 }, std::byte{ 0 } };
	ByteCursor cursor(block);

	EXPECT_EQ(cursor.ReadUInt32(), 0x04030201u);
	EXPECT_EQ(cursor.ReadUInt64(), 0x0000000100000005ull);
	EXPECT_EQ(cursor.Remaining(), 0u);
}

TEST(ByteCursor, RejectsReadPastEnd)
{
	const std::vector<std::byte> block(3);
	ByteCursor cursor(block);

	EXPECT_THROW(cursor.ReadUInt32(), ClipboardFormatError);
	EXPECT_THROW(cursor.Take(0xFFFFFFFFFFFFFFFFull), ClipboardFormatError);
	EXPECT_EQ(cursor.Remaining(), 3u);
}

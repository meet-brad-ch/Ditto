/**
 * @file IniSettingsStoreTests.cpp
 * @brief Unit tests for DittoCore::IniSettingsStore. Each test works in its own file in the
 *        temp folder, DittoTests_<process id>_<test name>.ini, and deletes it at the end.
 */
#include "IniSettingsStore.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <windows.h>

using DittoCore::IniSettingsStore;
using DittoCore::ISettingsStore;

namespace
{
	class IniSettingsStoreTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			std::array<wchar_t, MAX_PATH + 1> tempDir{};
			ASSERT_NE(::GetTempPathW(static_cast<DWORD>(tempDir.size()), tempDir.data()), 0u);
			const std::string testName{ ::testing::UnitTest::GetInstance()->current_test_info()->name() };
			m_filePath = std::wstring(tempDir.data()) + L"DittoTests_" + std::to_wstring(::GetCurrentProcessId()) + L"_" + std::wstring(testName.begin(), testName.end()) + L".ini";
			static_cast<void>(::DeleteFileW(m_filePath.c_str()));
		}

		void TearDown() override
		{
			static_cast<void>(::DeleteFileW(m_filePath.c_str()));
		}

		// Creates the file as UTF-16 with a byte order mark, as CGetSetOptions does
		void CreateUnicodeFile() const
		{
			std::ofstream file(m_filePath, std::ios::binary);
			file.put(static_cast<char>(0xFF));
			file.put(static_cast<char>(0xFE));
			ASSERT_TRUE(file.good());
		}

		// The raw text of a value, read with the Windows profile API
		std::wstring RawValue(const wchar_t* section, const wchar_t* name) const
		{
			std::array<wchar_t, 256> buffer{};
			::GetPrivateProfileStringW(section, name, L"<missing>", buffer.data(), static_cast<DWORD>(buffer.size()), m_filePath.c_str());
			return std::wstring(buffer.data());
		}

		std::wstring m_filePath{};
	};
}

TEST_F(IniSettingsStoreTest, MissingFileGivesTheDefaults)
{
	const IniSettingsStore store{ m_filePath };

	EXPECT_EQ(store.GetLong(L"", L"Number", 42), 42);
	EXPECT_EQ(store.GetLong(L"", L"Number", -3), -3);
	EXPECT_EQ(store.GetString(L"", L"Text", L"default", ISettingsStore::NoMaxSize), L"default");
}

TEST_F(IniSettingsStoreTest, MissingValueGivesTheDefaults)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetLong(L"", L"Other", 1));

	EXPECT_EQ(store.GetLong(L"", L"Number", 42), 42);
	EXPECT_EQ(store.GetString(L"", L"Text", L"default", ISettingsStore::NoMaxSize), L"default");
}

TEST_F(IniSettingsStoreTest, LongRoundTripsAsDecimalTextInTheDittoSection)
{
	IniSettingsStore store{ m_filePath };

	ASSERT_TRUE(store.SetLong(L"", L"Positive", 704));
	ASSERT_TRUE(store.SetLong(L"", L"Negative", -5));

	EXPECT_EQ(store.GetLong(L"", L"Positive", 0), 704);
	EXPECT_EQ(store.GetLong(L"", L"Negative", 0), -5);
	EXPECT_EQ(RawValue(L"Ditto", L"Negative"), L"-5");
	EXPECT_EQ(store.GetLong(L"Ditto", L"Positive", 0), 704);
}

TEST_F(IniSettingsStoreTest, SectionIsAnIniSection)
{
	IniSettingsStore store{ m_filePath };

	ASSERT_TRUE(store.SetLong(L"DisplayFont6", L"Height", -13));
	ASSERT_TRUE(store.SetString(L"PasteStrings", L"notepad.exe", L"^v"));

	EXPECT_EQ(RawValue(L"DisplayFont6", L"Height"), L"-13");
	EXPECT_EQ(store.GetLong(L"DisplayFont6", L"Height", 0), -13);
	EXPECT_EQ(store.GetString(L"PasteStrings", L"notepad.exe", L"", ISettingsStore::NoMaxSize), L"^v");
	EXPECT_EQ(store.GetLong(L"", L"Height", 0), 0);
}

TEST_F(IniSettingsStoreTest, NamesAreNotCaseSensitive)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetLong(L"Section", L"Name", 5));

	EXPECT_EQ(store.GetLong(L"SECTION", L"NAME", 0), 5);
}

// Numbers are parsed as GetPrivateProfileInt does (measured on Windows 11, 2026-10-07)
TEST_F(IniSettingsStoreTest, LongIsParsedAsGetPrivateProfileIntDoes)
{
	struct Case
	{
		const wchar_t* text{};
		long expected{};
	};
	const std::array<Case, 12> cases{ {
		{ L"42abc", 42 },
		{ L"abc", 0 },
		{ L"  7", 7 },
		{ L" -8 ", -8 },
		{ L"+3", 3 },
		{ L"10.5", 10 },
		{ L"\"12\"", 12 },
		{ L"0x10", 16 },
		{ L"0X1F", 0 },
		{ L"4294967295", -1 },
		{ L"4294967296", 0 },
		{ L"-", 0 },
	} };
	IniSettingsStore store{ m_filePath };

	for (const Case& c : cases)
	{
		ASSERT_TRUE(store.SetString(L"", L"Number", c.text));
		EXPECT_EQ(store.GetLong(L"", L"Number", 77), c.expected) << "text: " << ::testing::PrintToString(std::wstring(c.text));
	}
}

TEST_F(IniSettingsStoreTest, EmptyNumberGivesTheDefault)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetString(L"", L"Number", L""));

	EXPECT_EQ(store.GetLong(L"", L"Number", 77), 77);
}

TEST_F(IniSettingsStoreTest, StringRoundTripsInAUnicodeFile)
{
	CreateUnicodeFile();
	IniSettingsStore store{ m_filePath };

	ASSERT_TRUE(store.SetString(L"", L"Text", L"C:\\Clips\\Ditto.db \u00e9\u4e2d"));

	EXPECT_EQ(store.GetString(L"", L"Text", L"", ISettingsStore::NoMaxSize), L"C:\\Clips\\Ditto.db \u00e9\u4e2d");
}

// The profile API removes the blanks around a value and one pair of enclosing quotes
TEST_F(IniSettingsStoreTest, StringIsTrimmedAndUnquoted)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetString(L"", L"Quoted", L"\"quoted\""));
	ASSERT_TRUE(store.SetString(L"", L"Blanks", L"  text  "));

	EXPECT_EQ(store.GetString(L"", L"Quoted", L"", ISettingsStore::NoMaxSize), L"quoted");
	EXPECT_EQ(store.GetString(L"", L"Blanks", L"", ISettingsStore::NoMaxSize), L"text");
}

// The first read buffer holds 9999 characters; a longer value is read again with more room
TEST_F(IniSettingsStoreTest, LongStringIsReadWhole)
{
	IniSettingsStore store{ m_filePath };
	const std::wstring text(15000, L'x');
	const std::wstring boundary(9999, L'y');
	ASSERT_TRUE(store.SetString(L"", L"Text", text));
	ASSERT_TRUE(store.SetString(L"", L"Boundary", boundary));

	EXPECT_EQ(store.GetString(L"", L"Text", L"", ISettingsStore::NoMaxSize), text);
	EXPECT_EQ(store.GetString(L"", L"Boundary", L"", ISettingsStore::NoMaxSize), boundary);
}

// maxSize is a buffer of characters: maxSize - 1 of them are read
TEST_F(IniSettingsStoreTest, StringIsCutToMaxSize)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetString(L"", L"Text", L"abcdef"));

	EXPECT_EQ(store.GetString(L"", L"Text", L"", 4), L"abc");
	EXPECT_EQ(store.GetString(L"", L"Text", L"", 7), L"abcdef");
	EXPECT_EQ(store.GetString(L"", L"Text", L"", 1), L"");
	EXPECT_EQ(store.GetString(L"", L"Text", L"", 0), L"");
}

TEST_F(IniSettingsStoreTest, MaxSizeAboveTheFirstBufferCutsAfterGrowing)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetString(L"", L"Text", std::wstring(15000, L'x')));

	EXPECT_EQ(store.GetString(L"", L"Text", L"", 12000), std::wstring(11999, L'x'));
}

TEST_F(IniSettingsStoreTest, DefaultIsCutToMaxSize)
{
	const IniSettingsStore store{ m_filePath };

	EXPECT_EQ(store.GetString(L"", L"Missing", L"abcdef", 3), L"ab");
}

TEST_F(IniSettingsStoreTest, WriteIntoAMissingFolderFails)
{
	IniSettingsStore store{ L"C:\\DittoTestsMissingFolder\\Missing\\Ditto.Settings" };

	EXPECT_FALSE(store.SetLong(L"", L"Number", 1));
	EXPECT_FALSE(store.SetString(L"", L"Text", L"x"));
	EXPECT_THROW(store.DeleteSection(L"Section"), std::runtime_error);
}

TEST_F(IniSettingsStoreTest, DataIsNotSupported)
{
	IniSettingsStore store{ m_filePath };
	const std::array<std::byte, 1> data{ std::byte{ 1 } };

	EXPECT_THROW(store.GetData(L"", L"Data"), std::logic_error);
	EXPECT_THROW(store.SetData(L"", L"Data", data), std::logic_error);
}

TEST_F(IniSettingsStoreTest, DeleteSectionRemovesItsValuesOnly)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetLong(L"", L"Number", 1));
	ASSERT_TRUE(store.SetLong(L"DisplayFont6", L"Height", 2));

	store.DeleteSection(L"DisplayFont6");

	EXPECT_EQ(store.GetLong(L"DisplayFont6", L"Height", -1), -1);
	EXPECT_EQ(store.GetLong(L"", L"Number", -1), 1);
}

TEST_F(IniSettingsStoreTest, DeleteMissingSectionIsNoError)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetLong(L"", L"Number", 1));

	EXPECT_NO_THROW(store.DeleteSection(L"Missing"));
}

TEST_F(IniSettingsStoreTest, DeleteDefaultSectionIsRefused)
{
	IniSettingsStore store{ m_filePath };
	ASSERT_TRUE(store.SetLong(L"", L"Number", 1));

	EXPECT_THROW(store.DeleteSection(L""), std::invalid_argument);
	EXPECT_EQ(store.GetLong(L"", L"Number", -1), 1);
}

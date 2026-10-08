/**
 * @file RegistrySettingsStoreTests.cpp
 * @brief Unit tests for DittoCore::RegistrySettingsStore. Each test works in its own key,
 *        HKCU\\Software\\DittoTests\\<process id>_<test name>, and deletes it at the end.
 */
#include "RegistrySettingsStore.h"

#include <gtest/gtest.h>

#include <array>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <windows.h>

using DittoCore::ISettingsStore;
using DittoCore::RegistrySettingsStore;

namespace
{
	const std::wstring TestsKey{ L"Software\\DittoTests" };

	class RegistrySettingsStoreTest : public ::testing::Test
	{
	protected:
		void SetUp() override
		{
			const std::string testName{ ::testing::UnitTest::GetInstance()->current_test_info()->name() };
			m_rootPath = TestsKey + L"\\" + std::to_wstring(::GetCurrentProcessId()) + L"_" + std::wstring(testName.begin(), testName.end());
			static_cast<void>(::RegDeleteTreeW(HKEY_CURRENT_USER, m_rootPath.c_str()));
		}

		void TearDown() override
		{
			static_cast<void>(::RegDeleteTreeW(HKEY_CURRENT_USER, m_rootPath.c_str()));
			// removes Software\DittoTests only when no other test run uses it
			static_cast<void>(::RegDeleteKeyW(HKEY_CURRENT_USER, TestsKey.c_str()));
		}

		// Writes a raw value of any type under the test key (or its sub key)
		void WriteRaw(const std::wstring& subKey, const std::wstring& name, DWORD type, const void* data, DWORD size) const
		{
			HKEY key{};
			const std::wstring path{ subKey.empty() ? m_rootPath : m_rootPath + L"\\" + subKey };
			ASSERT_EQ(::RegCreateKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, nullptr, 0, KEY_ALL_ACCESS, nullptr, &key, nullptr), ERROR_SUCCESS);
			ASSERT_EQ(::RegSetValueExW(key, name.c_str(), 0, type, static_cast<const BYTE*>(data), size), ERROR_SUCCESS);
			static_cast<void>(::RegCloseKey(key));
		}

		// The type and size of a value under the test key, as stored
		void QueryRaw(const std::wstring& name, DWORD& type, DWORD& size) const
		{
			ASSERT_EQ(::RegGetValueW(HKEY_CURRENT_USER, m_rootPath.c_str(), name.c_str(), RRF_RT_ANY | RRF_NOEXPAND, &type, nullptr, &size), ERROR_SUCCESS);
		}

		bool KeyExists(const std::wstring& path) const
		{
			HKEY key{};
			if (::RegOpenKeyExW(HKEY_CURRENT_USER, path.c_str(), 0, KEY_READ, &key) != ERROR_SUCCESS)
			{
				return false;
			}
			static_cast<void>(::RegCloseKey(key));
			return true;
		}

		std::wstring m_rootPath{};
	};

	std::vector<std::byte> Bytes(std::initializer_list<unsigned char> values)
	{
		std::vector<std::byte> bytes{};
		for (const unsigned char value : values)
		{
			bytes.push_back(static_cast<std::byte>(value));
		}
		return bytes;
	}
}

TEST_F(RegistrySettingsStoreTest, MissingKeyGivesTheDefaults)
{
	const RegistrySettingsStore store{ m_rootPath };

	EXPECT_EQ(store.GetLong(L"", L"Number", 42), 42);
	EXPECT_EQ(store.GetString(L"", L"Text", L"default", ISettingsStore::NoMaxSize), L"default");
	EXPECT_TRUE(store.GetData(L"", L"Data").empty());
	EXPECT_FALSE(KeyExists(m_rootPath));
}

TEST_F(RegistrySettingsStoreTest, MissingValueGivesTheDefaults)
{
	RegistrySettingsStore store{ m_rootPath };
	ASSERT_TRUE(store.SetLong(L"", L"Other", 1));

	EXPECT_EQ(store.GetLong(L"", L"Number", -7), -7);
	EXPECT_EQ(store.GetString(L"", L"Text", L"default", ISettingsStore::NoMaxSize), L"default");
	EXPECT_TRUE(store.GetData(L"", L"Data").empty());
}

TEST_F(RegistrySettingsStoreTest, LongRoundTripsAsDword)
{
	RegistrySettingsStore store{ m_rootPath };

	ASSERT_TRUE(store.SetLong(L"", L"Positive", 704));
	ASSERT_TRUE(store.SetLong(L"", L"Negative", -2));

	EXPECT_EQ(store.GetLong(L"", L"Positive", 0), 704);
	EXPECT_EQ(store.GetLong(L"", L"Negative", 0), -2);
	DWORD type{};
	DWORD size{};
	QueryRaw(L"Negative", type, size);
	EXPECT_EQ(type, static_cast<DWORD>(REG_DWORD));
	EXPECT_EQ(size, sizeof(DWORD));
}

// A value longer than 4 bytes does not fit the read: the default comes back
TEST_F(RegistrySettingsStoreTest, LongOfALongerValueGivesTheDefault)
{
	RegistrySettingsStore store{ m_rootPath };
	ASSERT_TRUE(store.SetString(L"", L"Text", L"abcdef"));

	EXPECT_EQ(store.GetLong(L"", L"Text", 5), 5);
}

TEST_F(RegistrySettingsStoreTest, SectionIsASubKey)
{
	RegistrySettingsStore store{ m_rootPath };

	ASSERT_TRUE(store.SetLong(L"Sub", L"Number", 3));
	ASSERT_TRUE(store.SetString(L"Sub", L"Text", L"in sub"));

	EXPECT_TRUE(KeyExists(m_rootPath + L"\\Sub"));
	EXPECT_EQ(store.GetLong(L"Sub", L"Number", 0), 3);
	EXPECT_EQ(store.GetString(L"Sub", L"Text", L"", ISettingsStore::NoMaxSize), L"in sub");
	EXPECT_EQ(store.GetLong(L"", L"Number", 0), 0);
	EXPECT_EQ(store.GetString(L"", L"Text", L"root", ISettingsStore::NoMaxSize), L"root");
}

// A key name longer than 255 characters cannot be created or opened
TEST_F(RegistrySettingsStoreTest, KeyThatCannotBeCreatedFailsTheWrites)
{
	RegistrySettingsStore store{ m_rootPath };
	const std::wstring tooLong(300, L'a');
	const std::array<std::byte, 1> data{ std::byte{ 1 } };

	EXPECT_FALSE(store.SetLong(tooLong, L"Number", 1));
	EXPECT_FALSE(store.SetString(tooLong, L"Text", L"x"));
	EXPECT_FALSE(store.SetData(tooLong, L"Data", data));
	EXPECT_EQ(store.GetLong(tooLong, L"Number", 9), 9);
}

// The text is stored without a terminating null, as Ditto always wrote it
TEST_F(RegistrySettingsStoreTest, StringRoundTripsAsRegSzWithoutTerminator)
{
	RegistrySettingsStore store{ m_rootPath };

	ASSERT_TRUE(store.SetString(L"", L"Text", L"C:\\Clips\\Ditto.db \u00e9\u4e2d"));

	EXPECT_EQ(store.GetString(L"", L"Text", L"", ISettingsStore::NoMaxSize), L"C:\\Clips\\Ditto.db \u00e9\u4e2d");
	DWORD type{};
	DWORD size{};
	QueryRaw(L"Text", type, size);
	EXPECT_EQ(type, static_cast<DWORD>(REG_SZ));
}

TEST_F(RegistrySettingsStoreTest, EmptyStringValueIsEmptyNotTheDefault)
{
	RegistrySettingsStore store{ m_rootPath };

	ASSERT_TRUE(store.SetString(L"", L"Text", L""));

	EXPECT_EQ(store.GetString(L"", L"Text", L"default", ISettingsStore::NoMaxSize), L"");
}

TEST_F(RegistrySettingsStoreTest, StringStopsAtTheFirstNull)
{
	const wchar_t text[]{ L"ab\0cd" };
	WriteRaw(L"", L"Text", REG_SZ, text, sizeof(text));
	const RegistrySettingsStore store{ m_rootPath };

	EXPECT_EQ(store.GetString(L"", L"Text", L"", ISettingsStore::NoMaxSize), L"ab");
}

// maxSize counts bytes: "abcdef" (12 bytes, no terminator) needs a 14-byte buffer, which
// maxSize 13 gives (maxSize + 1); with less the read fails and the default comes back
TEST_F(RegistrySettingsStoreTest, StringLongerThanMaxSizeGivesTheDefault)
{
	RegistrySettingsStore store{ m_rootPath };
	ASSERT_TRUE(store.SetString(L"", L"Text", L"abcdef"));

	EXPECT_EQ(store.GetString(L"", L"Text", L"default", 13), L"abcdef");
	EXPECT_EQ(store.GetString(L"", L"Text", L"default", 12), L"default");
	EXPECT_EQ(store.GetString(L"", L"Text", L"default", 0), L"default");
	EXPECT_EQ(store.GetString(L"", L"Text", L"default", 1000), L"abcdef");
}

TEST_F(RegistrySettingsStoreTest, DataRoundTripsAsRegBinary)
{
	RegistrySettingsStore store{ m_rootPath };
	const std::vector<std::byte> data{ Bytes({ 0x00, 0xFF, 0x10, 0x80, 0x00 }) };

	ASSERT_TRUE(store.SetData(L"", L"Data", data));

	EXPECT_EQ(store.GetData(L"", L"Data"), data);
	DWORD type{};
	DWORD size{};
	QueryRaw(L"Data", type, size);
	EXPECT_EQ(type, static_cast<DWORD>(REG_BINARY));
	EXPECT_EQ(size, data.size());
}

TEST_F(RegistrySettingsStoreTest, DataOfAnyTypeIsItsBytes)
{
	const DWORD number{ 0x04030201 };
	WriteRaw(L"", L"Number", REG_DWORD, &number, sizeof(number));
	const RegistrySettingsStore store{ m_rootPath };

	EXPECT_EQ(store.GetData(L"", L"Number"), Bytes({ 0x01, 0x02, 0x03, 0x04 }));
}

TEST_F(RegistrySettingsStoreTest, DeleteSectionRemovesTheSubKeyOnly)
{
	RegistrySettingsStore store{ m_rootPath };
	ASSERT_TRUE(store.SetLong(L"", L"Number", 1));
	ASSERT_TRUE(store.SetLong(L"Sub", L"Number", 2));

	store.DeleteSection(L"Sub");

	EXPECT_FALSE(KeyExists(m_rootPath + L"\\Sub"));
	EXPECT_EQ(store.GetLong(L"Sub", L"Number", -1), -1);
	EXPECT_EQ(store.GetLong(L"", L"Number", -1), 1);
}

TEST_F(RegistrySettingsStoreTest, DeleteMissingSectionIsNoError)
{
	RegistrySettingsStore store{ m_rootPath };

	EXPECT_NO_THROW(store.DeleteSection(L"Missing"));
}

TEST_F(RegistrySettingsStoreTest, DeleteDefaultSectionIsRefused)
{
	RegistrySettingsStore store{ m_rootPath };
	ASSERT_TRUE(store.SetLong(L"", L"Number", 1));

	EXPECT_THROW(store.DeleteSection(L""), std::invalid_argument);
	EXPECT_EQ(store.GetLong(L"", L"Number", -1), 1);
}

TEST_F(RegistrySettingsStoreTest, DeleteSectionThatCannotBeDeletedThrows)
{
	RegistrySettingsStore store{ m_rootPath };

	EXPECT_THROW(store.DeleteSection(std::wstring(300, L'a')), std::runtime_error);
}

/**
 * @file InMemorySettingsStoreTests.cpp
 * @brief Unit tests for DittoCore::InMemorySettingsStore.
 */
#include "InMemorySettingsStore.h"

#include <gtest/gtest.h>

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

using DittoCore::InMemorySettingsStore;
using DittoCore::ISettingsStore;

TEST(InMemorySettingsStore, EmptyStoreGivesTheDefaults)
{
	const InMemorySettingsStore store{};

	EXPECT_EQ(store.GetLong(L"", L"Number", 42), 42);
	EXPECT_EQ(store.GetString(L"", L"Text", L"default", ISettingsStore::NoMaxSize), L"default");
	EXPECT_TRUE(store.GetData(L"", L"Data").empty());
}

TEST(InMemorySettingsStore, ValuesRoundTrip)
{
	InMemorySettingsStore store{};
	const std::vector<std::byte> data{ std::byte{ 0 }, std::byte{ 0xFF } };

	EXPECT_TRUE(store.SetLong(L"", L"Number", -2));
	EXPECT_TRUE(store.SetString(L"", L"Text", L"text"));
	EXPECT_TRUE(store.SetData(L"", L"Data", data));

	EXPECT_EQ(store.GetLong(L"", L"Number", 0), -2);
	EXPECT_EQ(store.GetString(L"", L"Text", L"", ISettingsStore::NoMaxSize), L"text");
	EXPECT_EQ(store.GetData(L"", L"Data"), data);
}

TEST(InMemorySettingsStore, WriteReplacesTheValueAndItsType)
{
	InMemorySettingsStore store{};
	ASSERT_TRUE(store.SetLong(L"", L"Value", 1));

	ASSERT_TRUE(store.SetString(L"", L"Value", L"text"));

	EXPECT_EQ(store.GetLong(L"", L"Value", 9), 9);
	EXPECT_EQ(store.GetString(L"", L"Value", L"", ISettingsStore::NoMaxSize), L"text");
	EXPECT_TRUE(store.GetData(L"", L"Value").empty());
}

TEST(InMemorySettingsStore, SectionsAndNamesAreSeparateAndCaseSensitive)
{
	InMemorySettingsStore store{};
	ASSERT_TRUE(store.SetLong(L"Sub", L"Number", 3));

	EXPECT_EQ(store.GetLong(L"Sub", L"Number", 0), 3);
	EXPECT_EQ(store.GetLong(L"", L"Number", 0), 0);
	EXPECT_EQ(store.GetLong(L"SUB", L"Number", 0), 0);
	EXPECT_EQ(store.GetLong(L"Sub", L"NUMBER", 0), 0);
}

TEST(InMemorySettingsStore, StringIsCutToMaxSizeAsInTheIniFile)
{
	InMemorySettingsStore store{};
	ASSERT_TRUE(store.SetString(L"", L"Text", L"abcdef"));

	EXPECT_EQ(store.GetString(L"", L"Text", L"", 4), L"abc");
	EXPECT_EQ(store.GetString(L"", L"Text", L"", 7), L"abcdef");
	EXPECT_EQ(store.GetString(L"", L"Text", L"", 0), L"");
	EXPECT_EQ(store.GetString(L"", L"Missing", L"abcdef", 3), L"ab");
}

TEST(InMemorySettingsStore, DeleteSectionRemovesItsValuesOnly)
{
	InMemorySettingsStore store{};
	ASSERT_TRUE(store.SetLong(L"", L"Number", 1));
	ASSERT_TRUE(store.SetLong(L"Sub", L"Number", 2));
	ASSERT_TRUE(store.SetString(L"Sub", L"Text", L"x"));

	store.DeleteSection(L"Sub");

	EXPECT_EQ(store.GetLong(L"Sub", L"Number", -1), -1);
	EXPECT_EQ(store.GetString(L"Sub", L"Text", L"gone", ISettingsStore::NoMaxSize), L"gone");
	EXPECT_EQ(store.GetLong(L"", L"Number", -1), 1);
}

TEST(InMemorySettingsStore, DeleteDefaultSectionIsRefused)
{
	InMemorySettingsStore store{};

	EXPECT_THROW(store.DeleteSection(L""), std::invalid_argument);
}

/**
 * @file ClipSavePolicyTests.cpp
 * @brief Unit tests for DittoCore::ClipSavePolicy.
 */
#include "ClipSavePolicy.h"

#include <gtest/gtest.h>

#include <cstdint>

using DittoCore::ClipSavePolicy;
using DittoCore::ClipSaveSettings;
using DittoCore::DuplicateCheck;

namespace
{
	ClipSavePolicy Duplicates(bool allowDuplicates, bool allowBackToBack)
	{
		ClipSaveSettings settings{};
		settings.allowDuplicates = allowDuplicates;
		settings.allowBackToBackDuplicates = allowBackToBack;
		return ClipSavePolicy(settings);
	}

	ClipSavePolicy MaxSize(std::int64_t maxClipSizeInBytes)
	{
		ClipSaveSettings settings{};
		settings.maxClipSizeInBytes = maxClipSizeInBytes;
		return ClipSavePolicy(settings);
	}
}

TEST(ClipSavePolicy, WithoutDuplicatesLooksUpAnyClipWithTheCrc)
{
	EXPECT_EQ(Duplicates(false, false).DuplicateCheckFor(7, 7), DuplicateCheck::AnyByCrc);
	EXPECT_EQ(Duplicates(false, true).DuplicateCheckFor(7, 8), DuplicateCheck::AnyByCrc);
}

TEST(ClipSavePolicy, ReusesTheLastClipForABackToBackRepeat)
{
	EXPECT_EQ(Duplicates(true, false).DuplicateCheckFor(7, 7), DuplicateCheck::LastAdded);
}

TEST(ClipSavePolicy, SavesANewClipWhenDuplicatesAreAllowed)
{
	EXPECT_EQ(Duplicates(true, false).DuplicateCheckFor(7, 8), DuplicateCheck::None);
	EXPECT_EQ(Duplicates(true, true).DuplicateCheckFor(7, 7), DuplicateCheck::None);
}

TEST(ClipSavePolicy, NoLimitAcceptsAnySize)
{
	EXPECT_FALSE(MaxSize(0).TooLarge(UINT64_MAX));
}

TEST(ClipSavePolicy, LimitRejectsOnlyLargerFormats)
{
	EXPECT_FALSE(MaxSize(100).TooLarge(100));
	EXPECT_TRUE(MaxSize(100).TooLarge(101));
}

// Regression: CClip compared the size as an int, so a format above 2 GiB looked small.
TEST(ClipSavePolicy, LimitHoldsAboveTwoGibibytes)
{
	EXPECT_TRUE(MaxSize(100).TooLarge(0x1'0000'0000ULL));
}

TEST(ClipSavePolicy, IgnoresBitmapsOnlyFromListedPrograms)
{
	ClipSaveSettings settings{};
	settings.ignoreDibFromApps = { L"excel.exe" };
	const ClipSavePolicy policy(settings);

	EXPECT_TRUE(policy.IgnoresDibFrom(L"excel.exe"));
	EXPECT_FALSE(policy.IgnoresDibFrom(L"word.exe"));
}

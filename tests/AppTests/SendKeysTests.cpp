/**
 * @file SendKeysTests.cpp
 * @brief Tests of CSendKeys' key string checks (malformed groups are rejected before any key is sent).
 */
#include "stdafx.h"
#include "SendKeys.h"

#include <gtest/gtest.h>

// A '{' without '}' made SendKeys step past the string's terminator and read on.
TEST(SendKeysTests, UnterminatedGroupIsRejected)
{
	CSendKeys sendKeys{};

	EXPECT_FALSE(sendKeys.SendKeys(_T("{ENTER")));
}

// {BEEP} without its numbers read them from behind the group's terminator (stale buffer data).
TEST(SendKeysTests, BeepWithoutArgumentsIsRejected)
{
	CSendKeys sendKeys{};

	EXPECT_FALSE(sendKeys.SendKeys(_T("{BEEP}")));
}

// {APPACTIVATE} without a title read the title from behind the group's terminator.
TEST(SendKeysTests, AppActivateWithoutTitleIsRejected)
{
	CSendKeys sendKeys{};

	EXPECT_FALSE(sendKeys.SendKeys(_T("{APPACTIVATE}")));
}

// A well-formed command group is still accepted ({DELAY} sends no key).
TEST(SendKeysTests, DelayGroupIsAccepted)
{
	CSendKeys sendKeys{};

	EXPECT_TRUE(sendKeys.SendKeys(_T("{DELAY=0}")));
}

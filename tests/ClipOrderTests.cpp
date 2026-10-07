/**
 * @file ClipOrderTests.cpp
 * @brief Unit tests for DittoCore::ClipOrder.
 */
#include "ClipOrder.h"

#include <gtest/gtest.h>

using DittoCore::ClipOrder;

TEST(ClipOrder, NewestGoesOneAboveTheHighest)
{
	EXPECT_EQ(ClipOrder::Newest(41.5), 42.5);
	EXPECT_EQ(ClipOrder::Newest(std::nullopt), 0.0);
}

TEST(ClipOrder, OldestGoesOneBelowTheLowest)
{
	EXPECT_EQ(ClipOrder::Oldest(-3.0), -4.0);
	EXPECT_EQ(ClipOrder::Oldest(std::nullopt), 0.0);
}

TEST(ClipOrder, TopStickyIsNeverZero)
{
	EXPECT_EQ(ClipOrder::TopSticky(std::nullopt), 1.0);
	EXPECT_EQ(ClipOrder::TopSticky(5.0), 6.0);
	EXPECT_EQ(ClipOrder::TopSticky(-1.0), 1.0);
}

TEST(ClipOrder, LastStickyIsNeverZero)
{
	EXPECT_EQ(ClipOrder::LastSticky(std::nullopt), 1.0);
	EXPECT_EQ(ClipOrder::LastSticky(5.0), 4.0);
	EXPECT_EQ(ClipOrder::LastSticky(1.0), -1.0);
}

TEST(ClipOrder, MovesToTheMidpointBetweenNeighbours)
{
	EXPECT_EQ(ClipOrder::MovedPast(10.0, 11.0, true), 10.5);
	EXPECT_EQ(ClipOrder::MovedPast(10.0, 9.0, false), 9.5);
}

TEST(ClipOrder, MovesOnePastTheLastNeighbour)
{
	EXPECT_EQ(ClipOrder::MovedPast(10.0, std::nullopt, true), 11.0);
	EXPECT_EQ(ClipOrder::MovedPast(10.0, std::nullopt, false), 9.0);
}

// Moving a clip up again and again between the same two neighbours must keep it strictly between
// them; with the orders bound as doubles (no longer printed with %f) the SQL sees these values.
TEST(ClipOrder, RepeatedMidpointsStayBetweenTheNeighbours)
{
	double low = 1.0;
	const double high = 2.0;
	for (int i = 0; i < 30; i++)
	{
		const double mid = ClipOrder::MovedPast(low, high, true);
		ASSERT_LT(low, mid);
		ASSERT_LT(mid, high);
		low = mid;
	}
}

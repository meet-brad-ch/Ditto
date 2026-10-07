/**
 * @file ClipOrder.cpp
 * @brief Implements DittoCore::ClipOrder.
 */
#include "ClipOrder.h"

namespace DittoCore
{
	double ClipOrder::Newest(std::optional<double> highest)
	{
		return highest ? *highest + 1 : 0;
	}

	double ClipOrder::Oldest(std::optional<double> lowest)
	{
		return lowest ? *lowest - 1 : 0;
	}

	double ClipOrder::TopSticky(std::optional<double> highest)
	{
		// upstream's rule: a sticky order is never 0
		const double order = highest ? *highest + 1 : 1;
		return order == 0.0 ? 1 : order;
	}

	double ClipOrder::LastSticky(std::optional<double> lowest)
	{
		const double order = lowest ? *lowest - 1 : 1;
		return order == 0.0 ? -1 : order;
	}

	double ClipOrder::MovedPast(double neighbour, std::optional<double> beyond, bool up)
	{
		if (beyond)
		{
			return neighbour + (*beyond - neighbour) / 2.0;
		}
		return up ? neighbour + 1 : neighbour - 1;
	}
}

/**
 * @file ClipOrder.h
 * @brief Declares DittoCore::ClipOrder.
 */
#pragma once

#include <optional>

namespace DittoCore
{
	/**
	 * @brief The sort orders of clips. Higher orders are listed first; a clip moved between two
	 *        others gets the midpoint of their orders. The rules are upstream Ditto's.
	 */
	class ClipOrder
	{
	public:
		/**
		 * @brief The order of a new clip at the top.
		 * @param highest The highest existing order, if there is a clip.
		 * @return One above @p highest, or 0 for the first clip.
		 */
		static double Newest(std::optional<double> highest);

		/**
		 * @brief The order of a clip put at the bottom.
		 * @param lowest The lowest existing order, if there is a clip.
		 * @return One below @p lowest, or 0 for the first clip.
		 */
		static double Oldest(std::optional<double> lowest);

		/**
		 * @brief The sticky order of a clip made the top sticky clip.
		 * @param highest The highest existing sticky order, if there is a sticky clip.
		 * @return One above @p highest, or 1 for the first; never 0.
		 */
		static double TopSticky(std::optional<double> highest);

		/**
		 * @brief The sticky order of a clip made the last sticky clip.
		 * @param lowest The lowest existing sticky order, if there is a sticky clip.
		 * @return One below @p lowest, or 1 for the first; never 0.
		 */
		static double LastSticky(std::optional<double> lowest);

		/**
		 * @brief The order of a clip moved one place up or down, past its neighbour.
		 * @param neighbour The order of the clip it moves past.
		 * @param beyond The order of the next clip after the neighbour, if there is one.
		 * @param up True when moving up (towards higher orders).
		 * @return The midpoint of @p neighbour and @p beyond, or one beyond @p neighbour.
		 */
		static double MovedPast(double neighbour, std::optional<double> beyond, bool up);
	};
}

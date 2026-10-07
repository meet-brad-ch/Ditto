/**
 * @file IRandomRange.h
 * @brief Declares DittoCore::IRandomRange.
 */
#pragma once

namespace DittoCore
{
	/**
	 * @brief A source of random integers, injected into Typoglycemia so tests can fix the order.
	 */
	class IRandomRange
	{
	public:
		/// Destroys the source.
		virtual ~IRandomRange() = default;

		/**
		 * @brief A random integer in a closed range.
		 * @param low The smallest value.
		 * @param high The largest value, at least @p low.
		 * @return A value in [@p low, @p high].
		 */
		virtual int Next(int low, int high) = 0;
	};
}

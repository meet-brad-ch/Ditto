/**
 * @file ClipboardFormatError.h
 * @brief Declares DittoCore::ClipboardFormatError.
 */
#pragma once

#include <stdexcept>
#include <string>

namespace DittoCore
{
	/**
	 * @brief Raised when clipboard data does not have the layout its format promises.
	 *
	 * Clipboard data comes from any process, so this is an expected input error. The top of
	 * the operation that needed the data catches it, stops that operation and shows the
	 * cause; the data is never used.
	 */
	class ClipboardFormatError : public std::runtime_error
	{
	public:
		/**
		 * @brief Creates the error.
		 * @param message What is wrong with the data, for the user-visible report.
		 */
		explicit ClipboardFormatError(const std::string& message) :
			std::runtime_error(message)
		{
		}
	};
}

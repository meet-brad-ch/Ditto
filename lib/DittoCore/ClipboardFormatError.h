#pragma once

#include <stdexcept>
#include <string>

namespace DittoCore
{
	// Raised when clipboard data does not have the layout its format promises.
	// Clipboard data comes from any process, so this is an expected input error:
	// the caller decides how to report it, but must not use the data.
	class ClipboardFormatError : public std::runtime_error
	{
	public:
		explicit ClipboardFormatError(const std::string& message)
			: std::runtime_error(message)
		{
		}
	};
}

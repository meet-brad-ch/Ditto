#pragma once

#include <stdexcept>
#include <string>
#include <tchar.h>
#include <string.h>

/**
 * @brief Fills the text buffer a list or tool tip control hands out in a notification.
 */
class CControlTextBuffer
{
public:
	/**
	 * @brief Copies text into the control's buffer, cut at the buffer size.
	 *
	 * Cutting is the intended display behaviour: the control shows what fits.
	 * @param buffer the control's buffer.
	 * @param bufferSize the buffer size in characters, terminator included.
	 * @param text the text to show.
	 * @throws std::invalid_argument when the control hands out no buffer.
	 * @throws std::runtime_error when the copy fails.
	 */
	static void CopyCut(LPTSTR buffer, int bufferSize, LPCTSTR text)
	{
		if (buffer == nullptr || bufferSize <= 0)
		{
			throw std::invalid_argument("the control handed out no text buffer");
		}

		const errno_t result = _tcsncpy_s(buffer, static_cast<size_t>(bufferSize), text, _TRUNCATE);
		if (result != 0 && result != STRUNCATE)
		{
			throw std::runtime_error("copying text into the control's buffer failed, error " + std::to_string(result));
		}
	}
};

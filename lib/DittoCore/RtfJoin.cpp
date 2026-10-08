/**
 * @file RtfJoin.cpp
 * @brief Implements DittoCore::RtfJoin.
 */
#include "RtfJoin.h"
#include "ClipboardFormatError.h"

#include <cstdint>
#include <string>

namespace DittoCore
{
	RtfJoin::RtfJoin(std::wstring_view separator) :
		m_separator(Escape(separator))
	{
	}

	void RtfJoin::Add(std::string_view document)
	{
		if (!document.starts_with(RtfStart))
		{
			throw ClipboardFormatError("RTF clip does not start with {\\rtf1");
		}
		const std::size_t lastBrace = document.rfind('}');
		if (lastBrace == std::string_view::npos || lastBrace < RtfStart.size())
		{
			throw ClipboardFormatError("RTF clip has no closing brace");
		}
		m_contents.emplace_back(document.substr(RtfStart.size(), lastBrace - RtfStart.size()));
	}

	std::string RtfJoin::Result() const
	{
		if (m_contents.empty())
		{
			throw ClipboardFormatError("no RTF clip to join");
		}
		std::string joined(RtfStart);
		joined += m_contents.front();
		for (std::size_t i = 1; i < m_contents.size(); i++)
		{
			joined += m_separator;
			joined += m_contents[i];
		}
		joined += '}';
		return joined;
	}

	std::string RtfJoin::Escape(std::wstring_view text)
	{
		std::string rtf;
		for (std::size_t i = 0; i < text.size(); i++)
		{
			if (text[i] == L'\r' || text[i] == L'\n')
			{
				// CRLF, CR or LF is one paragraph break
				if (text[i] == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n')
				{
					i++;
				}
				rtf += "\\par ";
			}
			else
			{
				rtf += EscapeCharacter(text[i]);
			}
		}
		return rtf;
	}

	std::string RtfJoin::EscapeCharacter(wchar_t c)
	{
		if (c == L'\\' || c == L'{' || c == L'}')
		{
			return std::string{ '\\', static_cast<char>(c) };
		}
		if (c == L'\t')
		{
			return "\\tab ";
		}
		if (c >= 0x20 && c < 0x7F)
		{
			return std::string(1, static_cast<char>(c));
		}
		// \u takes a signed 16-bit value; "?" is the fallback for readers without Unicode
		return "\\u" + std::to_string(static_cast<std::int16_t>(c)) + "?";
	}
}

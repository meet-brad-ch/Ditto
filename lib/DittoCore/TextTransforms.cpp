/**
 * @file TextTransforms.cpp
 * @brief Implements DittoCore::TextTransforms.
 */
#include "TextTransforms.h"

#include <cwctype>

namespace DittoCore
{
	std::wstring TextTransforms::AsciiOnly(std::wstring_view text)
	{
		std::wstring ascii;
		for (const wchar_t c : text)
		{
			if (c <= 0x7F)
			{
				ascii += c;
			}
		}
		return ascii;
	}

	std::wstring TextTransforms::RemoveLineFeeds(std::wstring_view text)
	{
		std::wstring line;
		for (std::size_t i = 0; i < text.size(); i++)
		{
			if (text[i] == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n')
			{
				i++;   // CRLF is one break
			}
			line += (text[i] == L'\r' || text[i] == L'\n') ? L' ' : text[i];
		}
		return line;
	}

	std::wstring TextTransforms::AddLineFeeds(std::wstring_view text, int count)
	{
		std::wstring result(text);
		for (int i = 0; i < count; i++)
		{
			result += L"\r\n";
		}
		return result;
	}

	std::wstring TextTransforms::AddDateTime(std::wstring_view text, std::wstring_view formattedTime)
	{
		std::wstring result(text);
		result += L"\r\n";
		result += formattedTime;
		return result;
	}

	std::wstring TextTransforms::Trim(std::wstring_view text)
	{
		std::size_t first = 0;
		while (first < text.size() && std::iswspace(text[first]))
		{
			first++;
		}
		std::size_t last = text.size();
		while (last > first && std::iswspace(text[last - 1]))
		{
			last--;
		}
		return std::wstring(text.substr(first, last - first));
	}

	std::wstring TextTransforms::PosixifyPaths(std::wstring_view text)
	{
		const std::wstring trimmed = Trim(text);
		std::wstring posix;
		for (std::size_t i = 0; i < trimmed.size(); i++)
		{
			if (IsDriveAt(trimmed, i))
			{
				posix += L'/';
				posix += static_cast<wchar_t>(std::towlower(trimmed[i]));
				posix += L'/';
				i += 2;   // the colon and the backslash
			}
			else
			{
				posix += trimmed[i] == L'\\' ? L'/' : trimmed[i];
			}
		}
		return posix;
	}

	bool TextTransforms::IsDriveAt(std::wstring_view text, std::size_t pos)
	{
		const bool letter = (text[pos] >= L'A' && text[pos] <= L'Z') || (text[pos] >= L'a' && text[pos] <= L'z');
		return letter && pos + 2 < text.size() && text[pos + 1] == L':' && text[pos + 2] == L'\\';
	}
}

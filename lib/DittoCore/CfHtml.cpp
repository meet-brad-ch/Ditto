/**
 * @file CfHtml.cpp
 * @brief Implements DittoCore::CfHtml.
 */
#include "CfHtml.h"
#include "ClipText.h"
#include "ClipboardFormatError.h"

#include <charconv>
#include <format>

#include <windows.h>

namespace DittoCore
{
	CfHtmlFragment CfHtml::Parse(std::span<const std::byte> block)
	{
		const std::string text = ClipText::ReadAnsiBounded(block);
		const Header header = ReadHeader(text);
		if (!header.startFragment || !header.endFragment)
		{
			throw ClipboardFormatError("CF_HTML header has no StartFragment/EndFragment offsets");
		}
		const std::size_t start = *header.startFragment;
		const std::size_t end = *header.endFragment;
		if (start > end || end > text.size())
		{
			throw ClipboardFormatError(std::format("CF_HTML fragment {}..{} is outside the {} bytes of HTML", start, end, text.size()));
		}
		return CfHtmlFragment{ header.version, header.sourceUrl, Trim(std::string_view(text).substr(start, end - start)) };
	}

	CfHtml::Header CfHtml::ReadHeader(const std::string& text)
	{
		Header header{};
		std::size_t pos{};
		while (pos < text.size())
		{
			const std::size_t eol = text.find_first_of("\r\n", pos);
			const std::string_view line = std::string_view(text).substr(pos, eol == std::string::npos ? std::string::npos : eol - pos);
			const std::size_t colon = line.find(':');
			if (colon == std::string_view::npos || line.starts_with('<'))
			{
				break; // the header ends where the HTML starts
			}
			ApplyField(header, line.substr(0, colon), line.substr(colon + 1));
			pos = eol == std::string::npos ? text.size() : text.find_first_not_of("\r\n", eol);
		}
		return header;
	}

	void CfHtml::ApplyField(Header& header, std::string_view name, std::string_view value)
	{
		if (name == "Version")
		{
			header.version = std::string(value);
		}
		else if (name == "SourceURL")
		{
			header.sourceUrl = std::string(value);
		}
		else if (name == "StartFragment")
		{
			header.startFragment = ParseOffset(value);
		}
		else if (name == "EndFragment")
		{
			header.endFragment = ParseOffset(value);
		}
	}

	std::size_t CfHtml::ParseOffset(std::string_view value)
	{
		const std::string_view digits = value.substr(0, value.find_last_not_of(' ') + 1);
		std::size_t offset{};
		const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), offset);
		if (digits.empty() || error != std::errc{} || end != digits.data() + digits.size())
		{
			throw ClipboardFormatError("CF_HTML offset '" + std::string(value) + "' is not a number");
		}
		return offset;
	}

	std::string CfHtml::Trim(std::string_view text)
	{
		const std::size_t first = text.find_first_not_of(" \t\r\n");
		if (first == std::string_view::npos)
		{
			return std::string();
		}
		return std::string(text.substr(first, text.find_last_not_of(" \t\r\n") - first + 1));
	}

	std::string CfHtml::Build(const std::string& fragment, const std::string& version, const std::string& sourceUrl)
	{
		static constexpr std::string_view htmlStart = "<html><body>\r\n<!--StartFragment-->";
		static constexpr std::string_view htmlEnd = "<!--EndFragment-->\r\n</body>\r\n</html>";
		const std::string versionLine = "Version:" + (version.empty() ? std::string("0.9") : version) + "\r\n";
		const std::string urlLine = sourceUrl.empty() ? std::string() : "SourceURL:" + sourceUrl + "\r\n";

		// four offset lines of fixed width: "Name:" + 10 digits + CRLF
		const std::size_t headerSize = versionLine.size() + urlLine.size() +
									   std::string_view("StartHTML:EndHTML:StartFragment:EndFragment:").size() + 4 * 12;
		const std::size_t startFragment = headerSize + htmlStart.size();
		const std::size_t endFragment = startFragment + fragment.size();
		const std::size_t endHtml = endFragment + htmlEnd.size();

		return versionLine +
			   std::format("StartHTML:{:010}\r\nEndHTML:{:010}\r\nStartFragment:{:010}\r\nEndFragment:{:010}\r\n", headerSize, endHtml, startFragment, endFragment) +
			   urlLine + std::string(htmlStart) + fragment + std::string(htmlEnd);
	}

	std::string CfHtml::HtmlFromText(std::wstring_view text)
	{
		std::wstring html;
		for (std::size_t i = 0; i < text.size(); ++i)
		{
			if (text[i] == L'\r' && i + 1 < text.size() && text[i + 1] == L'\n')
			{
				continue; // CR LF is one line break: the LF writes it
			}
			html += Escape(text[i]);
		}
		return ToUtf8(html);
	}

	std::wstring CfHtml::Escape(wchar_t c)
	{
		switch (c)
		{
		case L'&': return L"&amp;";
		case L'<': return L"&lt;";
		case L'>': return L"&gt;";
		case L'"': return L"&quot;";
		case L'\r':
		case L'\n': return L"<br>";
		default: return std::wstring(1, c);
		}
	}

	std::string CfHtml::ToUtf8(const std::wstring& text)
	{
		if (text.empty())
		{
			return std::string();
		}
		const int length = ::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
		if (length <= 0)
		{
			throw ClipboardFormatError("text cannot be converted to UTF-8 (error " + std::to_string(::GetLastError()) + ")");
		}
		std::string utf8(static_cast<std::size_t>(length), '\0');
		::WideCharToMultiByte(CP_UTF8, 0, text.data(), static_cast<int>(text.size()), utf8.data(), length, nullptr, nullptr);
		return utf8;
	}
}

/**
 * @file RtfNormalizer.cpp
 * @brief Implements DittoCore::RtfNormalizer.
 */
#include "RtfNormalizer.h"

namespace DittoCore
{
	std::string RtfNormalizer::Normalize(std::string_view rtf)
	{
		std::string normalized(rtf);
		RemoveSection(normalized, "{\\*\\datastore");
		DeleteControlWord(normalized, "\\rsid", true);
		DeleteControlWord(normalized, "\\insrsid", true);
		DeleteControlWord(normalized, "\\mdispDef1", false);
		return normalized;
	}

	void RtfNormalizer::RemoveSection(std::string& rtf, std::string_view section)
	{
		const std::size_t start = rtf.find(section);
		if (start == std::string::npos)
		{
			return;
		}
		int depth{};
		for (std::size_t pos = start + 1; pos < rtf.size(); pos++)
		{
			if (rtf[pos] == '{')
			{
				depth++;
			}
			else if (rtf[pos] == '}' && depth > 0)
			{
				depth--;
			}
			else if (rtf[pos] == '}')
			{
				rtf.erase(start, pos - start + 1);
				return;
			}
		}
	}

	void RtfNormalizer::DeleteControlWord(std::string& rtf, std::string_view word, bool trailingDigits)
	{
		std::size_t start = rtf.find(word);
		while (start != std::string::npos)
		{
			// "\\rsid" in the text is an escaped backslash followed by letters, not the control word
			const bool escaped = start > 0 && rtf[start - 1] == '\\';
			const std::size_t end = escaped ? start + 1 : ControlWordEnd(rtf, start, word, trailingDigits);
			if (end == std::string::npos)
			{
				return;
			}
			const bool hasNumber = end != start + word.size();
			if (!escaped && (!trailingDigits || hasNumber))
			{
				rtf.erase(start, end - start);
				start = rtf.find(word, start);
			}
			else
			{
				start = rtf.find(word, start + 1);
			}
		}
	}

	std::size_t RtfNormalizer::ControlWordEnd(const std::string& rtf, std::size_t start, std::string_view word, bool trailingDigits)
	{
		std::size_t end = start + word.size();
		if (!trailingDigits)
		{
			return end;
		}
		while (end < rtf.size() && rtf[end] >= '0' && rtf[end] <= '9')
		{
			end++;
		}
		return end < rtf.size() ? end : std::string::npos;
	}
}

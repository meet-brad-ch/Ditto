/**
 * @file CaseTransforms.cpp
 * @brief Implements DittoCore::CaseTransforms.
 */
#include "CaseTransforms.h"

#include <cwctype>

namespace DittoCore
{
	CaseTransforms::CaseTransforms(const ICaseMapper& cases) noexcept
		: m_cases(cases)
	{
	}

	std::wstring CaseTransforms::Upper(std::wstring_view text) const
	{
		return m_cases.ToUpper(text);
	}

	std::wstring CaseTransforms::Lower(std::wstring_view text) const
	{
		return m_cases.ToLower(text);
	}

	std::wstring CaseTransforms::InvertCase(std::wstring_view text) const
	{
		std::wstring inverted;
		inverted.reserve(text.size());
		for (const wchar_t c : text)
		{
			inverted += m_cases.IsUpper(c) ? m_cases.ToLower(c) : m_cases.ToUpper(c);
		}
		return inverted;
	}

	std::wstring CaseTransforms::CamelCase(std::wstring_view text) const
	{
		std::wstring camel;
		bool startOfWord = true;
		for (const wchar_t c : text)
		{
			if (c == L' ')
			{
				startOfWord = true;
			}
			else
			{
				camel += startOfWord ? m_cases.ToUpper(c) : m_cases.ToLower(c);
				startOfWord = false;
			}
		}
		return camel;
	}

	std::wstring CaseTransforms::Capitalize(std::wstring_view text) const
	{
		return RaiseAfter(text, [](wchar_t c) { return c == L' '; }, [](wchar_t c) { return c == L' '; });
	}

	std::wstring CaseTransforms::SentenceCase(std::wstring_view text) const
	{
		return RaiseAfter(text,
			[](wchar_t c) { return c == L'.' || c == L'!' || c == L'?'; },
			[](wchar_t c) { return std::iswspace(c) != 0; });
	}

	template <typename Separator, typename Skipped>
	std::wstring CaseTransforms::RaiseAfter(std::wstring_view text, Separator isSeparator, Skipped isSkipped) const
	{
		std::wstring result = m_cases.ToLower(text);
		bool raiseNext = true;
		for (wchar_t& c : result)
		{
			if (isSeparator(c))
			{
				raiseNext = true;
			}
			else if (raiseNext && !isSkipped(c))
			{
				c = m_cases.ToUpper(c);
				raiseNext = false;
			}
		}
		return result;
	}
}

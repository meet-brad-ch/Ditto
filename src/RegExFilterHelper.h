#pragma once

#include <vector>
#include <string>

class CRegExFilterData
{
public:
	std::wstring m_regEx;
	CString m_processFilters;
	CStringArray m_parsedProcessFilters;

	/**
	 * @brief Splits m_processFilters at the separator into m_parsedProcessFilters (empty parts
	 *        are skipped).
	 * @param separator The separator characters (the settings' CopyAppSeparator).
	 */
	void ParseFilters(const CString& separator);

	bool MatchesProcessFilters(CString& activeApp);
	bool MatchesRegEx(std::wstring& copiedText);

	/**
	 * @brief Copies the regex, the process filters and the parsed process filters.
	 * @param clip The filter to copy.
	 * @return This filter.
	 */
	const CRegExFilterData& operator=(const CRegExFilterData& clip)
	{
		m_regEx = clip.m_regEx;
		m_processFilters = clip.m_processFilters;
		m_parsedProcessFilters.Copy(clip.m_parsedProcessFilters);

		return *this;
	}
};

class CRegExFilterHelper
{
public:
	/** @brief The number of filter slots (stored in the settings by slot number). */
	static constexpr int MaxRegexFilters = 15;

	CRegExFilterHelper();
	~CRegExFilterHelper();

	/**
	 * @brief Puts a filter into a slot and parses its process filters; an invalid slot is ignored.
	 * @param pos The slot, 0 .. MaxRegexFilters - 1.
	 * @param data The filter (regex and unparsed process filters).
	 * @param separator The process filter separator (the settings' CopyAppSeparator).
	 */
	void Add(int pos, CRegExFilterData& data, const CString& separator);
	void SetRegEx(int pos, std::wstring regEx);
	/**
	 * @brief Sets and parses the process filters of a slot; an invalid slot is ignored.
	 * @param pos The slot, 0 .. MaxRegexFilters - 1.
	 * @param processName The process filters, separated by @p separator.
	 * @param separator The process filter separator (the settings' CopyAppSeparator).
	 */
	void SetProcessFilter(int pos, CString processName, const CString& separator);

	CRegExFilterData m_filters[MaxRegexFilters];

	bool TextMatchFilters(CString& activeApp, std::wstring& copiedText);

private:
	CCriticalSection m_critSection;
};

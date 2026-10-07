#pragma once

#include <vector>
#include <string>

class CRegExFilterData
{
public:
	std::wstring m_regEx;
	CString m_processFilters;
	CStringArray m_parsedProcessFilters;

	void ParseFilters();

	bool MatchesProcessFilters(CString &activeApp);
	bool MatchesRegEx(std::wstring &copiedText);

	const CRegExFilterData& operator=(const CRegExFilterData &clip)
	{
		m_regEx = clip.m_regEx;
		m_processFilters = clip.m_processFilters;

		ParseFilters();
		
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

	void Add(int pos, CRegExFilterData &data);
	void SetRegEx(int pos, std::wstring regEx);
	void SetProcessFilter(int pos, CString processName);

	CRegExFilterData m_filters[MaxRegexFilters];

	bool TextMatchFilters(CString &activeApp, std::wstring &copiedText);

private:
	CCriticalSection m_critSection;
};


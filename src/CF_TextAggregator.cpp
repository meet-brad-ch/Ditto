#include "stdafx.h"
#include ".\cf_textaggregator.h"
#include "Misc.h"
#include "..\Shared\TextConvert.h"
#include "ClipText.h"
#include "FileDropList.h"

#include <string>
#include <vector>

CCF_TextAggregator::CCF_TextAggregator(const CStringA& separator) :
	m_join(std::string_view(separator.GetString(), separator.GetLength()))
{
}

bool CCF_TextAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType)
{
	const std::size_t size = static_cast<std::size_t>(nDataSize);
	if (cfType == CF_HDROP)
	{
		std::vector<std::string> lines;
		for (const std::wstring& path : DittoCore::FileDropList::Parse(lpData, size).Paths())
		{
			const CStringA ansi = CTextConvert::UnicodeToAnsi(CString(path.c_str()));
			lines.emplace_back(ansi.GetString(), ansi.GetLength());
		}
		return m_join.AddLines(lines);
	}

	m_join.Add(DittoCore::ClipText::ReadAnsi(lpData, size));
	return true;
}

HGLOBAL CCF_TextAggregator::GetHGlobal()
{
	const std::string& text = m_join.Result();
	// with its terminating null
	return NewGlobalP(const_cast<char*>(text.c_str()), text.size() + 1);
}

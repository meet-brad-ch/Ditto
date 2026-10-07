#include "stdafx.h"
#include ".\cf_hdropaggregator.h"
#include "Misc.h"
#include "FileDropList.h"
#include "TextJoin.h"

bool CCF_HDropAggregator::AddClip(LPVOID lpData, int nDataSize, int /*nPos*/, int /*nCount*/, UINT /*cfType*/)
{
	for (std::wstring& path : DittoCore::FileDropList::Parse(lpData, static_cast<std::size_t>(nDataSize)).Paths())
	{
		m_paths.push_back(std::move(path));
	}
	return true;
}

HGLOBAL CCF_HDropAggregator::GetHGlobal()
{
	return NewDropBlock(m_paths);
}

HGLOBAL CCF_HDropAggregator::GetHGlobalAsString()
{
	DittoCore::TextJoin<wchar_t> lines(L"");
	lines.AddLines(m_paths);
	const std::wstring& text = lines.Result();
	// with its terminating null
	return NewGlobalP(const_cast<wchar_t*>(text.c_str()), (text.size() + 1) * sizeof(wchar_t));
}

HGLOBAL CCF_HDropAggregator::NewDropBlock(const std::vector<std::wstring>& paths)
{
	if (paths.empty())
	{
		return NULL;
	}
	std::vector<std::byte> block = DittoCore::FileDropList::Build(paths);
	return NewGlobalP(block.data(), block.size());
}

#include "stdafx.h"
#include ".\cf_unicodetextaggregator.h"
#include "Misc.h"
#include "ClipText.h"
#include "FileDropList.h"

#include <string>

CCF_UnicodeTextAggregator::CCF_UnicodeTextAggregator(const CStringW& separator) :
	m_join(std::wstring_view(separator.GetString(), separator.GetLength()))
{
}

bool CCF_UnicodeTextAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType)
{
	const std::size_t size = static_cast<std::size_t>(nDataSize);
	if (cfType == CF_HDROP)
	{
		return m_join.AddLines(DittoCore::FileDropList::Parse(lpData, size).Paths());
	}

	m_join.Add(DittoCore::ClipText::ReadWide(lpData, size));
	return true;
}

HGLOBAL CCF_UnicodeTextAggregator::GetHGlobal()
{
	const std::wstring& text = m_join.Result();
	// with its terminating null
	return NewGlobalP(const_cast<wchar_t*>(text.c_str()), (text.size() + 1) * sizeof(wchar_t));
}

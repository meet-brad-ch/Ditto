#include "stdafx.h"
#include ".\richtextaggregator.h"
#include "Misc.h"
#include "ClipText.h"

#include <string>

CRichTextAggregator::CRichTextAggregator(const CStringW& separator) :
	m_join(std::wstring_view(separator.GetString(), separator.GetLength()))
{
}

bool CRichTextAggregator::AddClip(LPVOID lpData, int nDataSize, int /*nPos*/, int /*nCount*/, UINT /*cfType*/)
{
	// RTF is length-delimited: read up to the first null or the end of the blob
	m_join.Add(DittoCore::ClipText::ReadAnsiBounded(lpData, static_cast<std::size_t>(nDataSize)));
	return true;
}

HGLOBAL CRichTextAggregator::GetHGlobal()
{
	const std::string rtf = m_join.Result();
	// with its terminating null
	return CGlobalMemory::NewGlobalP(const_cast<char*>(rtf.c_str()), rtf.size() + 1);
}

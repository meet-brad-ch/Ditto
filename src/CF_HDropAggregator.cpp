#include "stdafx.h"
#include ".\cf_hdropaggregator.h"
#include "FileDropList.h"

CCF_HDropAggregator::CCF_HDropAggregator(void)
{
}

CCF_HDropAggregator::~CCF_HDropAggregator(void)
{
}

bool CCF_HDropAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType)
{
	for (const std::wstring& path : DittoCore::FileDropList::Parse(lpData, static_cast<std::size_t>(nDataSize)).Paths())
	{
		m_DropFiles.AddFile(path.c_str());
	}

	return true;
}

HGLOBAL CCF_HDropAggregator::GetHGlobal()
{
	return m_DropFiles.CreateCF_HDROPBuffer();
}

HGLOBAL CCF_HDropAggregator::GetHGlobalAsString()
{
	return m_DropFiles.CreateCF_HDROPBufferAsString();
}
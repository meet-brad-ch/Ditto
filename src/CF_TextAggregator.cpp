#include "stdafx.h"
#include ".\cf_textaggregator.h"
#include "Misc.h"
#include "..\Shared\TextConvert.h"
#include "ClipText.h"
#include "FileDropList.h"

CCF_TextAggregator::CCF_TextAggregator(CStringA csSepator) :
	m_csSeparator(csSepator)
{
}

CCF_TextAggregator::~CCF_TextAggregator(void)
{
}

bool CCF_TextAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType)
{
	const std::size_t size = static_cast<std::size_t>(nDataSize);
	if (cfType == CF_HDROP)
	{
		CStringA hDropFiles = _T("");
		for (const std::wstring& path : DittoCore::FileDropList::Parse(lpData, size).Paths())
		{
			hDropFiles += CTextConvert::UnicodeToAnsi(CString(path.c_str()));
			hDropFiles += "\r\n";
		}

		if (hDropFiles != _T(""))
		{
			m_csNewText += hDropFiles;

			if (nPos != nCount - 1)
			{
				m_csNewText += m_csSeparator;
			}

			return true;
		}
		return false;
	}

	m_csNewText += DittoCore::ClipText::ReadAnsi(lpData, size).c_str();
	
	if(nPos != nCount-1)
	{
		m_csNewText += m_csSeparator;
	}

	return true;
}

HGLOBAL CCF_TextAggregator::GetHGlobal()
{
	long lLen = m_csNewText.GetLength();
	HGLOBAL hGlobal = NewGlobalP(m_csNewText.GetBuffer(lLen), lLen+sizeof(char));
	m_csNewText.ReleaseBuffer();

	return hGlobal;
}

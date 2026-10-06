#include "stdafx.h"
#include ".\cf_unicodetextaggregator.h"
#include "Misc.h"
#include "ClipText.h"
#include "FileDropList.h"

CCF_UnicodeTextAggregator::CCF_UnicodeTextAggregator(CStringW csSeparator) :
	m_csSeparator(csSeparator)
{
}

CCF_UnicodeTextAggregator::~CCF_UnicodeTextAggregator(void)
{
}

bool CCF_UnicodeTextAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType)
{
	if (cfType == CF_HDROP)
	{
		CString hDropFiles = _T("");
		for (const std::wstring& path : DittoCore::FileDropList::Parse(lpData, static_cast<std::size_t>(nDataSize)).Paths())
		{
			hDropFiles += path.c_str();
			hDropFiles += _T("\r\n");
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

	m_csNewText += DittoCore::ClipText::ReadWide(lpData, static_cast<std::size_t>(nDataSize)).c_str();
	
	if(nPos != nCount-1)
	{
		m_csNewText += m_csSeparator;
	}

	return true;
}

HGLOBAL CCF_UnicodeTextAggregator::GetHGlobal()
{
	long lLen = m_csNewText.GetLength() * sizeof(wchar_t);
	HGLOBAL hGlobal = NewGlobalP(m_csNewText.GetBuffer(lLen), lLen+sizeof(wchar_t));
	m_csNewText.ReleaseBuffer();

	return hGlobal;
}
#include "stdafx.h"
#include ".\richtextaggregator.h"
#include "Misc.h"
#include "ClipText.h"
#include "ClipboardFormatError.h"

#include <string>

CRichTextAggregator::CRichTextAggregator(CStringA csSeparator) :
	m_csSeparator(csSeparator)
{
	//Remove the first line feed
//	if(m_csSeparator.GetLength() > 1 && m_csSeparator[0] == '\r' && m_csSeparator[1] == '\n')
//	{
//		m_csSeparator.Delete(0);
//		m_csSeparator.Delete(0);
//	}

	m_csSeparator.Replace("\r\n", "\\par");
}

CRichTextAggregator::~CRichTextAggregator(void)
{
}

bool CRichTextAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType)
{
	// RTF is length-delimited: read up to the first null or the end of the blob, into a copy
	std::string text = DittoCore::ClipText::ReadAnsiBounded(lpData, static_cast<std::size_t>(nDataSize));

	if(nPos != nCount-1)
	{
		//Remove the last } at the end of the rtf, and anything after it
		const std::size_t lastBrace = text.rfind('}');
		text.erase(lastBrace == std::string::npos ? 0 : lastBrace);
	}
	else if(nPos >= 1)
	{
		//Remove the {\rtf1 at the start of the rtf
		const std::string rtfStart("{\\rtf1");
		if(text.compare(0, rtfStart.size(), rtfStart) != 0)
		{
			throw DittoCore::ClipboardFormatError("RTF clip does not start with {\\rtf1");
		}
		text.erase(0, rtfStart.size());
	}

	m_csNewText += text.c_str();

	if(nPos != nCount-1)
	{
		m_csNewText += m_csSeparator;
	}

	return true;
}

HGLOBAL CRichTextAggregator::GetHGlobal()
{
	long lLen = m_csNewText.GetLength();
	HGLOBAL hGlobal = NewGlobalP(m_csNewText.GetBuffer(lLen), lLen+sizeof(char));
	m_csNewText.ReleaseBuffer();

	return hGlobal;
}
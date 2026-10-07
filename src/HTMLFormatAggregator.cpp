#include "stdafx.h"
#include ".\htmlformataggregator.h"
#include "Misc.h"
#include "CfHtml.h"
#include "ClipboardFormatError.h"

#include <span>

CHTMLFormatAggregator::CHTMLFormatAggregator(const CStringW& separator) :
	m_separatorHtml(DittoCore::CfHtml::HtmlFromText(separator.GetString()))
{
}

bool CHTMLFormatAggregator::AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT /*cfType*/)
{
	if (nDataSize < 0)
	{
		throw DittoCore::ClipboardFormatError("CF_HTML clip has a negative size");
	}
	const DittoCore::CfHtmlFragment parsed = DittoCore::CfHtml::Parse(
		std::span<const std::byte>(static_cast<const std::byte*>(lpData), static_cast<std::size_t>(nDataSize)));
	if (parsed.fragment.empty())
	{
		return true;
	}

	m_html += parsed.fragment;
	if (m_sourceUrl.empty())
	{
		m_sourceUrl = parsed.sourceUrl;
	}
	if (m_version.empty())
	{
		m_version = parsed.version;
	}
	if (nPos != nCount - 1)
	{
		m_html += m_separatorHtml;
	}
	return true;
}

HGLOBAL CHTMLFormatAggregator::GetHGlobal()
{
	const std::string block = DittoCore::CfHtml::Build(m_html, m_version, m_sourceUrl);
	// with the terminating null that std::string keeps after its characters
	return CGlobalMemory::NewGlobalP(const_cast<char*>(block.c_str()), block.size() + 1);
}

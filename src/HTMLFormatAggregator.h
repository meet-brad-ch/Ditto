#pragma once
#include "IClipAggregator.h"

#include <string>

// Joins the CF_HTML fragments of several clips into one CF_HTML block (DittoCore::CfHtml does the
// parsing and building).
class CHTMLFormatAggregator : public IClipAggregator
{
public:
	// separator: the multi-paste separator as plain text; it is inserted as escaped HTML
	explicit CHTMLFormatAggregator(const CStringW& separator);

	// Adds one clip's CF_HTML; throws DittoCore::ClipboardFormatError when it is malformed
	virtual bool AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType);
	virtual HGLOBAL GetHGlobal();

protected:
	std::string m_separatorHtml;
	std::string m_html;
	std::string m_sourceUrl;
	std::string m_version;
};

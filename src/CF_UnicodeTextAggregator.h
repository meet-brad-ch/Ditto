#pragma once
#include "IClipAggregator.h"
#include "TextJoin.h"

// Joins the CF_UNICODETEXT of several clips (DittoCore::TextJoin); a file list is added as its paths.
class CCF_UnicodeTextAggregator : public IClipAggregator
{
public:
	explicit CCF_UnicodeTextAggregator(const CStringW& separator);

	// Adds one clip's CF_UNICODETEXT or CF_HDROP; throws DittoCore::ClipboardFormatError when it is malformed
	virtual bool AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType);
	virtual HGLOBAL GetHGlobal();

protected:
	DittoCore::TextJoin<wchar_t> m_join;
};

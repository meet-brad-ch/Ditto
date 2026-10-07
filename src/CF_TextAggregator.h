#pragma once
#include "IClipAggregator.h"
#include "TextJoin.h"

// Joins the CF_TEXT of several clips (DittoCore::TextJoin); a file list is added as its paths.
class CCF_TextAggregator : public IClipAggregator
{
public:
	explicit CCF_TextAggregator(const CStringA& separator);

	// Adds one clip's CF_TEXT or CF_HDROP; throws DittoCore::ClipboardFormatError when it is malformed
	virtual bool AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType);
	virtual HGLOBAL GetHGlobal();

protected:
	DittoCore::TextJoin<char> m_join;
};

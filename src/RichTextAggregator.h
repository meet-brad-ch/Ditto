#pragma once
#include "IClipAggregator.h"
#include "RtfJoin.h"

// Joins the RTF of several clips into one document (DittoCore::RtfJoin does the joining).
class CRichTextAggregator : public IClipAggregator
{
public:
	// separator: the multi-paste separator as plain text; it is inserted as escaped RTF
	explicit CRichTextAggregator(const CStringW& separator);

	// Adds one clip's RTF; throws DittoCore::ClipboardFormatError when it is not an RTF document
	virtual bool AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType);
	virtual HGLOBAL GetHGlobal();

protected:
	DittoCore::RtfJoin m_join;
};

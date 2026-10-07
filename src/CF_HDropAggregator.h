#pragma once
#include "IClipAggregator.h"

#include <string>
#include <vector>

// Joins the file lists of several clips into one CF_HDROP (DittoCore::FileDropList).
class CCF_HDropAggregator : public IClipAggregator
{
public:
	// Adds one clip's CF_HDROP; throws DittoCore::ClipboardFormatError when it is malformed
	virtual bool AddClip(LPVOID lpData, int nDataSize, int nPos, int nCount, UINT cfType);
	// The joined list as CF_HDROP; NULL when no clip had a file
	virtual HGLOBAL GetHGlobal();
	// The joined list as UTF-16 text, one path per line
	virtual HGLOBAL GetHGlobalAsString();

	// A CF_HDROP block of the paths; NULL when there are none
	static HGLOBAL NewDropBlock(const std::vector<std::wstring>& paths);

protected:
	std::vector<std::wstring> m_paths;
};

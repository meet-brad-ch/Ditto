#pragma once
#include "..\..\Shared\DittoDefines.h"
#include "..\..\Shared\IClip.h"

#include <afxcoll.h>

class CReadOnlyFlag
{
public:
	CReadOnlyFlag(void);
	~CReadOnlyFlag(void);

	bool ResetReadOnlyFlag(const CDittoInfo& DittoInfo, IClip* pClip, bool resetFlag);

protected:
	bool LoadUnicodeFiles(CStringArray& lines, IClipFormats* pFormats);
	bool LoadTextFiles(CStringArray& lines, IClipFormats* pFormats);
	bool LoadHDropFiles(CStringArray& lines, IClipFormats* pFormats);

private:
	/**
	 * @brief Fills lines with the clip's file names: from CF_HDROP, else CF_UNICODETEXT, else CF_TEXT.
	 * @param lines receives one entry per file (or text line).
	 * @param pFormats the clip formats.
	 * @throws DittoCore::ClipboardFormatError for malformed clip data.
	 */
	void LoadFileLines(CStringArray& lines, IClipFormats* pFormats);
	/**
	 * @brief Cuts the text before the first "//", "\\" or drive ("x:\" or "x:/") in a line.
	 * @param file the lower-case line.
	 * @return the line from the start of the path on, unchanged if none is found.
	 */
	static CString SkipToFileStart(CString file);
};

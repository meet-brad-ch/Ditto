#pragma once

#include "ClipIds.h"
#include "SpecialPasteOptions.h"
#include "CaseTransforms.h"
#include "IcuCaseMapper.h"

#include <functional>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

/*------------------------------------------------------------------*\
	COleClipSource
\*------------------------------------------------------------------*/
class COleClipSource : public COleDataSource
{
	//DECLARE_DYNAMIC(COleClipSource)

public:
	CClipIDs	m_ClipIDs;
	bool		m_bLoadedFormats;
	CSpecialPasteOptions m_pasteOptions;

	COleClipSource();
	virtual ~COleClipSource();

	BOOL DoDelayRender();
	BOOL DoImmediateRender();

	void PlainTextFilter(CClip &clip);

	INT_PTR PutFormatOnClipboard(CClipFormats *pFormats);

public:
	virtual BOOL OnRenderGlobalData(LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal);

protected:
	CClipFormats m_DelayRenderedFormats;
	bool m_convertToHDROPOnDelayRender;

	std::optional<HGLOBAL> RenderClipsOrReport(CLIPFORMAT format);

	// Case mapping for the special-paste transforms (ICU)
	CIcuCaseMapper m_caseMapper;
	DittoCore::CaseTransforms m_cases;

	// Applies a transform to the clip's text: CF_UNICODETEXT, or CF_TEXT when there is none;
	// CF_TEXT is then rewritten from the result
	void TransformText(CClip &clip, const std::function<std::wstring(std::wstring_view)>& transform);
	// Applies a transform to the clip's RTF, when it has one
	void TransformRtf(CClip &clip, const std::function<std::string(std::string_view)>& transform);

	void DoUpperLowerCase(CClip &clip, bool upper);
	void Capitalize(CClip &clip);
	void SentenceCase(CClip &clip);
	void RemoveLineFeeds(CClip &clip);
	void AddLineFeeds(CClip &clip, int count);
	void Typoglycemia(CClip &clip);
	HGLOBAL ConvertToFileDrop();
	void AddDateTime(CClip &clip);
	void SaveDittoFileDataToFile(CClip &clip);
	// Writes the files of one "Ditto File Data" record to the drag-files folder and adds them to
	// dropFiles; throws DittoCore::ClipboardFormatError for a malformed record or a failed MD5
	// check, CFileException when a file cannot be written. Returns whether it held any file.
	static bool SaveFileDataRecord(HGLOBAL record, std::vector<std::wstring>& dropFiles);
	// The file name of originalPath, numbered ("name (2).ext") when usedNames already has it.
	static CString UniqueFileName(const CString& originalPath, std::set<CString>& usedNames);
	void TrimWhiteSpace(CClip &clip);
	void PosixifyPaths(CClip &clip);
	void Slugify(CClip &clip);
	void InvertCase(CClip &clip);
	void CamelCase(CClip& clip);
	void AsciiOnly(CClip& clip);
	void PutGuidOntoClipboard(CClip& clip);
	void PasteAsImage(CClip& clip);
};
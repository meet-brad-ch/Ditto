#pragma once

#include "ClipIds.h"
#include "SpecialPasteOptions.h"

#include <optional>
#include <set>
#include <string>
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
	struct MatchInfoA
	{
		size_t pos;
		char drive;
	};
	struct MatchInfoW
	{
		size_t pos;
		wchar_t drive;
	};
	void ApplyDriveReplacements(std::string& str, const std::vector<MatchInfoA>& matches);
	void ApplyDriveReplacements(std::wstring& str, const std::vector<MatchInfoW>& matches);
	CStringA ConvertDrivesASCII(const CStringA& input);
	CStringW ConvertDrivesWide(const CStringW& input);
	void Slugify(CClip &clip);
	void InvertCase(CClip &clip);
	void CamelCase(CClip& clip);
	void AsciiOnly(CClip& clip);
	void PutGuidOntoClipboard(CClip& clip);
	void PasteAsImage(CClip& clip);
};
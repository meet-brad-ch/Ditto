#pragma once

#include "ClipIds.h"
#include "SpecialPasteOptions.h"
#include "CaseTransforms.h"
#include "IcuCaseMapper.h"

#include <array>
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
	/**
	 * @brief Adds the image as a PNG format to the clip; on a stream, save, allocation or lock failure
	 *        the clip keeps only its DIB.
	 * @param clip The clip that receives the PNG format.
	 * @param image The loaded image.
	 */
	void AddPngFormat(CClip& clip, CImage& image);

private:
	// DoImmediateRender's multi-clip text paste: adds the clips' text, unicode text and, unless the
	// paste is limited to text, HDROP, RTF and HTML, each joined with the multi-paste separator
	void AggregateTextFormats(CClip& clip);

	/**
	 * @brief DoImmediateRender's multi-clip step: joins the clips' images, or else their text formats.
	 * @param clip Receives the joined formats.
	 */
	void AggregateClips(CClip& clip);

	/** @brief A special paste: the option that selects it and the transform it applies. */
	struct SpecialPaste
	{
		/** @brief The paste option that selects this special paste. */
		bool CSpecialPasteOptions::* option{};
		/** @brief The transform; null when applyChecked is used. */
		void (COleClipSource::* apply)(CClip&) = nullptr;
		/** @brief A transform that can fail (returns false); null when apply is used. */
		bool (COleClipSource::* applyChecked)(CClip&) = nullptr;
	};

	/** @brief The special pastes in order of precedence: the first selected one is applied. */
	static const std::array<SpecialPaste, 17> s_specialPastes;

	/**
	 * @brief DoImmediateRender's special-paste step: applies the first selected special paste.
	 * @param clip The clip to transform.
	 * @return False when the special paste failed (reported); the paste then stops.
	 */
	bool ApplySpecialPaste(CClip& clip);

	/**
	 * @brief The upper or lower case special paste (upper when m_pasteUpperCase is set).
	 * @param clip The clip to transform.
	 */
	void ApplyUpperLowerCase(CClip& clip);

	/**
	 * @brief The "add one line feed" special paste.
	 * @param clip The clip to transform.
	 */
	void AddOneLineFeed(CClip& clip);

	/**
	 * @brief The "add two line feeds" special paste.
	 * @param clip The clip to transform.
	 */
	void AddTwoLineFeeds(CClip& clip);

	/**
	 * @brief The GUID special paste; a failed GUID creation is reported.
	 * @param clip The clip whose formats are replaced by a new GUID.
	 * @return False when no GUID could be created.
	 */
	bool PutGuidOntoClipboardOrReport(CClip& clip);

	/** @brief What PlainTextFilter finds in a clip's formats. */
	struct PlainTextScan
	{
		/** @brief Whether the clip has CF_TEXT or CF_UNICODETEXT. */
		bool foundText{};
		/** @brief The index of the (last) CF_HDROP format; -1 when absent. */
		INT_PTR hDropIndex{ -1 };
	};

	/**
	 * @brief PlainTextFilter's scan: looks for text formats and the CF_HDROP format.
	 * @param clip The clip.
	 * @return What was found.
	 */
	static PlainTextScan ScanForTextAndHDrop(CClip& clip);

	/**
	 * @brief Whether a delay render request comes within the lockout time after the paste started.
	 * @return True when the request is to be refused.
	 */
	bool IsDelayRenderLockedOut() const;

	/**
	 * @brief OnRenderGlobalData's first render of a format: renders the clips and caches a copy.
	 * @param cfFormat The requested format.
	 * @param hData Receives the rendered data (null when there is none).
	 * @return False when the request is refused (lockout) or the render failed (reported).
	 */
	bool RenderAndCache(CLIPFORMAT cfFormat, HGLOBAL& hData);

	/**
	 * @brief OnRenderGlobalData's hand-over: gives the rendered data to the caller's global, or copies it into it.
	 * @param hData The rendered data; owned by the caller's global or freed.
	 * @param phGlobal The caller's global.
	 * @return TRUE when there was data to hand over.
	 */
	static BOOL HandOverRenderedData(HGLOBAL hData, HGLOBAL* phGlobal);

	/** @brief The files ConvertToFileDrop writes for a drop. */
	struct DragFiles
	{
		/** @brief The drag-files folder, with a trailing backslash. */
		CString folder{};
		/** @brief The user's file name for the dropped files; empty for numbered names. */
		CString customName{};
		/** @brief The number of the next numbered file. */
		int nextId{};
		/** @brief The paths of the written files. */
		std::vector<std::wstring> paths{};
	};

	/**
	 * @brief The path of the next drag file: the custom name, or the default name numbered with nextId (then incremented).
	 * @param drag The drag files.
	 * @param defaultName The name without a custom name ("text", "image").
	 * @param extension The file extension, without a period.
	 * @return The file path.
	 */
	static CString NextDragFilePath(DragFiles& drag, const TCHAR* defaultName, const TCHAR* extension);

	/**
	 * @brief Writes one clip as a drag file: its unicode text, else its text, else its image.
	 * @param fileClip The clip, loaded with its formats.
	 * @param drag The drag files; gets the written file's path.
	 */
	static void AddDragFile(CClip& fileClip, DragFiles& drag);
};
#include "stdafx.h"
#include "CP_Main.h"
#include "OleClipSource.h"
#include "..\Shared\TextConvert.h"
#include "CF_HDropAggregator.h"
#include "CF_UnicodeTextAggregator.h"
#include "CF_TextAggregator.h"
#include "richtextaggregator.h"
#include "htmlformataggregator.h"
#include "ErrorReport.h"
#include "ClipboardFormatError.h"
#include "FileDataRecord.h"
#include "GlobalBytes.h"
#include "Path.h"
#include "Md5.h"
#include "RandomRange.h"
#include "RtfTransforms.h"
#include "Slugifier.h"
#include "TextTransforms.h"
#include "Typoglycemia.h"
#include "ImageFormatAggregator.h"
#include "BitmapHelper.h"
#include "Misc.h"
#include <string>
#include <vector>

/*------------------------------------------------------------------*\
COleClipSource
\*------------------------------------------------------------------*/
//IMPLEMENT_DYNAMIC(COleClipSource, COleDataSource)
COleClipSource::COleClipSource() :
	m_caseMapper(Services().IcuString()),
	m_cases(m_caseMapper)
{
	m_bLoadedFormats = false;
	m_convertToHDROPOnDelayRender = false;
}

COleClipSource::~COleClipSource()
{

}

CGetSetOptions& COleClipSource::Settings() const
{
	return theApp.Services().Settings();
}

CAppServices& COleClipSource::Services()
{
	return theApp.Services();
}

BOOL COleClipSource::DoDelayRender()
{
	CClipTypes types;
	m_ClipIDs.GetTypes(Services().ClipContext(), types);

	bool foundHDrop = false;

	INT_PTR count = types.GetSize();
	for(int i=0; i < count; i++)
	{
		if (m_pasteOptions.m_dragDropFilesOnly)
		{
			if (types[i] == CF_HDROP)
			{
				DelayRenderData(types[i]);
			}
		}
		else
		{
			DelayRenderData(types[i]);
		}

		if (types[i] == CF_HDROP)
		{
			foundHDrop = true;
		}
	}

	if (m_pasteOptions.m_placeCF_HDROP_OnDrag &&
		foundHDrop == false)
	{
		DelayRenderData(CF_HDROP);
		m_convertToHDROPOnDelayRender = true;
	}

	return count > 0;
}

BOOL COleClipSource::DoImmediateRender()
{
	if(m_bLoadedFormats)
		return TRUE;

	m_bLoadedFormats = true;

	if(m_pasteOptions.m_pPasteFormats != NULL)
	{
		return PutFormatOnClipboard(m_pasteOptions.m_pPasteFormats) > 0;
	}

	INT_PTR count = m_ClipIDs.GetSize();
	if(count <= 0)
		return 0;

	CClip clip(Services().ClipContext());

	if(count > 1)
	{
		AggregateClips(clip);
	}

	if (count >= 1 && clip.m_Formats.GetCount() == 0)
	{
		clip.LoadFormats(m_ClipIDs[0], m_pasteOptions.LimitFormatsToText(), m_pasteOptions.IncludeRTFForTextOnly());
	}

	if (m_pasteOptions.LimitFormatsToText())
	{
		PlainTextFilter(clip);
	}

	if (!ApplySpecialPaste(clip))
	{
		return FALSE;
	}

	SaveDittoFileDataToFile(clip);

	return PutFormatOnClipboard(&clip.m_Formats) > 0;
}

void COleClipSource::AggregateClips(CClip& clip)
{
	if (m_pasteOptions.m_pasteImagesHorizontal ||
		m_pasteOptions.m_pasteImagesVertically)
	{
		CImageFormatAggregator bigImage(m_pasteOptions.m_pasteImagesHorizontal, Services().ClipboardFormats().Png());
		if (m_ClipIDs.AggregateData(Services().ClipContext(), bigImage, CF_DIB, Settings().m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
		{
			CClipFormat cf(CF_DIB, bigImage.GetHGlobal());
			clip.m_Formats.Add(cf);
			//clip.m_Formats now owns the global data
			cf.m_autoDeleteData = false;
		}
	}
	else
	{
		AggregateTextFormats(clip);
	}
}

// upper and lower case share one transform, which picks upper when m_pasteUpperCase is set
const std::array<COleClipSource::SpecialPaste, 17> COleClipSource::s_specialPastes{ {
	{ &CSpecialPasteOptions::m_pasteUpperCase, &COleClipSource::ApplyUpperLowerCase, nullptr },
	{ &CSpecialPasteOptions::m_pasteLowerCase, &COleClipSource::ApplyUpperLowerCase, nullptr },
	{ &CSpecialPasteOptions::m_pasteCapitalize, &COleClipSource::Capitalize, nullptr },
	{ &CSpecialPasteOptions::m_pasteSentenceCase, &COleClipSource::SentenceCase, nullptr },
	{ &CSpecialPasteOptions::m_pasteRemoveLineFeeds, &COleClipSource::RemoveLineFeeds, nullptr },
	{ &CSpecialPasteOptions::m_pasteAddOneLineFeed, &COleClipSource::AddOneLineFeed, nullptr },
	{ &CSpecialPasteOptions::m_pasteAddTwoLineFeeds, &COleClipSource::AddTwoLineFeeds, nullptr },
	{ &CSpecialPasteOptions::m_pasteTypoglycemia, &COleClipSource::Typoglycemia, nullptr },
	{ &CSpecialPasteOptions::m_pasteAddingDateTime, &COleClipSource::AddDateTime, nullptr },
	{ &CSpecialPasteOptions::m_trimWhiteSpace, &COleClipSource::TrimWhiteSpace, nullptr },
	{ &CSpecialPasteOptions::m_PosixifyPaths, &COleClipSource::PosixifyPaths, nullptr },
	{ &CSpecialPasteOptions::m_pasteSlugify, &COleClipSource::Slugify, nullptr },
	{ &CSpecialPasteOptions::m_invertCase, &COleClipSource::InvertCase, nullptr },
	{ &CSpecialPasteOptions::m_pasteCamelCase, &COleClipSource::CamelCase, nullptr },
	{ &CSpecialPasteOptions::m_pasteAsciiOnly, &COleClipSource::AsciiOnly, nullptr },
	{ &CSpecialPasteOptions::m_pasteGuid, nullptr, &COleClipSource::PutGuidOntoClipboardOrReport },
	{ &CSpecialPasteOptions::m_pasteAsImage, &COleClipSource::PasteAsImage, nullptr },
} };

bool COleClipSource::ApplySpecialPaste(CClip& clip)
{
	for (const SpecialPaste& paste : s_specialPastes)
	{
		if (m_pasteOptions.*paste.option)
		{
			if (paste.applyChecked != nullptr)
			{
				return (this->*paste.applyChecked)(clip);
			}
			(this->*paste.apply)(clip);
			return true;
		}
	}
	return true;
}

void COleClipSource::ApplyUpperLowerCase(CClip& clip)
{
	DoUpperLowerCase(clip, m_pasteOptions.m_pasteUpperCase);
}

void COleClipSource::AddOneLineFeed(CClip& clip)
{
	AddLineFeeds(clip, 1);
}

void COleClipSource::AddTwoLineFeeds(CClip& clip)
{
	AddLineFeeds(clip, 2);
}

bool COleClipSource::PutGuidOntoClipboardOrReport(CClip& clip)
{
	try
	{
		PutGuidOntoClipboard(clip);
	}
	catch (const std::runtime_error& e)
	{
		// CStringUtil::NewGuidString() throws it when CoCreateGuid fails; the paste stops
		CErrorReport::Show(CStringUtil::Format(_T("Pasting a new GUID failed: %s"), CString(e.what()).GetString()));
		return false;
	}
	return true;
}

void COleClipSource::AggregateTextFormats(CClip& clip)
{
	CStringA SepA = CTextConvert::UnicodeToAnsi(Settings().GetMultiPasteSeparator());
	CCF_TextAggregator CFText(SepA);
	if (m_ClipIDs.AggregateData(Services().ClipContext(), CFText, CF_TEXT, Settings().m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
	{
		CClipFormat cf(CF_TEXT, CFText.GetHGlobal());
		clip.m_Formats.Add(cf);
		//clip.m_Formats now owns the global data
		cf.m_autoDeleteData = false;
	}

	CStringW SepW = Settings().GetMultiPasteSeparator();
	CCF_UnicodeTextAggregator CFUnicodeText(SepW);
	if (m_ClipIDs.AggregateData(Services().ClipContext(), CFUnicodeText, CF_UNICODETEXT, Settings().m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
	{
		CClipFormat cf(CF_UNICODETEXT, CFUnicodeText.GetHGlobal());
		clip.m_Formats.Add(cf);
		//clip.m_Formats now owns the global data
		cf.m_autoDeleteData = false;
	}

	if (m_pasteOptions.LimitFormatsToText() == false)
	{
		CCF_HDropAggregator HDrop;
		if (m_ClipIDs.AggregateData(Services().ClipContext(), HDrop, CF_HDROP, Settings().m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
		{
			CClipFormat cf(CF_HDROP, HDrop.GetHGlobal());
			clip.m_Formats.Add(cf);
			//clip.m_Formats now owns the global data
			cf.m_autoDeleteData = false;
		}

		CRichTextAggregator RichText(SepW);
		if (m_ClipIDs.AggregateData(Services().ClipContext(), RichText, Services().ClipboardFormats().Rtf(), Settings().m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
		{
			CClipFormat cf(Services().ClipboardFormats().Rtf(), RichText.GetHGlobal());
			clip.m_Formats.Add(cf);
			//clip.m_Formats now owns the global data
			cf.m_autoDeleteData = false;
		}

		CHTMLFormatAggregator Html(SepW);
		if (m_ClipIDs.AggregateData(Services().ClipContext(), Html, Services().ClipboardFormats().Html(), Settings().m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
		{
			CClipFormat cf(Services().ClipboardFormats().Html(), Html.GetHGlobal());
			clip.m_Formats.Add(cf);
			//clip.m_Formats now owns the global data
			cf.m_autoDeleteData = false;
		}
	}
}

void COleClipSource::TransformText(CClip &clip, const std::function<std::wstring(std::wstring_view)>& transform)
{
	IClipFormat *unicodeText = clip.m_Formats.FindFormatEx(CF_UNICODETEXT);
	IClipFormat *ansiText = clip.m_Formats.FindFormatEx(CF_TEXT);
	CString source;
	if (unicodeText != NULL)
	{
		source = unicodeText->GetAsCString();
	}
	else if (ansiText != NULL)
	{
		source = CTextConvert::AnsiToUnicode(ansiText->GetAsCStringA());
	}
	else
	{
		return;
	}

	// the text is changed once and CF_TEXT is written from the result, so both formats match;
	// upstream changed each format on its own, and several transforms broke or skipped CF_TEXT
	const std::wstring result = transform(std::wstring_view(source.GetString(), source.GetLength()));
	if (unicodeText != NULL)
	{
		unicodeText->Free();
		unicodeText->Data(CGlobalMemory::NewGlobalP(const_cast<wchar_t*>(result.c_str()), (result.size() + 1) * sizeof(wchar_t)));
	}
	if (ansiText != NULL)
	{
		const CStringA ansi = CTextConvert::UnicodeToAnsi(CString(result.c_str(), static_cast<int>(result.size())));
		ansiText->Free();
		ansiText->Data(CGlobalMemory::NewGlobalP(const_cast<char*>(ansi.GetString()), ansi.GetLength() + 1));
	}
}

void COleClipSource::TransformRtf(CClip &clip, const std::function<std::string(std::string_view)>& transform)
{
	IClipFormat *rtf = clip.m_Formats.FindFormatEx(Services().ClipboardFormats().Rtf());
	if (rtf == NULL)
	{
		return;
	}
	const CStringA source = rtf->GetAsCStringA();
	const std::string result = transform(std::string_view(source.GetString(), source.GetLength()));
	rtf->Free();
	rtf->Data(CGlobalMemory::NewGlobalP(const_cast<char*>(result.c_str()), result.size() + 1));
}

void COleClipSource::DoUpperLowerCase(CClip &clip, bool upper)
{
	TransformText(clip, [this, upper](std::wstring_view text) { return upper ? m_cases.Upper(text) : m_cases.Lower(text); });
}

void COleClipSource::InvertCase(CClip &clip)
{
	TransformText(clip, [this](std::wstring_view text) { return m_cases.InvertCase(text); });
}

void COleClipSource::CamelCase(CClip& clip)
{
	TransformText(clip, [this](std::wstring_view text) { return m_cases.CamelCase(text); });
}

void COleClipSource::Capitalize(CClip &clip)
{
	TransformText(clip, [this](std::wstring_view text) { return m_cases.Capitalize(text); });
}

void COleClipSource::SentenceCase(CClip &clip)
{
	TransformText(clip, [this](std::wstring_view text) { return m_cases.SentenceCase(text); });
}

void COleClipSource::AsciiOnly(CClip& clip)
{
	TransformText(clip, &DittoCore::TextTransforms::AsciiOnly);
}

COleClipSource::PlainTextScan COleClipSource::ScanForTextAndHDrop(CClip& clip)
{
	PlainTextScan scan{};
	INT_PTR	count = clip.m_Formats.GetCount();
	for (INT_PTR i = 0; i < count; i++)
	{
		CClipFormat *pCF = &clip.m_Formats.ElementAt(i);

		if (pCF->m_cfType == CF_TEXT ||
			pCF->m_cfType == CF_UNICODETEXT)
		{
			scan.foundText = true;
		}
		else if (pCF->m_cfType == CF_HDROP)
		{
			scan.hDropIndex = i;
		}
	}
	return scan;
}

void COleClipSource::PlainTextFilter(CClip &clip)
{
	const PlainTextScan scan{ ScanForTextAndHDrop(clip) };
	const bool foundText{ scan.foundText };
	const INT_PTR hDropIndex{ scan.hDropIndex };

	if (foundText &&
		hDropIndex > -1)
	{
		clip.m_Formats.RemoveAt(hDropIndex);
	}
	else if (foundText == false &&
		hDropIndex > -1)
	{
		CCF_HDropAggregator HDrop;
		if (m_ClipIDs.AggregateData(Services().ClipContext(), HDrop, CF_HDROP, Settings().m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
		{
			clip.m_Formats.RemoveAt(hDropIndex);

			CClipFormat format(CF_UNICODETEXT, HDrop.GetHGlobalAsString());
			clip.m_Formats.Add(format);
			format.m_autoDeleteData = false; //owned by m_DelayRenderedFormats			
		}
	}
}

void COleClipSource::RemoveLineFeeds(CClip &clip)
{
	TransformText(clip, &DittoCore::TextTransforms::RemoveLineFeeds);
	TransformRtf(clip, &DittoCore::RtfTransforms::RemoveLineFeeds);
}

void COleClipSource::AddLineFeeds(CClip &clip, int count)
{
	TransformText(clip, [count](std::wstring_view text) { return DittoCore::TextTransforms::AddLineFeeds(text, count); });
	TransformRtf(clip, [count](std::string_view rtf) { return DittoCore::RtfTransforms::AddLineFeeds(rtf, count); });
}

void COleClipSource::AddDateTime(CClip &clip)
{
	const CString now = COleDateTime::GetCurrentTime().Format();
	const std::wstring_view time(now.GetString(), now.GetLength());
	TransformText(clip, [time](std::wstring_view text) { return DittoCore::TextTransforms::AddDateTime(text, time); });
	TransformRtf(clip, [time](std::string_view rtf) { return DittoCore::RtfTransforms::AddDateTime(rtf, time); });
}

void COleClipSource::TrimWhiteSpace(CClip &clip)
{
	TransformText(clip, &DittoCore::TextTransforms::Trim);
}

void COleClipSource::PosixifyPaths(CClip& clip)
{
	TransformText(clip, &DittoCore::TextTransforms::PosixifyPaths);
}

bool COleClipSource::SaveFileDataRecord(HGLOBAL record, std::vector<std::wstring>& dropFiles)
{
	const DittoCore::GlobalBytes block(record);
	const std::vector<DittoCore::FileDataEntry> files = DittoCore::FileDataRecord::Parse(block.Bytes());
	const CString folder = theApp.Services().Settings().GetPath(CGetSetOptions::PathDragFiles);
	std::set<CString> usedNames;
	for (const DittoCore::FileDataEntry& file : files)
	{
		const CString originalPath = CTextConvert::Utf8ToUnicode(CStringA(file.path.data(), static_cast<int>(file.path.size())));
		const std::string md5{ DittoCore::Md5::Hex(file.data) };
		if (md5 != file.md5)
		{
			throw DittoCore::ClipboardFormatError("the saved contents of " + std::string(file.path) + " fail their MD5 check");
		}

		const CString newFilePath = folder + UniqueFileName(originalPath, usedNames);
		CLogger::Log(CStringUtil::Format(_T("Saving file contents from Ditto, original file: %s, size: %Iu, md5: %S, to: %s"), originalPath.GetString(), file.data.size(), md5.c_str(), newFilePath.GetString()));

		// the constructor throws CFileException when the file cannot be created; the paste stops
		CFile target(newFilePath, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary);
		target.Write(file.data.data(), static_cast<UINT>(file.data.size()));
		target.Close();
		dropFiles.push_back(newFilePath.GetString());
	}
	return !files.empty();
}

CString COleClipSource::UniqueFileName(const CString& originalPath, std::set<CString>& usedNames)
{
	using namespace nsPath;
	CPath path(originalPath);
	const CString name = path.GetName();
	CString candidate = name;
	// two copied files may share a name (from different folders); number the later ones
	for (int n = 2; usedNames.count(CString(candidate).MakeLower()) > 0; n++)
	{
		CPath numbered(originalPath);
		const CString extension = numbered.GetExtension();
		numbered.RemoveExtension();
		candidate = CStringUtil::Format(_T("%s (%d)%s%s"), numbered.GetName().GetString(), n, extension.IsEmpty() ? _T("") : _T("."), extension.GetString());
	}
	usedNames.insert(CString(candidate).MakeLower());
	return candidate;
}

void COleClipSource::SaveDittoFileDataToFile(CClip &clip)
{
	std::vector<std::wstring> hDrpData;
	CClipFormat* pCF;
	int hDropIndex = -1;
	bool savedFile = false;
	INT_PTR	count = clip.m_Formats.GetSize();
	for (int i = 0; i < count; i++)
	{
		pCF = &clip.m_Formats.ElementAt(i);

		if (pCF->m_cfType == Services().ClipboardFormats().DittoFileData())
		{
			savedFile = SaveFileDataRecord(pCF->m_hgData, hDrpData) || savedFile;
		}
		else if (pCF->m_cfType == CF_HDROP)
		{
			hDropIndex = i;
		}
	}

	if (savedFile)
	{
		if (hDropIndex >= 0)
		{
			clip.m_Formats.RemoveAt(hDropIndex);
		}

		CClipFormat cf(CF_HDROP, CCF_HDropAggregator::NewDropBlock(hDrpData));
		clip.m_Formats.Add(cf);

		//clip.m_Formats now owns the global data
		cf.m_autoDeleteData = false;
	}
}

void COleClipSource::Typoglycemia(CClip &clip)
{
	CRandomRange random;
	TransformText(clip, [&random](std::wstring_view text) { return DittoCore::Typoglycemia::Scramble(text, random); });
}

INT_PTR COleClipSource::PutFormatOnClipboard(CClipFormats *pFormats)
{
	CLogger::Log(_T("Start of put format on clipboard"));

	CClipFormat* pCF;
	INT_PTR	count = pFormats->GetSize();
	INT_PTR i = 0;

	for(i = 0; i < count; i++)
	{
		pCF = &pFormats->ElementAt(i);

		CLogger::Log(CStringUtil::Format(_T("Setting clipboard type: %s to the clipboard"), CClipboardFormats::GetFormatName(pCF->m_cfType).GetString()));

		CacheGlobalData(pCF->m_cfType, pCF->m_hgData);
		pCF->m_hgData = 0; // OLE owns it now
	}

	pFormats->RemoveAll();

	m_bLoadedFormats = true;

	CLogger::Log(_T("End of put format on clipboard"));

	return count;
}

// Renders the pasted clips in the given format. OnRenderGlobalData is a COM callback that no
// exception may leave: a malformed clip or a database error is shown to the user and gives no value.
std::optional<HGLOBAL> COleClipSource::RenderClipsOrReport(CLIPFORMAT format)
{
	try
	{
		HGLOBAL hData = m_ClipIDs.Render(Services().ClipContext(), format);
		// image and text clips are dropped as files written for the drop
		if (m_convertToHDROPOnDelayRender &&
			hData == NULL &&
			format == CF_HDROP)
		{
			hData = ConvertToFileDrop();
		}
		return hData;
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Ditto could not provide %s for the paste: the clip's data is malformed (%s)."),
			CClipboardFormats::GetFormatName(format).GetString(), CString(error.what()).GetString()));
	}
	catch (CppSQLite3Exception& error)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Ditto could not provide %s for the paste: database error %d (%s)."),
			CClipboardFormats::GetFormatName(format).GetString(), error.errorCode(), error.errorMessage()));
	}
	return std::nullopt;
}

BOOL COleClipSource::OnRenderGlobalData(LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal)
{
	if(m_inRenderGlobalData)
	{
		return FALSE;
	}
	m_inRenderGlobalData = true;

	HGLOBAL hData = NULL;

	CClipFormat *pFind = m_DelayRenderedFormats.FindFormat(lpFormatEtc->cfFormat);

	if(pFind)
	{
		if(pFind->m_hgData)
		{
			hData = CGlobalMemory::NewGlobalH(pFind->m_hgData, GlobalSize(pFind->m_hgData));
		}
	}
	else if (!RenderAndCache(lpFormatEtc->cfFormat, hData))
	{
		// FALSE tells the target the render failed
		m_inRenderGlobalData = false;
		return FALSE;
	}

	BOOL bRet = HandOverRenderedData(hData, phGlobal);

	m_inRenderGlobalData = false;

	return bRet;
}

bool COleClipSource::RenderAndCache(CLIPFORMAT cfFormat, HGLOBAL& hData)
{
	if(m_ClipIDs.GetCount() > 0)
	{
		const std::optional<HGLOBAL> rendered = RenderClipsOrReport(cfFormat);
		if (!rendered)
		{
			return false;
		}
		hData = *rendered;
	}

	//Add to a cache of already rendered data
	//Windows seems to call this function multiple times
	//so only the first time do we need to go get the data
	HGLOBAL hCopy = NULL;
	if(hData)
	{
		hCopy = CGlobalMemory::NewGlobalH(hData, GlobalSize(hData));
	}

	CClipFormat format(cfFormat, hCopy);
	m_DelayRenderedFormats.Add(format);
	format.m_autoDeleteData = false; //owned by m_DelayRenderedFormats

	return true;
}

BOOL COleClipSource::HandOverRenderedData(HGLOBAL hData, HGLOBAL* phGlobal)
{
	BOOL bRet = FALSE;
	if(hData)
	{
		// if phGlobal is null, we can just give the allocated mem
		// else, our data must fit within the GlobalSize(*phGlobal)
		if(*phGlobal == 0)
		{
			*phGlobal = hData;
		}
		else
		{
			SIZE_T len = min(::GlobalSize(*phGlobal), ::GlobalSize(hData));
			if(len)
			{
				CGlobalMemory::CopyToGlobalHH(*phGlobal, hData, len);
			}
			::GlobalFree(hData);
		}
		bRet = TRUE;
	}
	return bRet;
}

HGLOBAL COleClipSource::ConvertToFileDrop()
{
	DragFiles drag{};
	drag.folder = Settings().GetPath(CGetSetOptions::PathDragFiles);
	CreateDirectory(drag.folder, NULL);

	drag.nextId = Settings().GetDragId();
	int origDragId = drag.nextId;

	drag.customName = Settings().GetTempDragFileName();
	if (drag.customName != _T(""))
	{
		drag.nextId = 1;
	}

	for (int i = 0; i < m_ClipIDs.GetCount(); i++)
	{
		CClip fileClip(Services().ClipContext());
		fileClip.LoadFormats(m_ClipIDs[i]);

		AddDragFile(fileClip, drag);
	}

	if(drag.customName == _T("") &&
		drag.nextId != origDragId)
	{
		Settings().SetDragId(drag.nextId);
	}

	HGLOBAL hData = CCF_HDropAggregator::NewDropBlock(drag.paths);

	return hData;
}

CString COleClipSource::NextDragFilePath(DragFiles& drag, const TCHAR* defaultName, const TCHAR* extension)
{
	CString name = defaultName;
	CString file;
	if (drag.customName != _T(""))
	{
		name = drag.customName;
		file.Format(_T("%s%s.%s"), drag.folder.GetString(), name.GetString(), extension);
	}
	else
	{
		file.Format(_T("%s%s_%d.%s"), drag.folder.GetString(), name.GetString(), drag.nextId++, extension);
	}
	return file;
}

void COleClipSource::AddDragFile(CClip& fileClip, DragFiles& drag)
{
	CClipFormat *unicodeText = fileClip.m_Formats.FindFormat(CF_UNICODETEXT);
	CClipFormat *asciiText = fileClip.m_Formats.FindFormat(CF_TEXT);
	if (unicodeText || asciiText)
	{
		CString file = NextDragFilePath(drag, _T("text"), _T("txt"));

		// a file that was not written is not dropped; upstream dropped it empty or missing
		if (fileClip.WriteTextToFile(file, unicodeText != nullptr, unicodeText == nullptr, FALSE) == FALSE)
		{
			CErrorReport::Show(CStringUtil::Format(_T("Writing the dragged clip text to %s failed."), file.GetString()));
			return;
		}
		drag.paths.push_back(file.GetString());
		return;
	}

	CClipFormat *png = fileClip.m_Formats.FindFormat(Services().ClipboardFormats().Png());
	CClipFormat *bitmap = fileClip.m_Formats.FindFormat(CF_DIB);
	if (bitmap != NULL ||
		png != NULL)
	{
		CString file = NextDragFilePath(drag, _T("image"), _T("png"));

		if (fileClip.WriteImageToFile(file))
		{
			drag.paths.push_back(file.GetString());
		}
	}
}

void COleClipSource::Slugify(CClip &clip)
{
	const CString separator = Settings().GetSlugifySeparator();
	const std::wstring_view separatorView(separator.GetString(), separator.GetLength());
	TransformText(clip, [separatorView](std::wstring_view text) { return DittoCore::Slugifier::Slugify(text, separatorView); });
}

void COleClipSource::PutGuidOntoClipboard(CClip& clip)
{
	CLogger::Log(_T("Start of put Guid on clipboard"));

	clip.m_Formats.RemoveAll();

	CString guid = CStringUtil::NewGuidString();

	long len = guid.GetLength();
	HGLOBAL hGlobal = CGlobalMemory::NewGlobalP(guid.GetBuffer(), ((len + 1) * sizeof(wchar_t)));

	CClipFormat cf(CF_UNICODETEXT, hGlobal);
	clip.m_Formats.Add(cf);

	//clip.m_Formats now owns the global data
	cf.m_autoDeleteData = false;

	CLogger::Log(_T("End of put Guid on clipboard"));
}

void COleClipSource::PasteAsImage(CClip& clip)
{
	CLogger::Log(_T("Start of PasteAsImage"));

	IClipFormat* pUnicodeText = clip.m_Formats.FindFormatEx(CF_UNICODETEXT);
	if (pUnicodeText == NULL)
	{
		CLogger::Log(_T("PasteAsImage - no unicode text found"));
		return;
	}

	CString path = pUnicodeText->GetAsCString();
	path.Trim();

	if (path.IsEmpty() || !PathFileExists(path))
	{
		CLogger::Log(CStringUtil::Format(_T("PasteAsImage - path not found: %s"), path.GetString()));
		return;
	}

	CImage image;
	HRESULT hr = image.Load(path);
	if (FAILED(hr))
	{
		CLogger::Log(CStringUtil::Format(_T("PasteAsImage - failed to load image: %s"), path.GetString()));
		return;
	}

	HBITMAP hBitmap = (HBITMAP)image;
	HPALETTE hPal = NULL;
	HANDLE hDib = CBitmapHelper::hBitmapToDIB(hBitmap, BI_RGB, hPal);
	if (hDib == NULL)
	{
		CLogger::Log(_T("PasteAsImage - failed to convert to DIB"));
		return;
	}

	clip.m_Formats.RemoveAll();

	CClipFormat cfDib(CF_DIB, (HGLOBAL)hDib);
	clip.m_Formats.Add(cfDib);
	cfDib.m_autoDeleteData = false;

	AddPngFormat(clip, image);

	CLogger::Log(_T("End of PasteAsImage"));
}

void COleClipSource::AddPngFormat(CClip& clip, CImage& image)
{
	CComPtr<IStream> pStream{};
	if (FAILED(CreateStreamOnHGlobal(nullptr, TRUE, &pStream)) || FAILED(image.Save(pStream, Gdiplus::ImageFormatPNG)))
	{
		return;
	}

	LARGE_INTEGER liZero = { 0 };
	ULARGE_INTEGER ulSize{};
	pStream->Seek({ 0 }, STREAM_SEEK_END, &ulSize);
	pStream->Seek(liZero, STREAM_SEEK_SET, nullptr);

	HGLOBAL hPng = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)ulSize.QuadPart);
	if (hPng == nullptr)
	{
		return;
	}

	LPVOID pDst = GlobalLock(hPng);
	if (pDst == nullptr)
	{
		// a block that cannot be locked holds no PNG: the clip keeps only its DIB, as when the allocation fails
		CLogger::Log(_T("PasteAsImage - failed to lock the PNG block"));
		GlobalFree(hPng);
		return;
	}
	pStream->Read(pDst, (ULONG)ulSize.QuadPart, nullptr);
	GlobalUnlock(hPng);

	CClipFormat cfPng(Services().ClipboardFormats().Png(), hPng);
	clip.m_Formats.Add(cfPng);
	cfPng.m_autoDeleteData = false;
}
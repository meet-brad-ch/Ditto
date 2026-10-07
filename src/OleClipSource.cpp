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
	m_caseMapper(theApp.m_icuString),
	m_cases(m_caseMapper)
{
	m_bLoadedFormats = false;
	m_convertToHDROPOnDelayRender = false;
}

COleClipSource::~COleClipSource()
{

}

BOOL COleClipSource::DoDelayRender()
{
	CClipTypes types;
	m_ClipIDs.GetTypes(types);

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

	CClip clip;

	if(count > 1)
	{
		if (m_pasteOptions.m_pasteImagesHorizontal ||
			m_pasteOptions.m_pasteImagesVertically)
		{
			CImageFormatAggregator bigImage(m_pasteOptions.m_pasteImagesHorizontal);
			if (m_ClipIDs.AggregateData(bigImage, CF_DIB, CGetSetOptions::m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
			{
				CClipFormat cf(CF_DIB, bigImage.GetHGlobal());
				clip.m_Formats.Add(cf);
				//clip.m_Formats now owns the global data
				cf.m_autoDeleteData = false;
			}
		}
		else
		{
			CStringA SepA = CTextConvert::UnicodeToAnsi(CGetSetOptions::GetMultiPasteSeparator());
			CCF_TextAggregator CFText(SepA);
			if (m_ClipIDs.AggregateData(CFText, CF_TEXT, CGetSetOptions::m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
			{
				CClipFormat cf(CF_TEXT, CFText.GetHGlobal());
				clip.m_Formats.Add(cf);
				//clip.m_Formats now owns the global data
				cf.m_autoDeleteData = false;
			}

			CStringW SepW = CGetSetOptions::GetMultiPasteSeparator();
			CCF_UnicodeTextAggregator CFUnicodeText(SepW);
			if (m_ClipIDs.AggregateData(CFUnicodeText, CF_UNICODETEXT, CGetSetOptions::m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
			{
				CClipFormat cf(CF_UNICODETEXT, CFUnicodeText.GetHGlobal());
				clip.m_Formats.Add(cf);
				//clip.m_Formats now owns the global data
				cf.m_autoDeleteData = false;
			}

			if (m_pasteOptions.LimitFormatsToText() == false)
			{
				CCF_HDropAggregator HDrop;
				if (m_ClipIDs.AggregateData(HDrop, CF_HDROP, CGetSetOptions::m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
				{
					CClipFormat cf(CF_HDROP, HDrop.GetHGlobal());
					clip.m_Formats.Add(cf);
					//clip.m_Formats now owns the global data
					cf.m_autoDeleteData = false;
				}

				CRichTextAggregator RichText(SepW);
				if (m_ClipIDs.AggregateData(RichText, theApp.m_RTFFormat, CGetSetOptions::m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
				{
					CClipFormat cf(theApp.m_RTFFormat, RichText.GetHGlobal());
					clip.m_Formats.Add(cf);
					//clip.m_Formats now owns the global data
					cf.m_autoDeleteData = false;
				}

				CHTMLFormatAggregator Html(SepW);
				if (m_ClipIDs.AggregateData(Html, theApp.m_HTML_Format, CGetSetOptions::m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
				{
					CClipFormat cf(theApp.m_HTML_Format, Html.GetHGlobal());
					clip.m_Formats.Add(cf);
					//clip.m_Formats now owns the global data
					cf.m_autoDeleteData = false;
				}
			}
		}
	}

	if (count >= 1 && clip.m_Formats.GetCount() == 0)
	{
		clip.LoadFormats(m_ClipIDs[0], m_pasteOptions.LimitFormatsToText(), m_pasteOptions.IncludeRTFForTextOnly());
	}

	if (m_pasteOptions.LimitFormatsToText())
	{
		PlainTextFilter(clip);
	}

	if(m_pasteOptions.m_pasteUpperCase ||
		m_pasteOptions.m_pasteLowerCase)
	{
		DoUpperLowerCase(clip, m_pasteOptions.m_pasteUpperCase);
	}
	else if(m_pasteOptions.m_pasteCapitalize)
	{
		Capitalize(clip);
	}
	else if(m_pasteOptions.m_pasteSentenceCase)
	{
		SentenceCase(clip);
	}
	else if(m_pasteOptions.m_pasteRemoveLineFeeds)
	{
		RemoveLineFeeds(clip);
	}
	else if(m_pasteOptions.m_pasteAddOneLineFeed)
	{
		AddLineFeeds(clip, 1);
	}
	else if (m_pasteOptions.m_pasteAddTwoLineFeeds)
	{
		AddLineFeeds(clip, 2);
	}
	else if (m_pasteOptions.m_pasteTypoglycemia)
	{
		Typoglycemia(clip);
	}
	else if (m_pasteOptions.m_pasteAddingDateTime)
	{
		AddDateTime(clip);
	}
	else if (m_pasteOptions.m_trimWhiteSpace)
	{
		TrimWhiteSpace(clip);
	}
	else if (m_pasteOptions.m_PosixifyPaths)
	{
		PosixifyPaths(clip);
	}
	else if (m_pasteOptions.m_pasteSlugify)
	{
		Slugify(clip);
	}
	else if (m_pasteOptions.m_invertCase)
	{
		InvertCase(clip);
	}
	else if (m_pasteOptions.m_pasteCamelCase)
	{
		CamelCase(clip);
	}
	else if (m_pasteOptions.m_pasteAsciiOnly)
	{
		AsciiOnly(clip);
	}
	else if (m_pasteOptions.m_pasteGuid)
	{
		PutGuidOntoClipboard(clip);
	}
	else if (m_pasteOptions.m_pasteAsImage)
	{
		PasteAsImage(clip);
	}

	SaveDittoFileDataToFile(clip);

	return PutFormatOnClipboard(&clip.m_Formats) > 0;
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
		unicodeText->Data(NewGlobalP(const_cast<wchar_t*>(result.c_str()), (result.size() + 1) * sizeof(wchar_t)));
	}
	if (ansiText != NULL)
	{
		const CStringA ansi = CTextConvert::UnicodeToAnsi(CString(result.c_str(), static_cast<int>(result.size())));
		ansiText->Free();
		ansiText->Data(NewGlobalP(const_cast<char*>(ansi.GetString()), ansi.GetLength() + 1));
	}
}

void COleClipSource::TransformRtf(CClip &clip, const std::function<std::string(std::string_view)>& transform)
{
	IClipFormat *rtf = clip.m_Formats.FindFormatEx(theApp.m_RTFFormat);
	if (rtf == NULL)
	{
		return;
	}
	const CStringA source = rtf->GetAsCStringA();
	const std::string result = transform(std::string_view(source.GetString(), source.GetLength()));
	rtf->Free();
	rtf->Data(NewGlobalP(const_cast<char*>(result.c_str()), result.size() + 1));
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

void COleClipSource::PlainTextFilter(CClip &clip)
{
	bool foundText = false;
	INT_PTR hDropIndex = -1;
	INT_PTR	count = clip.m_Formats.GetCount();
	for (INT_PTR i = 0; i < count; i++)
	{
		CClipFormat *pCF = &clip.m_Formats.ElementAt(i);

		if (pCF->m_cfType == CF_TEXT ||
			pCF->m_cfType == CF_UNICODETEXT)
		{
			foundText = true;
		}
		else if (pCF->m_cfType == CF_HDROP)
		{
			hDropIndex = i;
		}
	}

	if (foundText &&
		hDropIndex > -1)
	{
		clip.m_Formats.RemoveAt(hDropIndex);
	}
	else if (foundText == false &&
		hDropIndex > -1)
	{
		CCF_HDropAggregator HDrop;
		if (m_ClipIDs.AggregateData(HDrop, CF_HDROP, CGetSetOptions::m_bMultiPasteReverse, m_pasteOptions.LimitFormatsToText()))
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
	const CString folder = CGetSetOptions::GetPath(PATH_DRAG_FILES);
	std::set<CString> usedNames;
	for (const DittoCore::FileDataEntry& file : files)
	{
		const CString originalPath = CTextConvert::Utf8ToUnicode(CStringA(file.path.data(), static_cast<int>(file.path.size())));
		CMd5 calcMd5;
		const std::string md5 = calcMd5.CalcMD5FromString(reinterpret_cast<const char*>(file.data.data()), static_cast<int>(file.data.size()));
		if (md5 != file.md5)
		{
			throw DittoCore::ClipboardFormatError("the saved contents of " + std::string(file.path) + " fail their MD5 check");
		}

		const CString newFilePath = folder + UniqueFileName(originalPath, usedNames);
		Log(StrF(_T("Saving file contents from Ditto, original file: %s, size: %Iu, md5: %S, to: %s"), originalPath.GetString(), file.data.size(), md5.c_str(), newFilePath.GetString()));

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
		candidate = StrF(_T("%s (%d)%s%s"), numbered.GetName().GetString(), n, extension.IsEmpty() ? _T("") : _T("."), extension.GetString());
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

		if (pCF->m_cfType == theApp.m_DittoFileData)
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
	Log(_T("Start of put format on clipboard"));

	CClipFormat* pCF;
	INT_PTR	count = pFormats->GetSize();
	INT_PTR i = 0;

	for(i = 0; i < count; i++)
	{
		pCF = &pFormats->ElementAt(i);

		Log(StrF(_T("Setting clipboard type: %s to the clipboard"), GetFormatName(pCF->m_cfType).GetString()));

		CacheGlobalData(pCF->m_cfType, pCF->m_hgData);
		pCF->m_hgData = 0; // OLE owns it now
	}

	pFormats->RemoveAll();

	m_bLoadedFormats = true;

	Log(_T("End of put format on clipboard"));

	return count;
}

// Renders the pasted clips in the given format. OnRenderGlobalData is a COM callback that no
// exception may leave: a malformed clip or a database error is shown to the user and gives no value.
std::optional<HGLOBAL> COleClipSource::RenderClipsOrReport(CLIPFORMAT format)
{
	try
	{
		HGLOBAL hData = m_ClipIDs.Render(format);
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
		CErrorReport::Show(StrF(_T("Ditto could not provide %s for the paste: the clip's data is malformed (%s)."),
			GetFormatName(format).GetString(), CString(error.what()).GetString()));
	}
	catch (CppSQLite3Exception& error)
	{
		CErrorReport::Show(StrF(_T("Ditto could not provide %s for the paste: database error %d (%s)."),
			GetFormatName(format).GetString(), error.errorCode(), error.errorMessage()));
	}
	return std::nullopt;
}

BOOL COleClipSource::OnRenderGlobalData(LPFORMATETC lpFormatEtc, HGLOBAL* phGlobal)
{
	static bool bInHere = false;

	if(bInHere)
	{
		return FALSE;
	}
	bInHere = true;

	HGLOBAL hData = NULL;

	CClipFormat *pFind = m_DelayRenderedFormats.FindFormat(lpFormatEtc->cfFormat);

	if(pFind)
	{
		if(pFind->m_hgData)
		{
			hData = NewGlobalH(pFind->m_hgData, GlobalSize(pFind->m_hgData));
		}
	}
	else
	{
		// m_delayRenderLockout holds a 32-bit tick value, so the difference uses 32-bit wrap-around arithmetic
		const DWORD now = static_cast<DWORD>(GetTickCount64());
		if (m_pasteOptions.m_delayRenderLockout > 0 &&
			(now - m_pasteOptions.m_delayRenderLockout) < (DWORD)CGetSetOptions::GetDelayRenderLockout())
		{
			bInHere = false;
			return false;
		}

		if(m_ClipIDs.GetCount() > 0)
		{
			const std::optional<HGLOBAL> rendered = RenderClipsOrReport(lpFormatEtc->cfFormat);
			if (!rendered)
			{
				bInHere = false;
				return FALSE;   // FALSE tells the target the render failed
			}
			hData = *rendered;
		}

		//Add to a cache of already rendered data
		//Windows seems to call this function multiple times
		//so only the first time do we need to go get the data
		HGLOBAL hCopy = NULL;
		if(hData)
		{
			hCopy = NewGlobalH(hData, GlobalSize(hData));
		}

		CClipFormat format(lpFormatEtc->cfFormat, hCopy);
		m_DelayRenderedFormats.Add(format);
		format.m_autoDeleteData = false; //owned by m_DelayRenderedFormats
	}

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
				CopyToGlobalHH(*phGlobal, hData, len);
			}
			::GlobalFree(hData);
		}
		bRet = TRUE;
	}

	bInHere = false;

	return bRet;
}

HGLOBAL COleClipSource::ConvertToFileDrop()
{
	CString path = CGetSetOptions::GetPath(PATH_DRAG_FILES);
	CreateDirectory(path, NULL);

	std::vector<std::wstring> fileList;

	int dragId = CGetSetOptions::GetDragId();
	int origDragId = dragId;

	auto customDragName = CGetSetOptions::GetTempDragFileName();
	if (customDragName != _T(""))
	{
		dragId = 1;
	}

	for (int i = 0; i < m_ClipIDs.GetCount(); i++)
	{
		CClip fileClip;
		fileClip.LoadFormats(m_ClipIDs[i]);

		CClipFormat *unicodeText = fileClip.m_Formats.FindFormat(CF_UNICODETEXT);
		if (unicodeText)
		{
			CString name = _T("text");
			CString file;
			if (customDragName != _T(""))
			{
				name = customDragName;
				file.Format(_T("%s%s.txt"), path.GetString(), name.GetString());
			}
			else
			{
				file.Format(_T("%s%s_%d.txt"), path.GetString(), name.GetString(), dragId++);
			}

			fileClip.WriteTextToFile(file, TRUE, FALSE, FALSE);
			fileList.push_back(file.GetString());
			continue;
		}

		CClipFormat *asciiText = fileClip.m_Formats.FindFormat(CF_TEXT);
		if (asciiText)
		{
			CString name = _T("text");
			CString file;
			if (customDragName != _T(""))
			{
				name = customDragName;
				file.Format(_T("%s%s.txt"), path.GetString(), name.GetString());
			}
			else
			{
				file.Format(_T("%s%s_%d.txt"), path.GetString(), name.GetString(), dragId++);
			}

			fileClip.WriteTextToFile(file, FALSE, TRUE, FALSE);
			fileList.push_back(file.GetString());
			continue;
		}

		CClipFormat *png = fileClip.m_Formats.FindFormat(theApp.m_PNG_Format);
		CClipFormat *bitmap = fileClip.m_Formats.FindFormat(CF_DIB);
		if (bitmap != NULL ||
			png != NULL)
		{
			CString name = _T("image");
			CString file;
			if (customDragName != _T(""))
			{
				name = customDragName;
				file.Format(_T("%s%s.png"), path.GetString(), name.GetString());
			}
			else
			{
				file.Format(_T("%s%s_%d.png"), path.GetString(), name.GetString(), dragId++);
			}

			if (fileClip.WriteImageToFile(file))
			{
				fileList.push_back(file.GetString());
			}
		}
	}

	if(customDragName == _T("") &&
		dragId != origDragId)
	{
		CGetSetOptions::SetDragId(dragId);
	}

	HGLOBAL hData = CCF_HDropAggregator::NewDropBlock(fileList);

	return hData;
}

void COleClipSource::Slugify(CClip &clip)
{
	const CString separator = CGetSetOptions::GetSlugifySeparator();
	const std::wstring_view separatorView(separator.GetString(), separator.GetLength());
	TransformText(clip, [separatorView](std::wstring_view text) { return DittoCore::Slugifier::Slugify(text, separatorView); });
}

void COleClipSource::PutGuidOntoClipboard(CClip& clip)
{
	Log(_T("Start of put Guid on clipboard"));

	clip.m_Formats.RemoveAll();

	CString guid = NewGuidString();

	long len = guid.GetLength();
	HGLOBAL hGlobal = NewGlobalP(guid.GetBuffer(), ((len + 1) * sizeof(wchar_t)));

	CClipFormat cf(CF_UNICODETEXT, hGlobal);
	clip.m_Formats.Add(cf);

	//clip.m_Formats now owns the global data
	cf.m_autoDeleteData = false;

	Log(_T("End of put Guid on clipboard"));
}

void COleClipSource::PasteAsImage(CClip& clip)
{
	Log(_T("Start of PasteAsImage"));

	IClipFormat* pUnicodeText = clip.m_Formats.FindFormatEx(CF_UNICODETEXT);
	if (pUnicodeText == NULL)
	{
		Log(_T("PasteAsImage - no unicode text found"));
		return;
	}

	CString path = pUnicodeText->GetAsCString();
	path.Trim();

	if (path.IsEmpty() || !PathFileExists(path))
	{
		Log(StrF(_T("PasteAsImage - path not found: %s"), path.GetString()));
		return;
	}

	CImage image;
	HRESULT hr = image.Load(path);
	if (FAILED(hr))
	{
		Log(StrF(_T("PasteAsImage - failed to load image: %s"), path.GetString()));
		return;
	}

	HBITMAP hBitmap = (HBITMAP)image;
	HPALETTE hPal = NULL;
	HANDLE hDib = CBitmapHelper::hBitmapToDIB(hBitmap, BI_RGB, hPal);
	if (hDib == NULL)
	{
		Log(_T("PasteAsImage - failed to convert to DIB"));
		return;
	}

	clip.m_Formats.RemoveAll();

	CClipFormat cfDib(CF_DIB, (HGLOBAL)hDib);
	clip.m_Formats.Add(cfDib);
	cfDib.m_autoDeleteData = false;

	IStream* pStream = nullptr;
	if (SUCCEEDED(CreateStreamOnHGlobal(nullptr, TRUE, &pStream)))
	{
		if (SUCCEEDED(image.Save(pStream, Gdiplus::ImageFormatPNG)))
		{
			LARGE_INTEGER liZero = { 0 };
			ULARGE_INTEGER ulSize;
			pStream->Seek({ 0 }, STREAM_SEEK_END, &ulSize);
			pStream->Seek(liZero, STREAM_SEEK_SET, nullptr);

			HGLOBAL hPng = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)ulSize.QuadPart);
			if (hPng)
			{
				LPVOID pDst = GlobalLock(hPng);
				pStream->Read(pDst, (ULONG)ulSize.QuadPart, nullptr);
				GlobalUnlock(hPng);

				CClipFormat cfPng(theApp.m_PNG_Format, hPng);
				clip.m_Formats.Add(cfPng);
				cfPng.m_autoDeleteData = false;
			}
		}
		pStream->Release();
	}

	Log(_T("End of PasteAsImage"));
}
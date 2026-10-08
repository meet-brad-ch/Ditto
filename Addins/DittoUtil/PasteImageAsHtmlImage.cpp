#include "StdAfx.h"
#include ".\pasteimageashtmlimage.h"
#include "../../shared/TextConvert.h"
#include "CfHtml.h"
#include "ClipboardFormatError.h"
#include "DibHeader.h"
#include "GlobalBytes.h"
#include "GlobalFileDrop.h"

#include <string>
#include <vector>

CPasteImageAsHtmlImage::CPasteImageAsHtmlImage(void)
{
}

CPasteImageAsHtmlImage::~CPasteImageAsHtmlImage(void)
{
}

bool CPasteImageAsHtmlImage::ConvertPathToHtmlImageTag(const CDittoInfo& DittoInfo, IClip* pClip)
{
	bool bRet = false;
	IClipFormats* pFormats = pClip->Clips();
	if (pFormats)
	{
		if (m_dibImagePath.IsEmpty() && CreateLocalPath(true) == false)
		{
			CString message;
			message.Format(_T("The images were not pasted as HTML: the temporary folder could not be found (error %lu)."), ::GetLastError());
			::MessageBox(DittoInfo.m_hWndDitto, message, _T("Ditto"), MB_OK | MB_ICONERROR);
			return false;
		}

		CString csIMG = _T("");

		if (!GetImageTags(DittoInfo.m_hWndDitto, pFormats, csIMG))
			return false;

		if (csIMG.IsEmpty() == FALSE)
		{
			pFormats->DeleteAll();
			const CStringA utf8 = CTextConvert::UnicodeToUTF8(csIMG);
			// a real CF_HTML block (header with byte offsets) with its terminating null
			const std::string block = DittoCore::CfHtml::Build(std::string(utf8.GetString(), utf8.GetLength()), "", "");
			pFormats->AddNew(DittoAddinHelpers::GetFormatID(_T("HTML Format")), DittoAddinHelpers::NewGlobalP(const_cast<char*>(block.c_str()), static_cast<UINT>(block.size() + 1)));
			bRet = true;
		}
	}

	return bRet;
}

bool CPasteImageAsHtmlImage::GetImageTags(HWND owner, IClipFormats* pFormats, CString& csIMG)
{
	IClipFormat* pCF_DIB = pFormats->FindFormatEx(CF_DIB);
	if (pCF_DIB != NULL)
	{
		return DibImageTag(owner, pCF_DIB, csIMG);
	}

	IClipFormat* pHDrop = pFormats->FindFormatEx(CF_HDROP);
	if (pHDrop)
	{
		return HDropImageTags(owner, pHDrop, csIMG);
	}

	return true;
}

bool CPasteImageAsHtmlImage::HDropImageTags(HWND owner, IClipFormat* pHDrop, CString& csIMG)
{
	std::vector<std::wstring> files;
	try
	{
		files = DittoCore::GlobalFileDrop::Read(pHDrop->Data()).Paths();
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		// add-in boundary: no exception may cross into Ditto
		CString message;
		message.Format(_T("The images were not pasted as HTML: the clip's file list is malformed (%s)."), CString(error.what()).GetString());
		::MessageBox(owner, message, _T("Ditto"), MB_OK | MB_ICONERROR);
		return false;
	}

	const size_t nNumFiles = files.size();
	for (size_t nFile = 0; nFile < nNumFiles; nFile++)
	{
		CString csOrigfile(files[nFile].c_str());
		CString csFile(csOrigfile);
		csFile = csFile.MakeLower();

		if (IsImageFile(csFile))
		{
			CString csFormat;
			csFormat.Format(_T("<IMG src=\"file:///%s\">"), csOrigfile.GetString());
			if (nFile < nNumFiles - 1)
			{
				csFormat += _T("<br>");
			}
			csIMG += csFormat;
		}
	}

	return true;
}

bool CPasteImageAsHtmlImage::IsImageFile(const CString& csFile)
{
	for (const TCHAR* extension : m_imageExtensions)
	{
		if (csFile.Find(extension) != -1)
		{
			return true;
		}
	}
	return false;
}

bool CPasteImageAsHtmlImage::DibImageTag(HWND owner, IClipFormat* pCF_DIB, CString& csIMG)
{
	CString csFile;
	csFile.Format(_T("%s\\%d.bmp"), m_dibImagePath.GetString(), m_nextDibImageName);
	m_nextDibImageName++;

	CString errorMessage;
	try
	{
		// add-in boundary: no exception may cross into Ditto
		const DittoCore::GlobalBytes block(pCF_DIB->Data());
		if (WriteDibToFile(csFile, block.Bytes(), errorMessage))
		{
			csIMG.Format(_T("<IMG src=\"file:///%s\">"), csFile.GetString());
			return true;
		}
	}
	catch (const DittoCore::ClipboardFormatError& error)
	{
		errorMessage.Format(_T("The image was not pasted as HTML: the clip's image is malformed (%s)."), CString(error.what()).GetString());
	}
	::MessageBox(owner, errorMessage, _T("Ditto"), MB_OK | MB_ICONERROR);
	return false;
}

bool CPasteImageAsHtmlImage::WriteDibToFile(const CString& csPath, std::span<const std::byte> dib, CString& errorMessage)
{
	// Read validates the DIB before anything is written
	const auto header = DittoCore::DibHeader::FileHeader(DittoCore::DibHeader::Read(dib), dib.size());

	CFile file;
	CFileException ex;
	if (!file.Open(csPath, CFile::modeCreate | CFile::modeWrite | CFile::typeBinary, &ex))
	{
		TCHAR exError[250]{};
		ex.GetErrorMessage(exError, _countof(exError));
		errorMessage.Format(_T("The image was not pasted as HTML: it could not be saved to %s (%s)."), csPath.GetString(), exError);
		return false;
	}

	file.Write(header.data(), static_cast<UINT>(header.size()));
	file.Write(dib.data(), static_cast<UINT>(dib.size()));
	file.Close();
	return true;
}

bool CPasteImageAsHtmlImage::CleanupPastedImages()
{
	if (m_dibImagePath.IsEmpty() && CreateLocalPath(false) == false)
	{
		return false;
	}

	CFileFind find;
	BOOL bCont = find.FindFile(m_dibImagePath + _T("\\*"));

	while (bCont)
	{
		bCont = find.FindNextFile();
		DeleteFile(find.GetFilePath());
	}
	find.Close();

	return RemoveDirectory(m_dibImagePath) != FALSE;
}

bool CPasteImageAsHtmlImage::CreateLocalPath(bool bCreateDir)
{
	// GetTempPath reads %TMP% first, then %TEMP%, %USERPROFILE% and the Windows folder; upstream
	// read %TMP% only, so without TMP the folder became "\ditto" (the root of the current drive)
	TCHAR tempPath[MAX_PATH + 1]{};
	const DWORD capacity{ static_cast<DWORD>(_countof(tempPath)) };
	const DWORD length{ ::GetTempPath(capacity, tempPath) };
	if (length == 0 || length > capacity)
	{
		return false;
	}

	// GetTempPath's path ends with a backslash
	m_dibImagePath = tempPath;
	m_dibImagePath += _T("ditto");
	if (bCreateDir)
	{
		CreateDirectory(m_dibImagePath, NULL);
	}
	return true;
}

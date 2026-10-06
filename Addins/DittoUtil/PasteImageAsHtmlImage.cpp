#include "StdAfx.h"
#include ".\pasteimageashtmlimage.h"
#include "../../shared/TextConvert.h"
#include "ClipboardFormatError.h"
#include "GlobalFileDrop.h"

#include <string>
#include <vector>

CString g_csDIBImagePath = _T("");
int g_nDIBImageName = 1;

CPasteImageAsHtmlImage::CPasteImageAsHtmlImage(void)
{
}

CPasteImageAsHtmlImage::~CPasteImageAsHtmlImage(void)
{
}

bool CPasteImageAsHtmlImage::ConvertPathToHtmlImageTag(const CDittoInfo &DittoInfo, IClip *pClip)
{
	bool bRet = false;
	IClipFormats *pFormats = pClip->Clips();
	if(pFormats)
	{
		if(g_csDIBImagePath.IsEmpty())
		{
			CreateLocalPath(true);
		}

		CString csIMG = _T("");

		IClipFormat *pCF_DIB = pFormats->FindFormatEx(CF_DIB);
		if(pCF_DIB != NULL)
		{
			CString csFile;
			csFile.Format(_T("%s\\%d.bmp"), g_csDIBImagePath, g_nDIBImageName);
			g_nDIBImageName++;


			LPVOID pvData = GlobalLock(pCF_DIB->Data());
			ULONG size = (ULONG)GlobalSize(pCF_DIB->Data());

			if(WriteDataToFile(csFile, pvData, size))
			{
				GlobalUnlock(pCF_DIB->Data());

				csIMG.Format(_T("<IMG src=\"file:///%s\">"), csFile);
			}
			else
			{
				GlobalUnlock(pCF_DIB->Data());
			}
		}
		else
		{
			IClipFormat *pHDrop = pFormats->FindFormatEx(CF_HDROP);
			if(pHDrop)
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
					::MessageBox(DittoInfo.m_hWndDitto, message, _T("Ditto"), MB_OK | MB_ICONERROR);
					return false;
				}

				const size_t nNumFiles = files.size();
				for(size_t nFile = 0; nFile < nNumFiles; nFile++)
				{
					{
						CString csOrigfile(files[nFile].c_str());
						CString csFile(csOrigfile);
						csFile = csFile.MakeLower();

						if(csFile.Find(_T(".bmp")) != -1 || 
							csFile.Find(_T(".dib")) != -1 ||
							csFile.Find(_T(".jpg")) != -1 ||
							csFile.Find(_T(".jpeg")) != -1 ||
							csFile.Find(_T(".jpe")) != -1 ||
							csFile.Find(_T(".jfif")) != -1 ||
							csFile.Find(_T(".gif")) != -1 ||
							csFile.Find(_T(".tif")) != -1 ||
							csFile.Find(_T(".tiff")) != -1 ||
							csFile.Find(_T(".png")) != -1)
						{
							CString csFormat;
							csFormat.Format(_T("<IMG src=\"file:///%s\">"), csOrigfile);
							if(nFile < nNumFiles-1)
							{
								csFormat += _T("<br>");
							}
							csIMG += csFormat;
						}
					}
				}
			}
		}

		if(csIMG.IsEmpty() == FALSE)
		{
			pFormats->DeleteAll();
			CStringA utf8 = CTextConvert::UnicodeToUTF8(csIMG);
			pFormats->AddNew(DittoAddinHelpers::GetFormatID(_T("HTML Format")), DittoAddinHelpers::NewGlobalP(utf8.GetBuffer(), utf8.GetLength()));
			bRet = true;
		}
	}

	return bRet;
}

bool CPasteImageAsHtmlImage::WriteDataToFile(CString csPath, LPVOID data, ULONG size)
{
	bool bRet = false;
	CFile file;
	CFileException ex;
	if(file.Open(csPath, CFile::modeCreate|CFile::modeWrite|CFile::typeBinary, &ex))
	{
		BITMAPINFO *lpBI = (BITMAPINFO *)data;

		int nPaletteEntries = 1 << lpBI->bmiHeader.biBitCount;
		if(lpBI->bmiHeader.biBitCount > 8)
			nPaletteEntries = 0;
		else if( lpBI->bmiHeader.biClrUsed != 0 )
			nPaletteEntries = lpBI->bmiHeader.biClrUsed;

		BITMAPFILEHEADER BFH;
		memset(&BFH, 0, sizeof( BITMAPFILEHEADER));
		BFH.bfType = 'MB';
		BFH.bfSize = sizeof(BITMAPFILEHEADER) + size;
		BFH.bfOffBits = sizeof(BITMAPFILEHEADER) + sizeof(BITMAPINFOHEADER) + nPaletteEntries * sizeof(RGBQUAD);

		file.Write(&BFH, sizeof(BITMAPFILEHEADER));
		file.Write(data, size);

		file.Close();

		bRet = true;
	}
	else
	{
		CString csError;
		TCHAR exError[250];
		ex.GetErrorMessage(exError, _countof(exError));

		csError.Format(_T("OutLookExpress Addin - Failed to write CF_DIB to file: %s, Error: %s"), csPath, exError);
		OutputDebugString(csPath);
	}

	return bRet;
}

bool CPasteImageAsHtmlImage::CleanupPastedImages()
{
	bool bRet = false;
	if(g_csDIBImagePath.IsEmpty())
	{
		CreateLocalPath(false);
	}

	CFileFind find;
	BOOL bCont = find.FindFile(g_csDIBImagePath + _T("\\*"));

	while(bCont)
	{
		bCont = find.FindNextFile();
		DeleteFile(find.GetFilePath());
	}
	find.Close();

	bRet = RemoveDirectory(g_csDIBImagePath) == TRUE;

	return false;;
}

void CPasteImageAsHtmlImage::CreateLocalPath(bool bCreateDir)
{
	g_csDIBImagePath = _wgetenv(_T("TMP"));;
	g_csDIBImagePath += _T("\\ditto");
	if(bCreateDir)
	{
		CreateDirectory(g_csDIBImagePath, NULL);
	}
}
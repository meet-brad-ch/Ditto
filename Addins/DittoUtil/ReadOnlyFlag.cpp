#include "StdAfx.h"
#include "ReadOnlyFlag.h"
#include "../../Shared/Tokenizer.h"
#include "../../Shared/TextConvert.h"
#include "ClipboardFormatError.h"
#include "GlobalFileDrop.h"


CReadOnlyFlag::CReadOnlyFlag(void)
{
}


CReadOnlyFlag::~CReadOnlyFlag(void)
{
}


bool CReadOnlyFlag::ResetReadOnlyFlag(const CDittoInfo &DittoInfo, IClip *pClip, bool resetFlag)
{
	IClipFormats *pFormats = pClip->Clips();
	if(pFormats)
	{
		CStringArray lines;

		try
		{
			LoadHDropFiles(lines, pFormats);
		}
		catch (const DittoCore::ClipboardFormatError& error)
		{
			// add-in boundary: no exception may cross into Ditto
			CString message;
			message.Format(_T("The read-only flag was not changed: the clip's file list is malformed (%s)."), CString(error.what()).GetString());
			::MessageBox(DittoInfo.m_hWndDitto, message, _T("Ditto"), MB_OK | MB_ICONERROR);
			return false;
		}

		if(lines.GetSize() <= 0)
		{
			LoadUnicodeFiles(lines, pFormats);
		}

		if(lines.GetSize() <= 0)
		{
			LoadTextFiles(lines, pFormats);
		}

		for(int i = 0; i < lines.GetSize(); i++)
		{
			CString file = lines[i].TrimLeft(' ').TrimRight(' ').MakeLower();

			//Find the first occurance of a file // or \\ for a network file or a->z:\\ for a local files
			int pos = file.Find(_T("//"));
			if(pos >= 0)
			{
				file = file.Mid(pos);
			}
			else
			{
				pos = file.Find(_T("\\\\"));
				if(pos >= 0)
				{
					file = file.Mid(pos);
				}
				else
				{
					for(wchar_t drive = 'a'; drive <= 'z'; drive++)
					{
						CString csDrive(drive);
						csDrive += _T(":\\");

						pos = file.Find(csDrive);
						if(pos >= 0)
						{
							file = file.Mid(pos);
							break;
						}

						csDrive =  drive;
						csDrive += _T(":/");

						pos = file.Find(csDrive);
						if(pos >= 0)
						{
							file = file.Mid(pos);
							break;
						}
					}
				}
			}

			BOOL success = FALSE;
			if(resetFlag)
			{
				success = ::SetFileAttributes(file, FILE_ATTRIBUTE_NORMAL);
			}
			else
			{
				success = ::SetFileAttributes(file, FILE_ATTRIBUTE_READONLY);
			}
		}
	}

	return true;
}

bool CReadOnlyFlag::LoadUnicodeFiles(CStringArray &lines, IClipFormats *pFormats)
{		
	IClipFormat *pFormat = pFormats->FindFormatEx(CF_UNICODETEXT);
	if(pFormat != NULL)
	{
		wchar_t *stringData = (wchar_t *)GlobalLock(pFormat->Data());
		if(stringData != NULL)
		{
			CString string(stringData);
			CString delim(_T("\r\n"));

			CTokenizer token(string, delim);
			CString line;
			while(token.Next(line))
			{
				lines.Add(line);
			}

			GlobalUnlock(pFormat->Data());
		}
	}

	return lines.GetSize() > 0;
}

bool CReadOnlyFlag::LoadTextFiles(CStringArray &lines, IClipFormats *pFormats)
{		
	IClipFormat *pFormat = pFormats->FindFormatEx(CF_TEXT);
	if(pFormat != NULL)
	{
		char *stringData = (char *)GlobalLock(pFormat->Data());
		if(stringData != NULL)
		{
			CStringA string(stringData);
			CStringW unicodeString(CTextConvert::AnsiToUnicode(string));
			CString delim(_T("\r\n"));

			CTokenizer token(unicodeString, delim);
			CString line;
			while(token.Next(line))
			{
				lines.Add(line);
			}

			GlobalUnlock(pFormat->Data());
		}
	}

	return lines.GetSize() > 0;
}

bool CReadOnlyFlag::LoadHDropFiles(CStringArray &lines, IClipFormats *pFormats)
{
	IClipFormat *pFormat = pFormats->FindFormatEx(CF_HDROP);
	if(pFormat != NULL)
	{
		for (const std::wstring& path : DittoCore::GlobalFileDrop::Read(pFormat->Data()).Paths())
		{
			lines.Add(path.c_str());
		}
	}

	return lines.GetSize() > 0;
}
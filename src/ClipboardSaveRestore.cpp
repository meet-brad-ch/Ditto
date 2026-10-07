#include "stdafx.h"
#include "CP_Main.h"
#include ".\clipboardsaverestore.h"
#include "GlobalFileDrop.h"

CClipboardSaveRestore::CClipboardSaveRestore(void)
{
}

CClipboardSaveRestore::~CClipboardSaveRestore(void)
{
}

bool CClipboardSaveRestore::Save(BOOL textOnly)
{
	m_Clipboard.RemoveAll();

	bool bRet = false;
	COleDataObjectEx oleData;
	CClipFormat cf;

	if(::OpenClipboard(theApp.m_MainhWnd))
	{
		UINT nFormat = EnumClipboardFormats(0);
		while(nFormat != 0)
		{
			if(IsFormatToSave(textOnly, nFormat))
			{
				SaveFormat(nFormat, cf);
			}
			nFormat = EnumClipboardFormats(nFormat);
		}

		::CloseClipboard();
		bRet = true;
	}

	return bRet;
}

bool CClipboardSaveRestore::IsFormatToSave(BOOL textOnly, UINT nFormat)
{
	return textOnly == false || (nFormat == CF_TEXT || nFormat == CF_UNICODETEXT || nFormat == CF_HDROP);
}

void CClipboardSaveRestore::SaveFormat(UINT nFormat, CClipFormat& cf)
{
	HGLOBAL hGlobal = ::GetClipboardData(nFormat);
	if(hGlobal && ::GlobalSize(hGlobal) > 0) // Ensure clipboard data is valid
	{
		LPVOID pvData = GlobalLock(hGlobal);
		if(pvData)
		{
			INT_PTR size = GlobalSize(hGlobal);
			if(size > 0)
			{
				//Copy the data locally
				cf.m_hgData = CGlobalMemory::NewGlobalP(pvData, size);
				// Clipboard format ids are 16-bit values, so they fit a CLIPFORMAT
				cf.m_cfType = static_cast<CLIPFORMAT>(nFormat);

				m_Clipboard.Add(cf);

				//m_Clipboard owns the data now
				cf.m_hgData = NULL;
			}

			GlobalUnlock(hGlobal);
		}
	}
}

bool CClipboardSaveRestore::Restore()
{
	bool bRet = false;

	if(::OpenClipboard(theApp.m_MainhWnd))
	{
		::EmptyClipboard();

		SetClipboardData(theApp.m_cfIgnoreClipboard, CGlobalMemory::NewGlobalP("Ignore", sizeof("Ignore")));

		INT_PTR size = m_Clipboard.GetSize();
		for(int nPos = 0; nPos < size; nPos++)
		{
			CClipFormat *pCF = &m_Clipboard.ElementAt(nPos);
			if(pCF && pCF->m_hgData && ::GlobalSize(pCF->m_hgData) > 0) // Ensure clipboard data is valid
			{
				::SetClipboardData(pCF->m_cfType, pCF->m_hgData);
				pCF->m_hgData = NULL;//clipboard now owns the data
			}
		}

		bRet = TRUE;
		::CloseClipboard();
	}

	m_Clipboard.RemoveAll();

	if(bRet == FALSE)
	{
		CLogger::Log(_T("CClipboardSaveRestore::Restore failed to restore clipboard"));
	}

	return bRet;
}

bool CClipboardSaveRestore::RestoreTextOnly()
{
	bool bRet = false;

	// Find the text formats and the file list first. The file list is read before the clipboard is
	// opened, so malformed data throws while the clipboard is still untouched.
	int hDropIndex = -1;
	const bool foundText = FindTextFormats(hDropIndex);

	//if there is no text but a hdrop, the hdrop is converted to text with the paths it lists
	const bool convertHDrop = (foundText == false && hDropIndex > -1);
	CString hDropString;
	if(convertHDrop)
	{
		hDropString = GetHDropFilePaths(hDropIndex);
	}

	if(::OpenClipboard(theApp.m_MainhWnd))
	{
		::EmptyClipboard();

		SetClipboardData(theApp.m_cfIgnoreClipboard, CGlobalMemory::NewGlobalP("Ignore", sizeof("Ignore")));

		SetTextFormatCopies();

		if(convertHDrop)
		{
			HGLOBAL newData = CGlobalMemory::NewGlobalP(hDropString.GetBuffer(), ((hDropString.GetLength() + 1) * sizeof(TCHAR)));
			::SetClipboardData(CF_UNICODETEXT, newData);
		}

		bRet = TRUE;
		::CloseClipboard();
	}

	if(bRet == FALSE)
	{
		CLogger::Log(_T("CClipboardSaveRestore::Restore failed to restore clipboard"));
	}

	return bRet;
}

bool CClipboardSaveRestore::HasValidData(const CClipFormat *pCF)
{
	return pCF && pCF->m_hgData && ::GlobalSize(pCF->m_hgData) > 0; // Ensure clipboard data is valid
}

bool CClipboardSaveRestore::IsTextFormat(CLIPFORMAT cfType)
{
	return cfType == CF_TEXT || cfType == CF_UNICODETEXT;
}

bool CClipboardSaveRestore::FindTextFormats(int& hDropIndex)
{
	bool foundText = false;

	INT_PTR size = m_Clipboard.GetSize();
	for(int pos = 0; pos < size; pos++)
	{
		CClipFormat *pCF = &m_Clipboard.ElementAt(pos);
		if(HasValidData(pCF))
		{
			if(IsTextFormat(pCF->m_cfType))
			{
				foundText = true;
			}
			else if(pCF->m_cfType == CF_HDROP)
			{
				hDropIndex = pos;
			}
		}
	}

	return foundText;
}

CString CClipboardSaveRestore::GetHDropFilePaths(int hDropIndex)
{
	CString hDropString;
	for (const std::wstring& path : DittoCore::GlobalFileDrop::Read(m_Clipboard.ElementAt(hDropIndex).m_hgData).Paths())
	{
		if (PathIsDirectory(path.c_str()) == FALSE)
		{
			hDropString += path.c_str();
			hDropString += _T("\r\n");
		}
	}
	return hDropString;
}

void CClipboardSaveRestore::SetTextFormatCopies()
{
	INT_PTR size = m_Clipboard.GetSize();
	for(int pos = 0; pos < size; pos++)
	{
		CClipFormat *pCF = &m_Clipboard.ElementAt(pos);
		if(HasValidData(pCF) && IsTextFormat(pCF->m_cfType))
		{
			//Make a copy of the data we are putting on the clipboard so we can still
			//restore all clips later in Restore()
			LPVOID localData = ::GlobalLock(pCF->m_hgData);

			HGLOBAL newData = CGlobalMemory::NewGlobalP(localData, ::GlobalSize(pCF->m_hgData));
			::SetClipboardData(pCF->m_cfType, newData);

			::GlobalUnlock(pCF->m_hgData);
		}
	}
}

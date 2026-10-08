#include "StdAfx.h"
#include "RemoveLineFeeds.h"

#include "ClipboardFormatError.h"
#include "ClipText.h"

#include <cstring>
#include <string>


CRemoveLineFeeds::CRemoveLineFeeds(void)
{
}


CRemoveLineFeeds::~CRemoveLineFeeds(void)
{
}

bool CRemoveLineFeeds::RemoveLineFeeds(const CDittoInfo& DittoInfo, IClip* pClip)
{
	bool didSomething = false;
	IClipFormats* pFormats = pClip->Clips();
	if (pFormats)
	{
		try
		{
			didSomething = Handle_CF_TEXT(pFormats);

			didSomething |= Handle_CF_UNICODETEXT(pFormats);

			didSomething |= Handle_RichText(pFormats);
		}
		catch (const DittoCore::ClipboardFormatError& error)
		{
			// add-in boundary: no exception may cross into Ditto
			CString message;
			message.Format(_T("Line feeds were not removed: the clip's data is malformed (%s)."), CString(error.what()).GetString());
			::MessageBox(DittoInfo.m_hWndDitto, message, _T("Ditto"), MB_OK | MB_ICONERROR);
			return false;
		}
	}

	return didSomething;
}

bool CRemoveLineFeeds::Handle_CF_TEXT(IClipFormats* pFormats)
{
	IClipFormat* pFormat = pFormats->FindFormatEx(CF_TEXT);
	if (pFormat == NULL)
	{
		return false;
	}

	DittoCore::GlobalBytes bytes(pFormat->Data());
	CStringA string(DittoCore::ClipText::ReadAnsiBounded(bytes.Bytes()).c_str());
	string.Replace("\r\n", " ");
	WriteBack(bytes, string.GetString(), string.GetLength(), sizeof(char));
	return true;
}

bool CRemoveLineFeeds::Handle_CF_UNICODETEXT(IClipFormats* pFormats)
{
	IClipFormat* pFormat = pFormats->FindFormatEx(CF_UNICODETEXT);
	if (pFormat == NULL)
	{
		return false;
	}

	DittoCore::GlobalBytes bytes(pFormat->Data());
	CStringW string(DittoCore::ClipText::ReadWideBounded(bytes.Bytes()).c_str());
	string.Replace(L"\r\n", L" ");
	WriteBack(bytes, string.GetString(), string.GetLength() * sizeof(wchar_t), sizeof(wchar_t));
	return true;
}

bool CRemoveLineFeeds::Handle_RichText(IClipFormats* pFormats)
{
	// Registered formats are in 0xC000-0xFFFF, so they fit a CLIPFORMAT
	CLIPFORMAT m_RTFFormat = static_cast<CLIPFORMAT>(::RegisterClipboardFormat(_T("Rich Text Format")));

	IClipFormat* pFormat = pFormats->FindFormatEx(m_RTFFormat);
	if (pFormat == NULL)
	{
		return false;
	}

	DittoCore::GlobalBytes bytes(pFormat->Data());
	CStringA string(DittoCore::ClipText::ReadAnsiBounded(bytes.Bytes()).c_str());
	string.Replace("\\par\r\n", " ");
	string.Replace("\\par ", " ");
	string.Replace("\\line ", " ");
	WriteBack(bytes, string.GetString(), string.GetLength(), sizeof(char));
	return true;
}

void CRemoveLineFeeds::WriteBack(DittoCore::GlobalBytes& bytes, const void* text, std::size_t textBytes, std::size_t terminatorBytes)
{
	const std::span<std::byte> block = bytes.WritableBytes();
	if (textBytes + terminatorBytes > block.size())
	{
		throw DittoCore::ClipboardFormatError("text of " + std::to_string(textBytes) + " bytes does not fit its block of " + std::to_string(block.size()));
	}
	std::memcpy(block.data(), text, textBytes);
	std::memset(block.data() + textBytes, 0, terminatorBytes);
}

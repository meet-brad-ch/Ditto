#include "StdAfx.h"
#include ".\pasteanyastext.h"
#include "SelectPasteFormat.h"
#include "ClipboardFormatError.h"
#include "GlobalBytes.h"

#include <algorithm>
#include <cstring>
#include <string>

PasteAnyAsText::PasteAnyAsText(void)
{
}

PasteAnyAsText::~PasteAnyAsText(void)
{
}

template <typename Char>
HGLOBAL PasteAnyAsText::TextBlock(std::span<const std::byte> bytes)
{
	std::basic_string<Char> text(bytes.size() / sizeof(Char), Char{});
	std::memcpy(text.data(), bytes.data(), text.size() * sizeof(Char));
	const std::size_t last = text.find_last_not_of(Char{});
	text.erase(last == std::basic_string<Char>::npos ? 0 : last + 1);
	// upstream turned every null into a space, the terminator too, so the text had none
	std::replace(text.begin(), text.end(), Char{}, static_cast<Char>(' '));
	return DittoAddinHelpers::NewGlobalP(const_cast<Char*>(text.c_str()), static_cast<UINT>((text.size() + 1) * sizeof(Char)));
}

bool PasteAnyAsText::SelectClipToPasteAsText(const CDittoInfo &DittoInfo, IClip *pClip)
{
	bool ret = false;
	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	IClipFormats *pFormats = pClip->Clips();

	CWnd* pWnd = CWnd::FromHandle(DittoInfo.m_hWndDitto);
	CSelectPasteFormat dlg(pWnd, pFormats);

	if(dlg.DoModal() == IDOK)
	{
		//Find the format that was selected, remove all then readd the data as text
		CLIPFORMAT format = dlg.SelectedFormat();
		if(format > 0)
		{
			IClipFormat *pText = pFormats->FindFormatEx(format);
			if(pText != NULL)
			{
				const bool unicode = dlg.PasteAsUnicode();
				HGLOBAL text = NULL;
				try
				{
					const DittoCore::GlobalBytes block(pText->Data());
					text = unicode ? TextBlock<wchar_t>(block.Bytes()) : TextBlock<char>(block.Bytes());
				}
				catch (const DittoCore::ClipboardFormatError& error)
				{
					// add-in boundary: no exception may cross into Ditto
					CString message;
					message.Format(_T("Paste as text stopped: the clip's data could not be read (%s)."), CString(error.what()).GetString());
					::MessageBox(DittoInfo.m_hWndDitto, message, _T("Ditto"), MB_OK | MB_ICONERROR);
					return false;
				}

				//Remove all other formats (they free their data) and add the text
				pFormats->DeleteAll();
				const CLIPFORMAT textFormat = unicode ? CF_UNICODETEXT : CF_TEXT;
				pFormats->AddNew(textFormat, text);

				IClipFormat *pAdded = pFormats->FindFormatEx(textFormat);
				if(pAdded != NULL)
				{
					pAdded->AutoDeleteData(true);
				}

				ret = true;
			}
		}
	}

	return ret;
}

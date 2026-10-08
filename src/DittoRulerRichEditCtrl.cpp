#include "stdafx.h"
#include "clip.h"
#include "CP_Main.h"
#include "..\shared\TextConvert.h"
#include ".\dittorulerricheditctrl.h"
#include "CopyProperties.h"
#include "ErrorReport.h"

CDittoRulerRichEditCtrl::CDittoRulerRichEditCtrl(void)
{	
	m_lID = -1;
}

CDittoRulerRichEditCtrl::~CDittoRulerRichEditCtrl(void)
{
}

bool CDittoRulerRichEditCtrl::LoadItem(long lID, CString csDesc)
{	
	bool bSetText = false;
	CClipFormat Clip;
	m_lID = lID;
	m_csDescription = csDesc;
	
	//If creating a new clip
	if(m_lID < 0)
	{
		m_rtf.SetModify(FALSE);
		return false;
	}

	// Registered formats are in 0xC000-0xFFFF, so they fit a CLIPFORMAT
	Clip.m_cfType = static_cast<CLIPFORMAT>(RegisterClipboardFormat(CF_RTF));
	if(HasClipData(lID, Clip))
	{
		CString cs(Clip.GetAsCStringA());
		SetRTF(cs);
		bSetText = true;		

		Clip.Free();
		Clip.Clear();
	}

	if(bSetText == false)
	{
		Clip.m_cfType = CF_UNICODETEXT;
		if(HasClipData(lID, Clip))
		{
			SetText(Clip.GetAsCString());
			bSetText = true;		

			Clip.Free();
			Clip.Clear();
		}
	}

	if(bSetText == false)
	{
		Clip.m_cfType = CF_TEXT;
		if(HasClipData(lID, Clip))
		{
			CString csText(Clip.GetAsCStringA());
			SetText(csText);

			bSetText = true;
		
			Clip.Free();
			Clip.Clear();
		}
	}

	m_rtf.SetModify(FALSE);

	return bSetText;
}

bool CDittoRulerRichEditCtrl::HasClipData(long lID, CClipFormat& Clip)
{
	return CClipDataReader(theApp.Services().Database()).GetClipData(lID, Clip) && Clip.m_hgData;
}

int CDittoRulerRichEditCtrl::SaveToDB(BOOL bUpdateDesc)
{
	int nRet = FALSE;

	if(m_rtf.GetModify() == FALSE)
	{
		CLogger::Log(_T("Clip has not been modified"));
		return DidntNeedToSave;
	}

	bool bSetModifyToFalse = true;
	try
	{
		//only save the types if they have them set as save types, mainly rtf type
		const std::unique_ptr<CClipTypes> pTypes{CClipDataReader(theApp.Services().Database()).LoadTypesFromDB()};
		if (!pTypes)
		{
			return FALSE; // LoadTypesFromDB reported the failure
		}

		int saveTypes{SaveTypesOf(*pTypes)};

		CClip Clip(theApp.Services().ClipContext());
		Clip.m_id = m_lID;
		LoadFormatsToSave(Clip, saveTypes);

		if(Clip.m_Formats.GetSize() <= 0)
		{
			return FALSE;
		}

		// no transaction around this: SaveFromEditWnd and AddToDB are each one transaction, and
		// upstream's transaction here stayed open while the properties dialog was shown
		const SaveClipResult saved{SaveClip(Clip, bUpdateDesc)};
		if(saved == SaveClipResult::Failed)
		{
			return FALSE;
		}
		bSetModifyToFalse = saved == SaveClipResult::Saved;

		nRet = SavedClipToDb;

		if(bUpdateDesc)
			theApp.Services().Windows().RefreshView();
	}
	catch (CppSQLite3Exception& e)
	{
		// the edit stays marked as modified, so it is not lost
		CErrorReport::Show(CStringUtil::Format(_T("Saving the edited clip id %ld failed: %s"), m_lID, e.errorMessage()));
		return FALSE;
	}

	if(bSetModifyToFalse)
		m_rtf.SetModify(FALSE);

	return nRet;
}

void CDittoRulerRichEditCtrl::LoadFormatsToSave(CClip& Clip, int saveTypes)
{
	if(saveTypes & stRTF)
	{
		LoadRTFData(Clip);
	}

	if(saveTypes & stCF_TEXT || saveTypes & stCF_UNICODETEXT)
	{
		LoadTextData(Clip);
	}
}

CDittoRulerRichEditCtrl::SaveClipResult CDittoRulerRichEditCtrl::SaveClip(CClip& Clip, BOOL& bUpdateDesc)
{
	if(m_lID >= 0)
	{
		// a failed save (shown by SaveFromEditWnd) keeps the edit marked as modified, so it is
		// not lost; upstream marked it saved
		return Clip.SaveFromEditWnd(bUpdateDesc) ? SaveClipResult::Saved : SaveClipResult::Failed;
	}

	return AddNewClip(Clip, bUpdateDesc);
}

CDittoRulerRichEditCtrl::SaveClipResult CDittoRulerRichEditCtrl::AddNewClip(CClip& Clip, BOOL& bUpdateDesc)
{
	if(Clip.MakeLatestOrder() == false)
	{
		return SaveClipResult::Failed;   // shown; the edit stays modified
	}
	CCopyProperties Prop(-1, this, &Clip);
	Prop.SetHandleKillFocus(true);
	Prop.SetToTopMost(false);
	if(Prop.DoModal() != IDOK)
	{
		return SaveClipResult::Cancelled;
	}
	// a failed save (shown by AddToDB) keeps the edit modified and the clip new; upstream
	// treated it like a cancelled dialog, so closing the tab lost the edit
	if(Clip.AddToDB() == false)
	{
		return SaveClipResult::Failed;
	}

	m_csDescription = Clip.m_Desc;
	m_lID = Clip.m_id;
	bUpdateDesc = TRUE;
	return SaveClipResult::Saved;
}

int CDittoRulerRichEditCtrl::SaveTypesOf(CClipTypes& types)
{
	int saveTypes{0};
	INT_PTR numTypes{types.GetSize()};
	for (int i = 0; i < numTypes; i++)
	{
		if (types.ElementAt(i) == theApp.Services().ClipboardFormats().Rtf())
		{
			saveTypes |= stRTF;
		}
		else if (types.ElementAt(i) == CF_TEXT ||
			types.ElementAt(i) == CF_UNICODETEXT)
		{
			saveTypes |= stCF_TEXT;
			saveTypes |= stCF_UNICODETEXT;
		}
	}

	return saveTypes;
}

bool CDittoRulerRichEditCtrl::LoadRTFData(CClip &Clip)
{
	CString csRTFOriginal = GetRTF();
	if(csRTFOriginal.IsEmpty())
	{
		CLogger::Log(_T("Rtf is empty, returning"));
		return false;
	}

	//remove the line feed at the end, not sure why the righ text always adds this
	CString right = csRTFOriginal.Right(9);
	if (right == _T("\\par\r\n}\r\n"))
	{
		CString r = csRTFOriginal.Left(csRTFOriginal.GetLength() - 9);
		r += _T("}");
		csRTFOriginal = r;
	}

	CStringA csRTF = CTextConvert::UnicodeToAnsi(csRTFOriginal);
	CClipFormat format;
	// Registered formats are in 0xC000-0xFFFF, so they fit a CLIPFORMAT
	format.m_cfType = static_cast<CLIPFORMAT>(RegisterClipboardFormat(_T("Rich Text Format")));
	int nLength = csRTF.GetLength() + 1;
	format.m_hgData = CGlobalMemory::NewGlobalP(csRTF.GetBuffer(nLength), nLength);
	Clip.m_Formats.Add(format);
	format.m_hgData = NULL; //Clip.m_formats owns data now

	return true;
}

bool CDittoRulerRichEditCtrl::LoadTextData(CClip &Clip)
{
	CString csText = GetText();
	if(csText.IsEmpty())
	{
		for(int i = 0; i < 20; i++)
		{
			Sleep(100);
			csText = GetText();
			if(csText.IsEmpty() == FALSE)
				break;

			CLogger::Log(CStringUtil::Format(_T("Get Text still empty pass = %d"), i));
		}
		if(csText.IsEmpty())
		{
			CLogger::Log(_T("Get Text still empty pass returning"));
			return false;
		}
	}

	CClipFormat format;

#ifdef _UNICODE
	format.m_cfType = CF_UNICODETEXT;
#else
	format.m_cfType = CF_TEXT;
#endif

	int nLength = csText.GetLength() * sizeof(TCHAR) + sizeof(TCHAR);
	format.m_hgData = CGlobalMemory::NewGlobalP(csText.GetBuffer(nLength), nLength);

	Clip.SetDescFromText(format.m_hgData, true);
	m_csDescription = Clip.m_Desc;
	m_csDescription = m_csDescription.Left(15);

	Clip.m_Formats.Add(format);
	format.m_hgData = NULL; //Clip.m_formats owns data now

	return true;
}

bool CDittoRulerRichEditCtrl::CloseEdit(bool bPrompt, BOOL bUpdateDesc)
{
	if(m_rtf.GetModify())
	{		
		int nRet = IDYES;
		
		if(bPrompt)
		{
			CString cs;
			cs.Format(_T("%s '%s'"), theApp.Services().Language().GetString("SaveChanges", "Do you want to save changes to").GetString(), m_csDescription.GetString());

			::SetForegroundWindow(m_hWnd);
			nRet = MessageBox(cs, _T("Ditto"), MB_YESNOCANCEL);
		}

		if(nRet == IDYES)
		{
			if(SaveToDB(bUpdateDesc) == false)
			{
				CString cs;
				cs.Format(_T("%s '%s'"), theApp.Services().Language().GetString("ErrorSaving", "Error saving clip").GetString(), m_csDescription.GetString());
				MessageBox(cs, _T("Ditto"), MB_OK);
				// the tab stays open with the unsaved edit; upstream closed it and lost the edit
				return false;
			}
		}
		else if(nRet == IDCANCEL)
		{
			return false;
		}
	}

	return true;
}
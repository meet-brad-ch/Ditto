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
	return theApp.GetClipData(lID, Clip) && Clip.m_hgData;
}

long CDittoRulerRichEditCtrl::GetTypeFlags(long lID)
{
	long lRet = stNONE;

	try
	{
		CLIPFORMAT cfType = CF_TEXT;
		CppSQLite3Query q = theApp.m_db.execQueryEx(_T("SELECT lID FROM Data WHERE lParentID = %d AND strClipboardFormat = '%s'"), lID, CClipboardFormats::GetFormatName(cfType).GetString());
		if(q.eof() == false)
		{
			lRet |= stCF_TEXT;
		}

		cfType = CF_UNICODETEXT;
		q = theApp.m_db.execQueryEx(_T("SELECT lID FROM Data WHERE lParentID = %d AND strClipboardFormat = '%s'"), lID, CClipboardFormats::GetFormatName(cfType).GetString());
		if(q.eof() == false)
		{
			lRet |= stCF_UNICODETEXT;
		}

		// Registered formats are in 0xC000-0xFFFF, so they fit a CLIPFORMAT
		cfType = static_cast<CLIPFORMAT>(RegisterClipboardFormat(_T("Rich Text Format")));
		q = theApp.m_db.execQueryEx(_T("SELECT lID FROM Data WHERE lParentID = %d AND strClipboardFormat = '%s'"), lID, CClipboardFormats::GetFormatName(cfType).GetString());
		if(q.eof() == false)
		{
			lRet |= stRTF;
		}
	}
	catch (CppSQLite3Exception& e)
	{
		CErrorReport::Show(CStringUtil::Format(_T("Reading the formats of clip id %ld failed: %s"), lID, e.errorMessage()));
		return stNONE;
	}

	return lRet;
}

void CDittoRulerRichEditCtrl::d()
{
	CString cs = m_rtf.GetText();
	CString s;
	s.Format(_T("error = %d, %s"), GetLastError(), cs.GetString());
	MessageBox(s);
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
		const std::unique_ptr<CClipTypes> pTypes{theApp.LoadTypesFromDB()};
		if (!pTypes)
		{
			return FALSE; // LoadTypesFromDB reported the failure
		}

		int saveTypes{SaveTypesOf(*pTypes)};

		CClip Clip;
		Clip.m_id = m_lID;
		LoadFormatsToSave(Clip, saveTypes);

		if(Clip.m_Formats.GetSize() <= 0)
		{
			return FALSE;
		}

		// no transaction around this: SaveFromEditWnd and AddToDB are each one transaction, and
		// upstream's transaction here stayed open while the properties dialog was shown
		if(m_lID >= 0)
		{
			Clip.SaveFromEditWnd(bUpdateDesc);
		}
		else
		{
			bSetModifyToFalse = AddNewClip(Clip, bUpdateDesc);
		}

		nRet = SavedClipToDb;

		if(bUpdateDesc)
			theApp.RefreshView();
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

bool CDittoRulerRichEditCtrl::AddNewClip(CClip& Clip, BOOL& bUpdateDesc)
{
	bool bSetModifyToFalse = false;
	Clip.MakeLatestOrder();
	CCopyProperties Prop(-1, this, &Clip);
	Prop.SetHandleKillFocus(true);
	Prop.SetToTopMost(false);
	if(Prop.DoModal() == IDOK)
	{
		Clip.AddToDB();
		m_csDescription = Clip.m_Desc;
		m_lID = Clip.m_id;
		bUpdateDesc = TRUE;
		bSetModifyToFalse = true;
	}

	return bSetModifyToFalse;
}

int CDittoRulerRichEditCtrl::SaveTypesOf(CClipTypes& types)
{
	int saveTypes{0};
	INT_PTR numTypes{types.GetSize()};
	for (int i = 0; i < numTypes; i++)
	{
		if (types.ElementAt(i) == theApp.m_RTFFormat)
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
			cs.Format(_T("%s '%s'"), theApp.m_Language.GetString("SaveChanges", "Do you want to save changes to").GetString(), m_csDescription.GetString());

			::SetForegroundWindow(m_hWnd);
			nRet = MessageBox(cs, _T("Ditto"), MB_YESNOCANCEL);
		}

		if(nRet == IDYES)
		{
			if(SaveToDB(bUpdateDesc) == false)
			{
				CString cs;
				cs.Format(_T("%s '%s'"), theApp.m_Language.GetString("ErrorSaving", "Error saving clip").GetString(), m_csDescription.GetString());
				MessageBox(cs, _T("Ditto"), MB_OK);
			}
		}
		else if(nRet == IDCANCEL)
		{
			return false;
		}
	}

	return true;
}
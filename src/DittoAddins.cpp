#include "stdafx.h"
#include ".\dittoaddins.h"
#include "misc.h"
#include "CP_Main.h"

CDittoAddins::CDittoAddins(CGetSetOptions& settings, CMultiLanguage& language, CAppWindows& windows) :
	m_settings(settings),
	m_language(language),
	m_windows(windows)
{
}

CDittoAddins::~CDittoAddins(void)
{
	UnloadAll();
}

bool CDittoAddins::UnloadAll()
{
	CLogger::Log(CStringUtil::Format(_T("Ditto Addin - Unloading all addins Count: %zu"), m_Addins.size()));

	// the menu lookups point into m_Addins
	m_FunctionMap.RemoveAll();
	m_Addins.clear();

	return true;
}

bool CDittoAddins::LoadAll()
{
	CDittoInfo DittoInfo;
	LoadDittoInfo(DittoInfo);

	CString csDir = m_settings.GetPath(CGetSetOptions::PathAddins);

	CFileFind find;
	BOOL bCont = find.FindFile(csDir + _T("*.dll"));

	while(bCont)
	{
		bCont = find.FindNextFile();

		CLogger::Log(CStringUtil::Format(_T("Ditto Addin - Trying to load addin file %s"), find.GetFilePath().GetString()));

		auto pAddin{std::make_unique<CDittoAddin>()};
		if(pAddin->DoLoad(find.GetFilePath(), DittoInfo))
		{
			CLogger::Log(CStringUtil::Format(_T("Ditto Addin - Success, loaded addin: %s"), find.GetFilePath().GetString()));
			m_Addins.push_back(std::move(pAddin));
		}
		else
		{
			// the failed addin is deleted at the end of this iteration
			CLogger::Log(CStringUtil::Format(_T("Ditto Addin - Failed loading Adding Error: %s"), pAddin->LastError().GetString()));
		}
	}

	return m_Addins.size() > 0;
}

bool CDittoAddins::AddPrePasteAddinsToMenu(CMenu *pMenu)
{
	bool bRet = false;

	m_FunctionMap.RemoveAll();
	int nMenuId = 3000;

	HMENU AllAddinsMenu = ::CreateMenu();

	for(const std::unique_ptr<CDittoAddin>& addin : m_Addins)
	{
		CDittoAddin *pAddin{addin.get()};
		if(pAddin)
		{
			INT_PTR subCount = pAddin->m_PrePasteFunctions.size();
			if(subCount > 1)
			{
				HMENU AddinMenu = ::CreateMenu();
				for(int x = 0; x < subCount; x++)
				{
					::AppendMenu(AddinMenu, MF_ENABLED, nMenuId, pAddin->m_PrePasteFunctions[x].m_csDisplayName);

					CFunctionLookup lookup;
					lookup.m_csFunctionName = pAddin->m_PrePasteFunctions[x].m_csFunction;
					lookup.m_pAddin = pAddin;
					m_FunctionMap.SetAt(nMenuId, lookup);
					nMenuId++;
				}

				::AppendMenu(AllAddinsMenu, MF_ENABLED|MF_POPUP, (UINT_PTR)AddinMenu, pAddin->DisplayName());
				bRet = true;
			}
			else if(subCount == 1)
			{
				//If there is only 1 function for this add in then just show one menu with addin name - function
				CFunctionLookup lookup;
				lookup.m_csFunctionName = pAddin->m_PrePasteFunctions[0].m_csFunction;
				lookup.m_pAddin = pAddin;
				m_FunctionMap.SetAt(nMenuId, lookup);

				CString menuName;
				menuName.Format(_T("%s - %s"), pAddin->DisplayName().GetString(), pAddin->m_PrePasteFunctions[0].m_csDisplayName.GetString());

				::AppendMenu(AllAddinsMenu, MF_ENABLED, nMenuId, menuName);
				bRet = true;
				nMenuId++;
			}
		}
	}

	if(bRet)
	{
		pMenu->InsertMenu(17, MF_BYPOSITION | MF_SEPARATOR);
		pMenu->InsertMenu(18, MF_BYPOSITION|MF_ENABLED|MF_STRING|MF_POPUP, (UINT_PTR)AllAddinsMenu, m_language.GetString("Add_Ins", "Add-Ins"));
	}

	return bRet;
}

bool CDittoAddins::CallPrePasteFunction(int Id, IClip *pClip)
{
	bool bRet = false;
	CFunctionLookup func;
	if(m_FunctionMap.Lookup(Id, func))
	{
		CDittoInfo DittoInfo;
		LoadDittoInfo(DittoInfo);

		bRet = func.m_pAddin->PrePasteFunction(DittoInfo, func.m_csFunctionName, pClip);
	}

	return bRet;
}

void CDittoAddins::LoadDittoInfo(CDittoInfo &DittoInfo)
{
	DittoInfo.m_csDatabasePath = m_settings.GetDBPath();
	DittoInfo.m_csLanguageCode = m_language.GetLangCode();
	DittoInfo.m_csSqliteVersion = sqlite3_libversion();
	DittoInfo.m_hWndDitto = m_windows.QPastehWnd();
}

void CDittoAddins::AboutScreenText(CStringArray &arr)
{
	for(const std::unique_ptr<CDittoAddin>& addin : m_Addins)
	{
		CDittoAddin *pAddin{addin.get()};
		if(pAddin)
		{
			CString csLine;
			csLine.Format(_T("%s Ver: %d, Ver2: %d"), pAddin->DisplayName().GetString(),pAddin->Version(), pAddin->PrivateVersion());
			arr.Add(csLine);
			INT_PTR subCount = pAddin->m_PrePasteFunctions.size();
			for(int x = 0; x < subCount; x++)
			{
				CString csLine2;
				csLine2.Format(_T("    %s (%s)"), pAddin->m_PrePasteFunctions[x].m_csDisplayName.GetString(), pAddin->m_PrePasteFunctions[x].m_csDetailDescription.GetString());
				arr.Add(csLine2);
			}
			arr.Add("");
		}
	}
}
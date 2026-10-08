// MultiLanguage.cpp: implementation of the CMultiLanguage class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "cp_main.h"
#include "MultiLanguage.h"
#include "..\Shared\TextConvert.h"
#include "..\Shared\TextConvert.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CMultiLanguage::CMultiLanguage()
{
	m_csAuthor = "";
	m_csLangCode = "";
	m_lFileVersion = 0;
	m_bOnlyGetHeader = false;
}

CMultiLanguage::~CMultiLanguage()
{
	ClearArrays();
}

void CMultiLanguage::ClearArrays()
{
	m_csAuthor = "";
	m_csLangCode = "";
	m_lFileVersion = 0;
	m_bOnlyGetHeader = false;

	ClearArray(m_RightClickMenu);
	ClearArray(m_GroupsRightClickMenu);
	ClearArray(m_ClipProperties);
	ClearArray(m_OptionsGeneral);
	ClearArray(m_OptionsSupportedTypes);
	ClearArray(m_OptionsShortcuts);
	ClearArray(m_OptionsQuickPaste);
	ClearArray(m_OptionsQuickPasteKeyboard);
	ClearArray(m_OptionsStats);
	ClearArray(m_OptionsSupportedTypesAdd);
	ClearArray(m_MoveToGroups);
	ClearArray(m_TrayIconRightClickMenu);
	ClearArray(m_OptionsSheet);
	ClearArray(m_OptionsCopyBuffers);
	ClearArray(m_GlobalHotKeys);

	ClearMap(m_StringMap);
}

void CMultiLanguage::ClearArray(LANGUAGE_ARRAY &Array)
{
	Array.clear();
}

void CMultiLanguage::ClearMap(LANGUAGE_MAP &Map)
{
	Map.clear();
}

CString CMultiLanguage::GetString(CString csID, CString csDefault)
{
	const LANGUAGE_MAP::const_iterator found{m_StringMap.find(csID)};
	if(found == m_StringMap.end())
	{
		return csDefault;
	}

	if(found->second.m_csForeignLang.GetLength() <= 0)
		return csDefault;

	return found->second.m_csForeignLang;
}

CString CMultiLanguage::GetGlobalHotKeyString(CString csID, CString csDefault)
{
	for(const CLangItem& item : m_GlobalHotKeys)
	{
		if(item.m_csID == csID)
		{
			return item.m_csForeignLang;
		}
	}

	return csDefault;
}

CString CMultiLanguage::GetDeleteClipDataString(CString csID, CString csDefault)
{
	for(const CLangItem& item : m_DeleteClipData)
	{
		if(item.m_csID == csID)
		{
			return item.m_csForeignLang;
		}
	}

	return csDefault;
}

CString CMultiLanguage::GetQuickPasteKeyboardString(int id, CString csDefault)
{
	for (const CLangItem& item : m_OptionsQuickPasteKeyboard)
	{
		if (item.m_nID == id)
		{
			return item.m_csForeignLang;
		}
	}

	return csDefault;
}

bool CMultiLanguage::UpdateRightClickMenu(CMenu *pMenu)
{
	return UpdateMenuToLanguage(pMenu, m_RightClickMenu);
}

bool CMultiLanguage::UpdateGroupsRightClickMenu(CMenu *pMenu)
{
	return UpdateMenuToLanguage(pMenu, m_GroupsRightClickMenu);
}

bool CMultiLanguage::UpdateTrayIconRightClickMenu(CMenu *pMenu)
{
	return UpdateMenuToLanguage(pMenu, m_TrayIconRightClickMenu);
}

bool CMultiLanguage::UpdateClipProperties(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_ClipProperties);
}

bool CMultiLanguage::UpdateOptionGeneral(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsGeneral);
}

bool CMultiLanguage::UpdateOptionSupportedTypes(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsSupportedTypes);
}

bool CMultiLanguage::UpdateOptionShortcuts(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsShortcuts);
}

bool CMultiLanguage::UpdateOptionQuickPaste(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsQuickPaste);
}

bool CMultiLanguage::UpdateOptionQuickPasteKeyboard(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsQuickPasteKeyboard);
}

bool CMultiLanguage::UpdateOptionStats(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsStats);
}

bool CMultiLanguage::UpdateOptionSupportedTypesAdd(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsSupportedTypesAdd);
}

bool CMultiLanguage::UpdateMoveToGroups(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_MoveToGroups);
}

bool CMultiLanguage::UpdateOptionsSheet(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsSheet);
}

bool CMultiLanguage::UpdateOptionCopyBuffers(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_OptionsCopyBuffers);
}

bool CMultiLanguage::UpdateGlobalHotKeys(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_GlobalHotKeys);
}

bool CMultiLanguage::UpdateDeleteClipData(CWnd *pParent)
{
	return UpdateWindowToLanguage(pParent, m_DeleteClipData);
}

bool CMultiLanguage::UpdateMenuToLanguage(CMenu *pMenu, LANGUAGE_ARRAY &Array)
{
	for(const CLangItem& item : Array)
	{
		if(item.m_csForeignLang.GetLength() > 0)
		{
			if(item.m_nID > 0)
			{
				pMenu->ModifyMenu(item.m_nID, MF_BYCOMMAND, item.m_nID, item.m_csForeignLang);
			}
			else
			{
				//If an item doesn't have a menu id then its a group menu
				//just search for the text and update the text with the foreign text
				int nMenuPos{};
				CMenu *pNewMenu = GetMenuPos(pMenu, item.m_csEnglishLang, nMenuPos);
				if(pNewMenu)
				{
					pNewMenu->ModifyMenu(nMenuPos, MF_BYPOSITION, static_cast<UINT_PTR>(-1), item.m_csForeignLang);
				}
			}
		}
	}

	return true;
}

bool CMultiLanguage::UpdateWindowToLanguage(CWnd *pParent, LANGUAGE_ARRAY &Array)
{
	for(const CLangItem& item : Array)
	{
		if(item.m_csForeignLang.GetLength() > 0)
		{
			if(item.m_nID > 0)
			{
				CWnd *pWnd = pParent->GetDlgItem(item.m_nID);
				if(pWnd)
				{
					pWnd->SetWindowText(item.m_csForeignLang);
				}
			}
			//If item id is -1 then set the title for the dialog
			else if(item.m_nID == -1)
			{
				pParent->SetWindowText(item.m_csForeignLang);
			}
		}
	}

	return true;
}


CMenu * CMultiLanguage::GetMenuPos(CMenu *pMenu, const CString &csLookingForMenuText, int &nMenuPos, bool returnChildIfOne)
{
	CMenu *pSubMenu;
	CString csMenuText;

	int nCount = pMenu->GetMenuItemCount();
	for(int i = 0; i < nCount; i++)
	{
		pMenu->GetMenuString(i, csMenuText, MF_BYPOSITION);

		if(csMenuText == csLookingForMenuText)
		{
			nMenuPos = i;
			if (returnChildIfOne)
			{
				pSubMenu = pMenu->GetSubMenu(i);
				if (pSubMenu != NULL)
				{
					return pSubMenu;
				}
			}
			return pMenu;
		}

		pSubMenu = pMenu->GetSubMenu(i);
		if(pSubMenu)
		{
			CMenu *pMenuReturn = GetMenuPos(pSubMenu, csLookingForMenuText, nMenuPos, returnChildIfOne);
			if(pMenuReturn)
				return pMenuReturn;
		}
	}

	return NULL;
}

bool CMultiLanguage::LoadLanguageFile(const CString& languageDir, CString csFile)
{
	m_csLastError = "";

	CString csPath = languageDir;
	csPath += csFile;
	csPath += ".xml";

	ClearArrays();

	if(csFile.GetLength() <= 0)
	{
		m_csLastError = "Language file is blank";
		return false;
	}

	// collapsed whitespace, as TinyXML (Ditto's earlier parser) read the language files
	tinyxml2::XMLDocument doc(true, tinyxml2::COLLAPSE_WHITESPACE);
	if(CXmlFile::Load(doc, csPath) != tinyxml2::XML_SUCCESS)
	{
		m_csLastError.Format(_T("Error loading file %s - reason = %s, Line: %d"), csFile.GetString(), CTextConvert::AnsiToUnicode(doc.ErrorStr()).GetString(), doc.ErrorLineNum());
		CLogger::Log(m_csLastError);
		return false;
	}

	tinyxml2::XMLElement *ItemHeader = doc.FirstChildElement("Ditto_Language_File");
	if(!ItemHeader)
	{
		m_csLastError.Format(_T("Error finding the section Ditto_Language_File"));
		ASSERT(!m_csLastError);
		CLogger::Log(m_csLastError);
		return false;
	}

	CString csVersion = ItemHeader->Attribute("Version");
	m_lFileVersion = _ttoi(csVersion);
	m_csAuthor = ItemHeader->Attribute("Author");
	m_csNotes = ItemHeader->Attribute("Notes");
	m_csLangCode = ItemHeader->Attribute("LanguageCode");

	if(m_bOnlyGetHeader)
		return true;

	bool bRet = LoadSection(*ItemHeader, m_RightClickMenu, "Ditto_Right_Click_Menu");
	bRet = LoadSection(*ItemHeader, m_GroupsRightClickMenu, "Ditto_Groups_Right_Click_Menu");
	bRet = LoadSection(*ItemHeader, m_OptionsGeneral, "Ditto_Options_General");
	bRet = LoadSection(*ItemHeader, m_ClipProperties, "Ditto_Clip_Properties");
	bRet = LoadSection(*ItemHeader, m_OptionsSupportedTypes, "Ditto_Options_Supported_Types");
	bRet = LoadSection(*ItemHeader, m_OptionsShortcuts, "Ditto_Options_Shortcuts");
	bRet = LoadSection(*ItemHeader, m_OptionsQuickPaste, "Ditto_Options_Quick_Paste");
	bRet = LoadSection(*ItemHeader, m_OptionsQuickPasteKeyboard, "Ditto_Options_Quick_Paste_Keyboard");
	bRet = LoadSection(*ItemHeader, m_OptionsStats, "Ditto_Options_Stats");
	bRet = LoadSection(*ItemHeader, m_OptionsSupportedTypesAdd, "Ditto_Options_Supported_Types_Add");
	bRet = LoadSection(*ItemHeader, m_MoveToGroups, "Ditto_Move_To_Groups");
	bRet = LoadSection(*ItemHeader, m_OptionsSheet, "Ditto_Options_Sheet");
	bRet = LoadSection(*ItemHeader, m_TrayIconRightClickMenu, "Ditto_Tray_Icon_Menu");
	bRet = LoadSection(*ItemHeader, m_OptionsCopyBuffers, "Ditto_Options_CopyBuffers");
	bRet = LoadSection(*ItemHeader, m_GlobalHotKeys, "Ditto_GlobalHotKeys");
	bRet = LoadSection(*ItemHeader, m_DeleteClipData, "Ditto_DeleteClipData");
	
	bRet = LoadStringTableSection(*ItemHeader, m_StringMap, "Ditto_String_Table");

	return true;
}

bool CMultiLanguage::LoadSection(const tinyxml2::XMLElement &doc, LANGUAGE_ARRAY &Array, CString csSection)
{
	CStringA csSectionA = CTextConvert::UnicodeToAnsi(csSection);
	const tinyxml2::XMLElement *node = doc.FirstChildElement(csSectionA);
	if(!node)
	{
		m_csLastError.Format(_T("Error finding the section %s"), csSection.GetString());
		//ASSERT(!m_csLastError);
		CLogger::Log(m_csLastError);
		return false;
	}

	const tinyxml2::XMLNode* ForeignNode{};
	CString csID;
	CString csLineFeed("\n");

	const tinyxml2::XMLElement *ItemElement = node->FirstChildElement();

	//load all items for this section
	//they look like
	//<Item English_Text = "Use Ctrl - Num" ID= "32777"></Item>
	while(ItemElement)
	{
 		ForeignNode = ItemElement->FirstChild();
 		if(ForeignNode)
 		{
			CLangItem item{};
			item.m_csEnglishLang = ItemElement->Attribute("English_Text");
			csID = ItemElement->Attribute("ID");
			item.m_nID = _ttoi(csID);
			if(item.m_nID == 0)
			{
				item.m_csID = csID;
			}

			LPCSTR Value = ForeignNode->Value();
			item.m_csForeignLang = CTextConvert::Utf8ToUnicode(Value);

			//Replace the literal "\n" with line feeds
 			item.m_csForeignLang.Replace(_T("\\n"), csLineFeed);

			Array.push_back(item);
 		}		

		ItemElement = ItemElement->NextSiblingElement();
	}
	
	return true;
}

bool CMultiLanguage::LoadStringTableSection(const tinyxml2::XMLElement &doc, LANGUAGE_MAP &Map, CString csSection)
{
	CStringA csSectionA = CTextConvert::UnicodeToAnsi(csSection);
	const tinyxml2::XMLElement *node = doc.FirstChildElement(csSectionA);
	if(!node)
	{
		CString cs;
		cs.Format(_T("Error finding the section %s"), csSection.GetString());
		ASSERT(!cs);
		CLogger::Log(cs);
		return false;
	}

	CString csLineFeed("\n");
	const tinyxml2::XMLNode* ForeignNode{};

	const tinyxml2::XMLElement *ItemElement = node->FirstChildElement();

	//load all items for this section
	//they look like
	//<Item English_Text = "Use Ctrl - Num" ID= "32777"></Item>
	while(ItemElement)
	{
		CLangItem item{};

		item.m_csEnglishLang = ItemElement->Attribute("English_Text");
		item.m_csID = ItemElement->Attribute("ID");

		ForeignNode = ItemElement->FirstChild();
		if(ForeignNode)
		{
			LPCSTR Value = ForeignNode->Value();
			item.m_csForeignLang = CTextConvert::Utf8ToUnicode(Value);

			//Replace the literal "\n" with line feeds
			item.m_csForeignLang.Replace(_T("\\n"), csLineFeed);
		}

		// a repeated ID replaces the earlier item
		Map.insert_or_assign(item.m_csID, item);

		ItemElement = ItemElement->NextSiblingElement();
	}

	return true;
}

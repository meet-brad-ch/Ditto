#include "stdafx.h"
#include ".\theme.h"
#include "..\Shared\TextConvert.h"
#include "Misc.h"
#include "Options.h"
#include "..\Shared\Tokenizer.h"
#include "CP_Main.h"

CTheme::CTheme(void)
{
	m_lFileVersion = 0;
	m_LastWriteTime = 0;
	m_lastTheme = _T("");

	LoadDefaults();
}

CTheme::~CTheme(void)
{
}


void CTheme::LoadDefaults()
{
	m_CaptionLeft = RGB(255, 255, 255);
	m_CaptionRight = RGB(204, 204, 204);

	m_Border = RGB(204, 204, 204);
	m_BorderTopMost = RGB(204, 204, 204);
	m_BorderNotConnected = RGB(204, 204, 204);

	m_CaptionLeftTopMost = RGB(255, 255, 255);
	m_CaptionRightTopMost = RGB(204, 204, 204);
	
	m_CaptionLeftNotConnected = RGB(255, 255, 255);
	m_CaptionRightNotConnected = RGB(255, 255, 0);

	m_CaptionTextColor = RGB(191, 191, 191);
	m_ListBoxOddRowsBG = RGB(255, 255, 255);
	m_ListBoxEvenRowsBG = RGB(243, 243, 243);
	m_ListBoxOddRowsText = RGB(0, 0, 0);
	m_ListBoxEvenRowsText = RGB(0, 0, 0);
	m_ListBoxSelectedBG = RGB(204, 204, 204);
	m_ListBoxSelectedNoFocusBG = RGB(204, 204, 204);
	m_ListBoxSelectedText = RGB(0, 0, 0);
	m_ListBoxSelectedNoFocusText = RGB(0, 0, 0);
	m_clipPastedColor = RGB(0, 255, 0);
	m_listSmallQuickPasteIndexColor = RGB(180, 180, 180);
	m_mainWindowBG = RGB(240, 240, 240);
	m_searchTextBoxFocusBG = RGB(255, 255, 255);
	m_searchTextBoxFocusText = RGB(0, 0, 0);
	m_searchTextBoxFocusBorder = RGB(255, 255, 255);
	m_searchTextHighlight = RGB(255, 0, 0);

	m_groupTreeBG = RGB(240, 240, 240);
	m_groupTreeText = RGB(127, 127, 127);

	m_descriptionWindowBG = RGB(240, 240, 240);// GetSysColor(COLOR_INFOBK);//RGB(240, 240, 240);//
	/*int r = GetRValue(m_descriptionWindowBG);
	int g = GetGValue(m_descriptionWindowBG);
	int b = GetBValue(m_descriptionWindowBG);*/

	m_descriptionWindowText = RGB(0, 0, 0);

	// Modern scrollbar defaults - rounded look
	m_scrollBarThumb = RGB(180, 180, 180);
	m_scrollBarThumbHover = RGB(140, 140, 140);
	m_scrollBarTrack = RGB(240, 240, 240);

	m_captionSize = 25;
	m_captionFontSize = 19;
}

bool CTheme::Load(CGetSetOptions& settings, CString csTheme, bool bHeaderOnly, bool bCheckLastWriteTime)
{
	const bool followWindows10Theme = csTheme.IsEmpty();
	if (followWindows10Theme)
	{
		csTheme = GetWindowsThemeName();
	}

	if (IsDefaultThemeName(csTheme))
	{
		return LoadDefaultTheme(followWindows10Theme);
	}

	CString csPath = settings.GetPath(CGetSetOptions::PathThemes);
	csPath += csTheme;
	csPath += ".xml";

	__int64 LastWrite = CFileSystem::GetLastWriteTime(csPath);

	if(bCheckLastWriteTime)
	{	
		if(m_lastTheme == csTheme &&
			LastWrite == m_LastWriteTime)
		{
			return true;
		}
	}

	LoadDefaults();

	m_LastWriteTime = LastWrite;
	m_lastTheme = csTheme;

	CLogger::Log(CStringUtil::Format(_T("Loading Theme %s"), csPath.GetString()));

	return LoadThemeFile(csPath, bHeaderOnly, followWindows10Theme);
}

CString CTheme::GetWindowsThemeName()
{
	if (CSystemTheme::DarkAppWindows10Setting())
	{
		CLogger::Log(_T("Loading theme based on windows setting of dark mode for apps"));
		return _T("DarkerDitto");
	}
	return _T("");
}

bool CTheme::IsDefaultThemeName(const CString& csTheme)
{
	return csTheme.IsEmpty() || csTheme == _T("Ditto") || csTheme == _T("(Default)") || csTheme == _T("(Ditto)");
}

bool CTheme::LoadDefaultTheme(bool followWindows10Theme)
{
	LoadDefaults();

	if (followWindows10Theme)
	{
		LoadWindowsAccentColor();
	}

	m_LastWriteTime = 0;
	m_lastTheme = _T("");

	CLogger::Log(_T("Loading default ditto values for themes"));

	return false;
}

bool CTheme::LoadThemeFile(const CString& csPath, bool bHeaderOnly, bool followWindows10Theme)
{
	// collapsed whitespace, as TinyXML (Ditto's earlier parser) read the theme files
	tinyxml2::XMLDocument doc(true, tinyxml2::COLLAPSE_WHITESPACE);
	if(CXmlFile::Load(doc, csPath) != tinyxml2::XML_SUCCESS)
	{
		m_csLastError.Format(_T("Error loading Theme %s - reason = %hs"), csPath.GetString(), doc.ErrorStr());
		ASSERT(!m_csLastError);
		CLogger::Log(m_csLastError);
		return false;
	}

	const tinyxml2::XMLElement *ItemHeader = doc.FirstChildElement("Ditto_Theme_File");
	if(!ItemHeader)
	{
		m_csLastError.Format(_T("Error finding the section Ditto_Theme_File"));
		ASSERT(!m_csLastError);
		CLogger::Log(m_csLastError);
		return false;
	}

	CString csVersion = ItemHeader->Attribute("Version");
	m_lFileVersion = _ttoi(csVersion);
	m_csAuthor = ItemHeader->Attribute("Author");
	m_csNotes = ItemHeader->Attribute("Notes");

	if(bHeaderOnly)
		return true;


	LoadThemeValues(ItemHeader);

	if (followWindows10Theme)
	{
		LoadWindowsAccentColor();
	}

	return true;
}

void CTheme::LoadThemeValues(const tinyxml2::XMLElement *ItemHeader)
{
	LoadColor(ItemHeader, "CaptionLeft", m_CaptionLeft);
	LoadColor(ItemHeader, "CaptionRight", m_CaptionRight);
	LoadColor(ItemHeader, "CaptionLeftTopMost", m_CaptionLeftTopMost);
	LoadColor(ItemHeader, "CaptionRightTopMost", m_CaptionRightTopMost);
	LoadColor(ItemHeader, "CaptionLeftNotConnected", m_CaptionLeftNotConnected);
	LoadColor(ItemHeader, "CaptionRightNotConnected", m_CaptionRightNotConnected);
	LoadColor(ItemHeader, "CaptionTextColor", m_CaptionTextColor);
	LoadColor(ItemHeader, "ListBoxOddRowsBG", m_ListBoxOddRowsBG);
	LoadColor(ItemHeader, "ListBoxEvenRowsBG", m_ListBoxEvenRowsBG);
	LoadColor(ItemHeader, "ListBoxOddRowsText", m_ListBoxOddRowsText);
	LoadColor(ItemHeader, "ListBoxEvenRowsText", m_ListBoxEvenRowsText);
	LoadColor(ItemHeader, "ListBoxSelectedBG", m_ListBoxSelectedBG);
	LoadColor(ItemHeader, "ListBoxSelectedNoFocusBG", m_ListBoxSelectedNoFocusBG);
	LoadColor(ItemHeader, "ListBoxSelectedText", m_ListBoxSelectedText);
	LoadColor(ItemHeader, "ListBoxSelectedNoFocusText", m_ListBoxSelectedNoFocusText);
	LoadColor(ItemHeader, "ClipPastedColor", m_clipPastedColor);
	LoadColor(ItemHeader, "MainWindowBG", m_mainWindowBG);
	LoadColor(ItemHeader, "SearchTextBoxFocusBG", m_searchTextBoxFocusBG);
	LoadColor(ItemHeader, "SearchTextBoxFocusText", m_searchTextBoxFocusText);
	LoadColor(ItemHeader, "SearchTextBoxFocusBorder", m_searchTextBoxFocusBorder);
	LoadColor(ItemHeader, "SearchTextHighlight", m_searchTextHighlight);

	LoadColor(ItemHeader, "Border", m_Border);
	LoadColor(ItemHeader, "BorderTopMost", m_BorderTopMost);
	LoadColor(ItemHeader, "BorderNotConnected", m_BorderNotConnected);

	LoadColor(ItemHeader, "GroupTreeBG", m_groupTreeBG);
	LoadColor(ItemHeader, "GroupTreeText", m_groupTreeText);
	
	LoadInt(ItemHeader, "CaptionSize", m_captionSize);
	LoadInt(ItemHeader, "CaptionFontSize", m_captionFontSize);

	LoadColor(ItemHeader, "DescriptionWindowBG", m_descriptionWindowBG);
	LoadColor(ItemHeader, "DescriptionWindowText", m_descriptionWindowText);

	// Modern scrollbar colors
	LoadColor(ItemHeader, "ScrollBarThumb", m_scrollBarThumb);
	LoadColor(ItemHeader, "ScrollBarThumbHover", m_scrollBarThumbHover);
	LoadColor(ItemHeader, "ScrollBarTrack", m_scrollBarTrack);
}

void CTheme::LoadWindowsAccentColor()
{
	DWORD accent = CSystemTheme::Windows10AccentColor();
	if (accent != -1)
	{
		//windows seems to be bgr, convert to rgb
		auto r = GetRValue(accent);
		auto g = GetGValue(accent);
		auto b = GetBValue(accent);

		m_clipPastedColor = RGB(b, g, r);
		m_searchTextBoxFocusBorder = m_clipPastedColor;
		m_searchTextHighlight = m_clipPastedColor;

		//if (CSystemTheme::Windows10ColorTitleBar())
		//{
		//	m_CaptionRight = m_clipPastedColor;
		//	m_CaptionLeft = m_clipPastedColor;
		//	m_Border = m_clipPastedColor;
		//}
	}
}

COLORREF CTheme::HslToRgb(float h, float s, float l)
{
	if (s == 0.0f)
	{
		// Grayscale, achromatic
		BYTE gray = static_cast<BYTE>(l * 255.0f + 0.5f);
		return RGB(gray, gray, gray);
	}

	auto hueToRgb = [](float p, float q, float t) -> float
	{
		if (t < 0.0f) t += 1.0f;
		if (t > 1.0f) t -= 1.0f;
		if (t < 1.0f / 6.0f) return p + (q - p) * 6.0f * t;
		if (t < 1.0f / 2.0f) return q;
		if (t < 2.0f / 3.0f) return p + (q - p) * (2.0f / 3.0f - t) * 6.0f;
		return p;
	};

	float q = l < 0.5f ? l * (1.0f + s) : l + s - l * s;
	float p = 2.0f * l - q;
	float h_norm = h / 360.0f;

	float r_f = hueToRgb(p, q, h_norm + 1.0f / 3.0f);
	float g_f = hueToRgb(p, q, h_norm);
	float b_f = hueToRgb(p, q, h_norm - 1.0f / 3.0f);

	BYTE r = static_cast<BYTE>(r_f * 255.0f + 0.5f);
	BYTE g = static_cast<BYTE>(g_f * 255.0f + 0.5f);
	BYTE b = static_cast<BYTE>(b_f * 255.0f + 0.5f);

	return RGB(r, g, b);
}

bool CTheme::LoadColor(const tinyxml2::XMLElement *pParent, CStringA csNode, COLORREF &Color)
{
	int intValue = 0;
	return LoadElement(pParent, csNode, Color, intValue);
}

bool CTheme::LoadInt(const tinyxml2::XMLElement *pParent, CStringA csNode, int &intValue)
{
	COLORREF colorValue = 0;
	return LoadElement(pParent, csNode, colorValue, intValue);
}

bool CTheme::LoadElement(const tinyxml2::XMLElement *pParent, CStringA csNode, COLORREF &Color, int &intValue)
{
	const tinyxml2::XMLElement *pColorNode = pParent->FirstChildElement(csNode);
	if(pColorNode == NULL)
	{
		m_csLastError.Format(_T("Theme Load, error loading Node = %hs"), csNode.GetString());
		CLogger::Log(m_csLastError);
		return false;
	}

	const tinyxml2::XMLNode *pColor = pColorNode->FirstChild();
	if(pColor == NULL)
	{
		m_csLastError.Format(_T("Theme Load, error getting node text for = %hs"), csNode.GetString());
		CLogger::Log(m_csLastError);
		return false;
	}
	
	CString csColor = pColor->Value();
	csColor.Trim();

	if (csColor.IsEmpty())
	{
		return false;
	}

	if (HasColorPrefix(csColor, _T("rgb(")))
	{
		return ParseRgbValue(csNode, csColor, Color);
	}
	else if (HasColorPrefix(csColor, _T("hsl(")))
	{
		return ParseHslValue(csNode, csColor, Color);
	}
	else if (csColor.GetAt(0) == _T('#') && csColor.GetLength() == 7)
	{
		long r = _tcstol(csColor.Mid(1, 2), NULL, 16);
		long g = _tcstol(csColor.Mid(3, 2), NULL, 16);
		long b = _tcstol(csColor.Mid(5, 2), NULL, 16);

		Color = RGB(r, g, b);
	}
	else
	{
		intValue = _ttoi(csColor);
		Color = (COLORREF)intValue;
	}

	return true;
}

bool CTheme::HasColorPrefix(const CString& csColor, LPCTSTR prefix)
{
	return csColor.GetLength() > 4 && csColor.Left(4).CompareNoCase(prefix) == 0;
}

bool CTheme::ParseRgbValue(const CStringA& csNode, const CString& csColor, COLORREF &Color)
{
	CString values = csColor.Mid(4, csColor.GetLength() - 5);
	values.Trim();

	CTokenizer token(values, _T(", "));
	CString csR, csG, csB;

	token.Next(csR);
	token.Next(csG);
	token.Next(csB);

	csR.Trim();
	csG.Trim();
	csB.Trim();

	if (!csR.IsEmpty() && csG.IsEmpty() && csB.IsEmpty())
	{
		Color = _ttoi(csR);
	}
	else if (!csR.IsEmpty() && !csG.IsEmpty() && !csB.IsEmpty())
	{
		Color = RGB(_ttoi(csR), _ttoi(csG), _ttoi(csB));
	}
	else
	{
		m_csLastError.Format(_T("Theme Load, malformed/incomplete RGB value for Node = %hs, Value = %s"), csNode.GetString(), csColor.GetString());
		CLogger::Log(m_csLastError);
		return false;
	}
	return true;
}

bool CTheme::ParseHslValue(const CStringA& csNode, const CString& csColor, COLORREF &Color)
{
	CString values = csColor.Mid(4, csColor.GetLength() - 5);
	values.Trim();

	CTokenizer token(values, _T(", %"));
	CString csH, csS, csL;

	token.Next(csH);
	token.Next(csS);
	token.Next(csL);

	csH.Trim();
	csS.Trim();
	csL.Trim();

	if (!csH.IsEmpty() && !csS.IsEmpty() && !csL.IsEmpty())
	{
		float h = (float)_tstof(csH);
		float s = (float)_tstof(csS);
		float l = (float)_tstof(csL);

		s = max(0.0f, min(100.0f, s)) / 100.0f;
		l = max(0.0f, min(100.0f, l)) / 100.0f;

		Color = HslToRgb(h, s, l);
	}
	else
	{
		m_csLastError.Format(_T("Theme Load, malformed/incomplete HSL value for Node = %hs, Value = %s"), csNode.GetString(), csColor.GetString());
		CLogger::Log(m_csLastError);
		return false;
	}
	return true;
}
#pragma once

#include "XmlFile.h"

class CTheme
{
public:
	CTheme(void);
	~CTheme(void);

	bool Load(CString csTheme, bool bHeaderOnly = false, bool bCheckLastWriteTime = false);	

	COLORREF CaptionLeft() const { return m_CaptionLeft; }
	COLORREF CaptionRight() const { return m_CaptionRight; }
	COLORREF Border() const { return m_Border; }
	COLORREF BorderTopMost() const { return m_BorderTopMost; }
	COLORREF BorderNotConnected() const { return m_BorderNotConnected; }
	COLORREF CaptionLeftTopMost() const { return m_CaptionLeftTopMost; }
	COLORREF CaptionRightTopMost() const { return m_CaptionRightTopMost; }
	COLORREF CaptionLeftNotConnected() const { return m_CaptionLeftNotConnected; }
	COLORREF CaptionRightNotConnected() const { return m_CaptionRightNotConnected; }
	COLORREF CaptionTextColor() const { return m_CaptionTextColor; }
	
	COLORREF ListBoxOddRowsBG() const { return m_ListBoxOddRowsBG; }
	COLORREF ListBoxEvenRowsBG() const { return m_ListBoxEvenRowsBG; }
	COLORREF ListBoxOddRowsText() const { return m_ListBoxOddRowsText; }
	COLORREF ListBoxEvenRowsText() const { return m_ListBoxEvenRowsText; }
	COLORREF ListBoxSelectedBG() const { return m_ListBoxSelectedBG; }
	COLORREF ListBoxSelectedNoFocusBG() const { return m_ListBoxSelectedNoFocusBG; }
	COLORREF ListBoxSelectedText() const { return m_ListBoxSelectedText; }
	COLORREF ListBoxSelectedNoFocusText() const { return m_ListBoxSelectedNoFocusText; }
	COLORREF ClipPastedColor() const { return m_clipPastedColor; }

	COLORREF ListSmallQuickPasteIndexColor() const { return m_listSmallQuickPasteIndexColor;  }
	COLORREF MainWindowBG() const { return m_mainWindowBG; }
	COLORREF SearchTextBoxFocusBG() const { return m_searchTextBoxFocusBG; }
	COLORREF SearchTextBoxFocusText() const { return m_searchTextBoxFocusText; }
	COLORREF SearchTextBoxFocusBorder() const { return m_searchTextBoxFocusBorder; }
	COLORREF SearchTextHighlight() const { return m_searchTextHighlight; }

	COLORREF GroupTreeBG() const { return m_groupTreeBG; }
	COLORREF GroupTreeText() const { return m_groupTreeText; }

	int GetCaptionSize() const { return m_captionSize; }
	int GetCaptionFontSize() const { return m_captionFontSize; }

	COLORREF DescriptionWindowBG() const { return m_descriptionWindowBG; }
	COLORREF DescriptionWindowText() const { return m_descriptionWindowText; }

	// Modern scrollbar colors
	COLORREF ScrollBarThumb() const { return m_scrollBarThumb; }
	COLORREF ScrollBarThumbHover() const { return m_scrollBarThumbHover; }
	COLORREF ScrollBarTrack() const { return m_scrollBarTrack; }

	CString Notes() const { return m_csNotes; }
	CString Author() const { return m_csAuthor; }
	long FileVersion() const { return m_lFileVersion; }

	CString LastError() const { return m_csLastError; }

protected:
	bool LoadElement(const tinyxml2::XMLElement *pParent, CStringA csNode, COLORREF &Color, int &intValue);

	bool LoadInt(const tinyxml2::XMLElement *pParent, CStringA csNode, int &intValue);
	bool LoadColor(const tinyxml2::XMLElement *pParent, CStringA csNode, COLORREF &Color);
	void LoadWindowsAccentColor();

private:
	/**
	 * @brief Load's step for an empty theme name: the theme that follows the Windows app mode.
	 * @return "DarkerDitto" when Windows apps use dark mode, an empty name otherwise.
	 */
	static CString GetWindowsThemeName();
	/**
	 * @brief Tells whether a theme name means the built-in Ditto defaults.
	 * @param csTheme the theme name.
	 * @return true for "", "Ditto", "(Default)" and "(Ditto)".
	 */
	static bool IsDefaultThemeName(const CString& csTheme);
	/**
	 * @brief Load's step for the default theme: loads the defaults and forgets the last theme file.
	 * @param followWindows10Theme true to take the Windows accent colour.
	 * @return false (no theme file was loaded).
	 */
	bool LoadDefaultTheme(bool followWindows10Theme);
	/**
	 * @brief Load's step: reads the theme XML file header and (unless bHeaderOnly) its values.
	 * @param csPath the theme file path.
	 * @param bHeaderOnly true to read only version, author and notes.
	 * @param followWindows10Theme true to take the Windows accent colour after the values.
	 * @return false if the file could not be read or has no Ditto_Theme_File section.
	 */
	bool LoadThemeFile(const CString& csPath, bool bHeaderOnly, bool followWindows10Theme);
	/**
	 * @brief Loads all colour and size values of the theme file.
	 * @param ItemHeader the Ditto_Theme_File element.
	 */
	void LoadThemeValues(const tinyxml2::XMLElement *ItemHeader);
	/**
	 * @brief Tells whether a colour text starts with a 4 character function prefix.
	 * @param csColor the trimmed colour text.
	 * @param prefix the prefix, "rgb(" or "hsl(" (compared without case).
	 * @return true if csColor is longer than 4 characters and starts with prefix.
	 */
	static bool HasColorPrefix(const CString& csColor, LPCTSTR prefix);
	/**
	 * @brief Parses an "rgb(r, g, b)" (or "rgb(value)") colour text.
	 * @param csNode the node name (for the error text).
	 * @param csColor the colour text.
	 * @param Color receives the colour.
	 * @return false (and sets m_csLastError) for a malformed value.
	 */
	bool ParseRgbValue(const CStringA& csNode, const CString& csColor, COLORREF &Color);
	/**
	 * @brief Parses an "hsl(h, s%, l%)" colour text.
	 * @param csNode the node name (for the error text).
	 * @param csColor the colour text.
	 * @param Color receives the colour.
	 * @return false (and sets m_csLastError) for a malformed value.
	 */
	bool ParseHslValue(const CStringA& csNode, const CString& csColor, COLORREF &Color);
	/**
	 * @brief Converts an HSL colour to RGB.
	 * @param h the hue in degrees, 0-360.
	 * @param s the saturation, 0-1.
	 * @param l the lightness, 0-1.
	 * @return the RGB colour.
	 */
	static COLORREF HslToRgb(float h, float s, float l);

protected:
	COLORREF m_CaptionLeft;
	COLORREF m_CaptionRight;
	COLORREF m_CaptionLeftTopMost;
	COLORREF m_CaptionRightTopMost;
	COLORREF m_CaptionLeftNotConnected;
	COLORREF m_CaptionRightNotConnected;
	COLORREF m_CaptionTextColor;

	COLORREF m_ListBoxOddRowsBG;
	COLORREF m_ListBoxEvenRowsBG;
	COLORREF m_ListBoxOddRowsText;
	COLORREF m_ListBoxEvenRowsText;
	COLORREF m_ListBoxSelectedBG;
	COLORREF m_ListBoxSelectedNoFocusBG;
	COLORREF m_ListBoxSelectedText;
	COLORREF m_ListBoxSelectedNoFocusText;	
	COLORREF m_clipPastedColor;
	COLORREF m_listSmallQuickPasteIndexColor;
	COLORREF m_mainWindowBG;
	COLORREF m_Border;
	COLORREF m_BorderTopMost;
	COLORREF m_BorderNotConnected;
	COLORREF m_searchTextBoxFocusBG;
	COLORREF m_searchTextBoxFocusText;
	COLORREF m_searchTextBoxFocusBorder;
	COLORREF m_searchTextHighlight;

	COLORREF m_groupTreeBG;
	COLORREF m_groupTreeText;

	COLORREF m_descriptionWindowBG;
	COLORREF m_descriptionWindowText;

	// Modern scrollbar colors
	COLORREF m_scrollBarThumb;
	COLORREF m_scrollBarThumbHover;
	COLORREF m_scrollBarTrack;

	int m_captionSize;
	int m_captionFontSize;

	CString m_csLastError;
	long m_lFileVersion;
	CString m_csAuthor;
	CString m_csNotes;

	__int64 m_LastWriteTime;
	CString m_lastTheme;

	void LoadDefaults();
};

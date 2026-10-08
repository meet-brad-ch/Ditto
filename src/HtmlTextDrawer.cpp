/* HtmlTextDrawer (formerly DrawHTML())
 * Drop-in replacement for DrawText() supporting a tiny subset of HTML.
 */
#include "stdafx.h"
#include "HtmlTextDrawer.h"
#include <climits>
#include <tchar.h>

//use unprintable characters so it doesn't find copied html to convert

const std::array<HtmlTextDrawer::TagInfo, 3> HtmlTextDrawer::s_tags{ {
	{ nullptr, tNONE, 0, 0 },
	{ _T("\x04"), tFONT, 1, 0 },
	{ _T("\x05"), tBR, 0, 1 },
	/*{ _T("b"),      tB,    0, 0},
	{ _T("br"),     tBR,   0, 1},
	{ _T("em"),     tI,    0, 0},
	{ _T("font"),   tFONT, 1, 0},
	{ _T("i"),      tI,    0, 0},
	{ _T("p"),      tP,    0, 1},
	{ _T("strong"), tB,    0, 0},
	{ _T("sub"),    tSUB,  0, 0},
	{ _T("sup"),    tSUP,  0, 0},
	{ _T("u"),      tU,    0, 0},*/
} };

const std::array<int, HtmlTextDrawer::tNUMTAGS> HtmlTextDrawer::s_styleFlags{
	0,             // tNONE
	s_bold,        // tB
	0,             // tBR
	0,             // tFONT
	s_italic,      // tI
	0,             // tP
	s_subscript,   // tSUB
	s_superscript, // tSUP
	s_underline,   // tU
};

int HtmlTextDrawer::GetToken(LPCTSTR* String, int* Size, int* TokenLength, BOOL* WhiteSpace)
{
	ASSERT(String != NULL && *String != NULL);
	ASSERT(Size != NULL);

	/* check for leading white space, then skip it */
	const BOOL EntryWhiteSpace{ ReadEntryWhiteSpace(*String, WhiteSpace) };
	TokenCursor cursor{ *String, *String, 0, *Size };
	SkipLeadingWhiteSpace(cursor);
	*Size = cursor.size;
	if (cursor.size <= 0)
		return -1; /* no printable text left */

	cursor.end = cursor.start;
	int Index{ 0 };
	int IsEndTag{ 0 };
	if (*cursor.end == _T('\x01'))
		Index = ScanTag(cursor, WhiteSpace, EntryWhiteSpace, IsEndTag);
	else
		ScanWord(cursor); /* normal word (no tag) */

	if (TokenLength != NULL)
		*TokenLength = cursor.length;
	*Size -= cursor.length;
	*String = cursor.start;
	return s_tags[Index].token | IsEndTag;
}

BOOL HtmlTextDrawer::ReadEntryWhiteSpace(LPCTSTR Start, BOOL* WhiteSpace)
{
	if (WhiteSpace == NULL)
		return FALSE;

	const BOOL EntryWhiteSpace{ *WhiteSpace };
	*WhiteSpace = EntryWhiteSpace || _istspace(*Start);
	return EntryWhiteSpace;
}

void HtmlTextDrawer::SkipLeadingWhiteSpace(TokenCursor& cursor)
{
	while (cursor.size > 0 && _istspace(*cursor.start))
	{
		cursor.start++;
		cursor.size -= 1;
	} /* while */
}

void HtmlTextDrawer::Advance(TokenCursor& cursor)
{
	cursor.end++;
	cursor.length++;
}

int HtmlTextDrawer::ScanTag(TokenCursor& cursor, BOOL* WhiteSpace, BOOL EntryWhiteSpace, int& IsEndTag)
{
	/* might be a HTML tag, check */
	ScanTagName(cursor, IsEndTag);
	int Index{ FindTag(cursor.start + (IsEndTag ? 2 : 1)) };
	if (Index > 0)
		Index = AcceptTagParameters(cursor, Index, IsEndTag, WhiteSpace);
	if (*cursor.end == _T('\x02'))
		Advance(cursor);
	/* skip trailing white space in some circumstances */
	SkipTrailingWhiteSpace(cursor, Index, EntryWhiteSpace);
	return Index;
}

void HtmlTextDrawer::ScanTagName(TokenCursor& cursor, int& IsEndTag)
{
	Advance(cursor);
	if (cursor.length < cursor.size && *cursor.end == _T('\x03'))
	{
		IsEndTag = s_endFlag;
		Advance(cursor);
	} /* if */
	while (cursor.length < cursor.size && !_istspace(*cursor.end) && *cursor.end != _T('\x01') && *cursor.end != _T('\x02'))
	{
		Advance(cursor);
	} /* while */
}

int HtmlTextDrawer::FindTag(LPCTSTR name)
{
	int Index{ static_cast<int>(s_tags.size()) - 1 };
	for (; Index > 0; Index--)
		if (!_tcsnicmp(name, s_tags[Index].mnemonic, _tcslen(s_tags[Index].mnemonic)))
			break;
	return Index;
}

int HtmlTextDrawer::AcceptTagParameters(TokenCursor& cursor, int Index, int IsEndTag, BOOL* WhiteSpace)
{
	/* so it is a tag, see whether to accept parameters */
	if (s_tags[Index].param && !IsEndTag)
	{
		SkipTagParameters(cursor);
	}
	else if (*cursor.end != _T('\x02'))
	{
		/* no parameters, then '>' must follow the tag */
		Index = 0;
	} /* if */
	if (WhiteSpace != NULL && s_tags[Index].block)
		*WhiteSpace = FALSE;
	return Index;
}

void HtmlTextDrawer::SkipTagParameters(TokenCursor& cursor)
{
	while (cursor.length < cursor.size && *cursor.end != _T('\x01') && *cursor.end != _T('\x02'))
	{
		Advance(cursor);
	} /* while */
}

void HtmlTextDrawer::SkipTrailingWhiteSpace(TokenCursor& cursor, int Index, BOOL EntryWhiteSpace)
{
	if (Index > 0 && (s_tags[Index].block || EntryWhiteSpace))
	{
		while (cursor.length < cursor.size && _istspace(*cursor.end))
		{
			Advance(cursor);
		} /* while */
	} /* if */
}

void HtmlTextDrawer::ScanWord(TokenCursor& cursor)
{
	while (cursor.length < cursor.size && !_istspace(*cursor.end) && *cursor.end != _T('\x01'))
	{
		Advance(cursor);
	} /* while */
}

int HtmlTextDrawer::HexDigit(TCHAR ch)
{
	if (ch >= _T('0') && ch <= _T('9'))
		return ch - _T('0');
	if (ch >= _T('A') && ch <= _T('F'))
		return ch - _T('A') + 10;
	if (ch >= _T('a') && ch <= _T('f'))
		return ch - _T('a') + 10;
	return 0;
}

COLORREF HtmlTextDrawer::ParseColor(LPCTSTR String)
{
	if (*String == _T('\'') || *String == _T('"'))
		String++;
	if (*String == _T('#'))
		String++;
	const int Red{ (HexDigit(String[0]) << 4) | HexDigit(String[1]) };
	const int Green{ (HexDigit(String[2]) << 4) | HexDigit(String[3]) };
	const int Blue{ (HexDigit(String[4]) << 4) | HexDigit(String[5]) };
	return RGB(Red, Green, Blue);
}

int HtmlTextDrawer::StyleFlagOf(int token)
{
	if (token < 0 || token >= static_cast<int>(s_styleFlags.size()))
		return 0;
	return s_styleFlags[static_cast<size_t>(token)];
}

BOOL HtmlTextDrawer::PushColor(COLORREF clr)
{
	if (m_colorStackTop < s_colorStackSize)
		m_colorStack[static_cast<size_t>(m_colorStackTop++)] = ::GetTextColor(m_hdc);
	::SetTextColor(m_hdc, clr);
	return TRUE;
}

BOOL HtmlTextDrawer::PopColor()
{
	const BOOL okay{ m_colorStackTop > 0 };
	const COLORREF clr{ okay ? m_colorStack[static_cast<size_t>(--m_colorStackTop)] : m_colorStack[0] };
	::SetTextColor(m_hdc, clr);
	return okay;
}

HFONT HtmlTextDrawer::GetFontVariant(HFONT hfontSource, int Styles)
{
	LOGFONT logFont{};

	::SelectObject(m_hdc, ::GetStockObject(SYSTEM_FONT));
	if (!::GetObject(hfontSource, static_cast<int>(sizeof logFont), &logFont))
		return NULL;

	/* set parameters, create new font */
	logFont.lfWeight = (Styles & s_bold) ? FW_BOLD : FW_NORMAL;
	logFont.lfItalic = static_cast<BYTE>((Styles & s_italic) != 0);
	logFont.lfUnderline = static_cast<BYTE>((Styles & s_underline) != 0);
	if (Styles & (s_superscript | s_subscript))
		logFont.lfHeight = logFont.lfHeight * 7 / 10;
	return ::CreateFontIndirect(&logFont);
}

int HtmlTextDrawer::Draw(
	HDC hdc,          // handle of device context
	LPCTSTR lpString, // address of string to draw
	int nCount,       // string length, in characters
	LPRECT lpRect,    // address of structure with formatting dimensions
	UINT uFormat      // text-drawing flags
)
{
	if (hdc == NULL || lpString == NULL)
		return 0;
	if (nCount < 0)
		nCount = static_cast<int>(_tcslen(lpString)); /* display strings are far below INT_MAX characters */

	m_hdc = hdc;
	SetDrawArea(lpRect);

	/* toggle flags we do not support */
	uFormat &= ~static_cast<UINT>(DT_CENTER | DT_RIGHT | DT_TABSTOP);
	uFormat |= (DT_LEFT | DT_NOPREFIX);
	m_format = uFormat;

	/* get the "default" font from the DC */
	const int SavedDC{ ::SaveDC(hdc) };
	BeginText();

	/* run through the string, word for word */
	DrawTokens(lpString, nCount);

	::RestoreDC(hdc, SavedDC);
	DeleteFontVariants();

	/* store width and height back into the lpRect structure */
	StoreTextSize(lpRect);

	return m_height;
}

void HtmlTextDrawer::SetDrawArea(LPRECT lpRect)
{
	m_maxHeight = INT_MAX;

	if (lpRect != NULL)
	{
		m_left = lpRect->left;
		m_top = lpRect->top;
		m_maxWidth = lpRect->right - lpRect->left;
		m_maxHeight = lpRect->bottom - lpRect->top;
	}
	else
	{
		POINT CurPos{};
		::GetCurrentPositionEx(m_hdc, &CurPos);
		m_left = CurPos.x;
		m_top = CurPos.y;
		m_maxWidth = ::GetDeviceCaps(m_hdc, HORZRES) - m_left;
	} /* if */
	if (m_maxWidth < 0)
		m_maxWidth = 0;
}

void HtmlTextDrawer::BeginText()
{
	m_baseFont = static_cast<HFONT>(::SelectObject(m_hdc, ::GetStockObject(SYSTEM_FONT)));
	::SelectObject(m_hdc, m_baseFont);
	/* clear the other fonts, they are created "on demand" */
	m_fonts.fill(nullptr);
	m_fonts[0] = m_baseFont;
	m_styles = 0; /* assume the active font is normal weight, roman, non-underlined */

	/* get font height (use characters with ascender and descender);
	 * we make the assumption here that changing the font style will
	 * not change the font height
	 */
	SIZE size{};
	::GetTextExtentPoint32(m_hdc, _T("Åy"), 2, &size);
	m_lineHeight = size.cy;

	m_spaceWidth = 0;
	m_xPos = 0;
	m_minWidth = 0;
	m_colorStackTop = 0;
	m_curStyles = -1; /* force a select of the proper style */
	m_height = 0;
	m_whiteSpace = FALSE;
}

void HtmlTextDrawer::DrawTokens(LPCTSTR lpString, int nCount)
{
	LPCTSTR Start{ lpString };
	for (;;)
	{
		int TokenLength{};
		const int Tag{ GetToken(&Start, &nCount, &TokenLength, &m_whiteSpace) };
		if (Tag < 0)
			break;
		ApplyToken(Tag, Start, Start == lpString, TokenLength);

		if ((m_height + m_lineHeight) >= m_maxHeight)
			break;

		Start += TokenLength;
	} /* for */
}

void HtmlTextDrawer::ApplyToken(int Tag, LPCTSTR Start, bool atTextStart, int TokenLength)
{
	const bool endTag{ (Tag & s_endFlag) != 0 };
	const int token{ Tag & ~s_endFlag };
	const int styleFlag{ StyleFlagOf(token) };
	if (styleFlag != 0)
		m_styles = endTag ? m_styles & ~styleFlag : m_styles | styleFlag;
	else if (token == tP)
		StartParagraph(endTag, atTextStart);
	else if (token == tBR)
		BreakLine(endTag);
	else if (token == tFONT)
		ApplyFontTag(endTag, Start);
	else if (Tag != (tNONE | s_endFlag))
		DrawWord(Start, TokenLength);
}

void HtmlTextDrawer::StartParagraph(bool endTag, bool atTextStart)
{
	if (!endTag && (m_format & DT_SINGLELINE) == 0)
	{
		if (!atTextStart)
			m_height += 3 * m_lineHeight / 2;
		m_xPos = 0;
	} /* if */
}

void HtmlTextDrawer::BreakLine(bool endTag)
{
	if (!endTag && (m_format & DT_SINGLELINE) == 0)
	{
		m_height += m_lineHeight;
		m_xPos = 0;
	} /* if */
}

void HtmlTextDrawer::ApplyFontTag(bool endTag, LPCTSTR Start)
{
	if (!endTag)
	{
		if (_tcsnicmp(Start + 3, _T("color="), 6) == 0)
			PushColor(ParseColor(Start + 9));
	}
	else
	{
		PopColor();
	} /* if */
}

void HtmlTextDrawer::DrawWord(LPCTSTR Start, int TokenLength)
{
	SelectStyleFont();
	/* check word length, check whether to wrap around */
	SIZE size{};
	::GetTextExtentPoint32(m_hdc, Start, TokenLength, &size);
	if (size.cx > m_maxWidth)
		m_maxWidth = size.cx; /* must increase width: long non-breakable word */
	if (m_whiteSpace)
		m_xPos += m_spaceWidth;
	if (m_xPos + size.cx > m_maxWidth && m_whiteSpace)
		WrapOrWiden(size.cx);
	/* output text (unless DT_CALCRECT is set) */
	if ((m_format & DT_CALCRECT) == 0)
		OutputWord(Start, TokenLength);
	/* update current position */
	m_xPos += size.cx;
	if (m_xPos > m_minWidth)
		m_minWidth = m_xPos;
	m_whiteSpace = FALSE;
}

void HtmlTextDrawer::SelectStyleFont()
{
	if (m_curStyles == m_styles)
		return;

	const size_t variant{ static_cast<size_t>(m_styles) };
	if (m_fonts[variant] == NULL)
		m_fonts[variant] = GetFontVariant(m_baseFont, m_styles);
	m_curStyles = m_styles;
	::SelectObject(m_hdc, m_fonts[variant]);
	/* get the width of a space character (for word spacing) */
	SIZE size{};
	::GetTextExtentPoint32(m_hdc, _T(" "), 1, &size);
	m_spaceWidth = size.cx;
}

void HtmlTextDrawer::WrapOrWiden(int wordWidth)
{
	if ((m_format & DT_WORDBREAK) != 0)
	{
		/* word wrap */
		m_height += m_lineHeight;
		m_xPos = 0;
	}
	else
	{
		/* no word wrap, must increase the width */
		m_maxWidth = m_xPos + wordWidth;
	} /* if */
}

void HtmlTextDrawer::OutputWord(LPCTSTR Start, int TokenLength)
{
	/* handle negative heights, too (suggestion of "Sims")  */
	RECT rc{};
	SetLineRect(rc, m_left + m_xPos, m_left + m_maxWidth);

	/* reposition subscript text to align below the baseline */
	::DrawText(m_hdc, Start, TokenLength, &rc,
			   m_format | ((m_styles & s_subscript) ? DT_BOTTOM | DT_SINGLELINE : 0));

	/* for the underline style, the spaces between words should be
	 * underlined as well
	 */
	if (m_whiteSpace && (m_styles & s_underline) && m_xPos >= m_spaceWidth)
	{
		SetLineRect(rc, m_left + m_xPos - m_spaceWidth, m_left + m_xPos);
		// a writable buffer: with DT_MODIFYSTRING, DrawText may append an ellipsis (up to 4 characters)
		TCHAR space[6]{ _T(' ') };
		::DrawText(m_hdc, space, 1, &rc, m_format);
	} /* if */
}

void HtmlTextDrawer::SetLineRect(RECT& rc, int left, int right) const
{
	if (m_top < 0)
		::SetRect(&rc, left, m_top - m_height, right, m_top - (m_height + m_lineHeight));
	else
		::SetRect(&rc, left, m_top + m_height, right, m_top + m_height + m_lineHeight);
}

void HtmlTextDrawer::DeleteFontVariants()
{
	for (size_t Index = 1; Index < m_fonts.size(); Index++) /* do not erase m_fonts[0] */
		if (m_fonts[Index] != NULL)
			::DeleteObject(m_fonts[Index]);
}

void HtmlTextDrawer::StoreTextSize(LPRECT lpRect) const
{
	if ((m_format & DT_CALCRECT) != 0 && lpRect != NULL)
	{
		lpRect->right = lpRect->left + m_minWidth;
		if (lpRect->top < 0)
			lpRect->bottom = lpRect->top - (m_height + m_lineHeight);
		else
			lpRect->bottom = lpRect->top + m_height + m_lineHeight;
	} /* if */
}

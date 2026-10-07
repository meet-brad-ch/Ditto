#pragma once
/* HtmlTextDrawer (formerly DrawHTML())
 * Drop-in replacement for DrawText() supporting a tiny subset of HTML.
 */

#include <array>

/**
 * @brief Draws text with a tiny subset of HTML, as a drop-in replacement for DrawText().
 *
 * The tags use unprintable characters so that copied HTML is never converted: 0x01 opens a tag,
 * 0x02 closes it, 0x03 after 0x01 marks an end tag, 0x04 is the font tag (it takes a
 * color='#rrggbb' parameter) and 0x05 is a line break.
 * The text colours that font tags replace are kept on a small stack that lives as long as the
 * drawer; each Draw starts with an empty stack (closing a font tag on an empty stack restores the
 * bottom entry, as the former file-level stack did).
 */
class HtmlTextDrawer
{
public:
	/**
	 * @brief Draws the text, word by word, with the tags applied (DrawText's parameters).
	 * @param hdc the device context; nothing is drawn when it is NULL.
	 * @param lpString the text to draw; nothing is drawn when it is NULL.
	 * @param nCount the text length in characters; negative for a null-terminated text.
	 * @param lpRect the formatting rectangle; NULL to draw at the current position of the DC.
	 *        With DT_CALCRECT its right and bottom receive the size of the text.
	 * @param uFormat the DrawText flags (DT_CENTER, DT_RIGHT and DT_TABSTOP are not supported).
	 * @return the height of the text above the last line (0 when hdc or lpString is NULL).
	 */
	int Draw(HDC hdc, LPCTSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);

private:
	/** @brief The token ids that GetToken returns (an end tag adds s_endFlag). */
	enum TokenId : int { tNONE, tB, tBR, tFONT, tI, tP, tSUB, tSUP, tU, tNUMTAGS };

	/** @brief One tag the drawer recognises. */
	struct TagInfo
	{
		/** @brief The tag name that follows the 0x01 (or 0x01 0x03) opening; NULL for no tag. */
		LPCTSTR mnemonic{};
		/** @brief The token id of the tag. */
		short token{};
		/** @brief Nonzero when the tag takes parameters. */
		short param{};
		/** @brief Nonzero when the tag is a block (line breaking) tag. */
		short block{};
	};

	/** @brief One step of a token scan: the token found so far and the text left. */
	struct TokenCursor
	{
		/** @brief The first character of the token. */
		LPCTSTR start{};
		/** @brief The character after the token scanned so far. */
		LPCTSTR end{};
		/** @brief The number of characters in the token so far. */
		int length{};
		/** @brief The number of characters left from start on. */
		int size{};
	};

	/** @brief Added to a token id for an end tag. */
	static constexpr int s_endFlag{0x100};
	/** @brief Font variant flag: bold. */
	static constexpr int s_bold{0x01};
	/** @brief Font variant flag: italic. */
	static constexpr int s_italic{s_bold << 1};
	/** @brief Font variant flag: underline. */
	static constexpr int s_underline{s_italic << 1};
	/** @brief Font variant flag: superscript. */
	static constexpr int s_superscript{s_underline << 1};
	/** @brief Font variant flag: subscript. */
	static constexpr int s_subscript{s_superscript << 1};
	/** @brief The number of font variants (all flag combinations). */
	static constexpr int s_fontVariants{s_subscript << 1};
	/** @brief The number of text colours the colour stack holds. */
	static constexpr int s_colorStackSize{8};

	/** @brief The recognised tags; entry 0 stands for "no tag". */
	static const std::array<TagInfo, 3> s_tags;
	/** @brief The font variant flag of each token id (0 for the tokens that are no style). */
	static const std::array<int, tNUMTAGS> s_styleFlags;

	/**
	 * @brief Finds the next token (a word or a tag) in the text.
	 * @param String in: the text left; out: the first character of the token.
	 * @param Size in: the number of characters left; out: the number left after the token.
	 * @param TokenLength receives the token length in characters; may be NULL.
	 * @param WhiteSpace in: whether white space came before; out: whether white space comes
	 *        before the token (a block tag clears it); may be NULL.
	 * @return the token id (plus s_endFlag for an end tag), or -1 when no printable text is left.
	 */
	static int GetToken(LPCTSTR* String, int* Size, int* TokenLength, BOOL* WhiteSpace);
	/**
	 * @brief Records whether white space comes before the next token.
	 * @param Start the first character of the text left.
	 * @param WhiteSpace in: whether white space came before; out: or-ed with the white space at
	 *        Start; may be NULL.
	 * @return the white space state on entry (FALSE when WhiteSpace is NULL).
	 */
	static BOOL ReadEntryWhiteSpace(LPCTSTR Start, BOOL* WhiteSpace);
	/**
	 * @brief Skips the white space at the start of the cursor.
	 * @param cursor the scan; its start and size move past the white space.
	 */
	static void SkipLeadingWhiteSpace(TokenCursor& cursor);
	/**
	 * @brief Adds the character at the cursor's end to the token.
	 * @param cursor the scan.
	 */
	static void Advance(TokenCursor& cursor);
	/**
	 * @brief Scans a token that starts with 0x01 (a tag, if its name is known).
	 * @param cursor the scan; its end and length cover the tag.
	 * @param WhiteSpace the caller's white space state; a block tag clears it; may be NULL.
	 * @param EntryWhiteSpace the white space state before the token.
	 * @param IsEndTag receives s_endFlag for an end tag, else 0.
	 * @return the index of the tag in s_tags, 0 when it is no known tag.
	 */
	static int ScanTag(TokenCursor& cursor, BOOL* WhiteSpace, BOOL EntryWhiteSpace, int& IsEndTag);
	/**
	 * @brief Scans the 0x01 opening, the optional 0x03 end mark and the tag name.
	 * @param cursor the scan.
	 * @param IsEndTag receives s_endFlag for an end tag, else 0.
	 */
	static void ScanTagName(TokenCursor& cursor, int& IsEndTag);
	/**
	 * @brief Finds the tag whose name starts the text.
	 * @param name the text after the tag opening.
	 * @return the index of the tag in s_tags, 0 when none matches.
	 */
	static int FindTag(LPCTSTR name);
	/**
	 * @brief Takes the parameters of a known tag, or rejects the tag when its closing is missing.
	 * @param cursor the scan; its end and length cover the parameters.
	 * @param Index the index of the tag in s_tags.
	 * @param IsEndTag s_endFlag for an end tag, else 0.
	 * @param WhiteSpace the caller's white space state; a block tag clears it; may be NULL.
	 * @return the index of the tag, 0 when it is rejected.
	 */
	static int AcceptTagParameters(TokenCursor& cursor, int Index, int IsEndTag, BOOL* WhiteSpace);
	/**
	 * @brief Skips the parameters of a tag (up to its 0x02 closing or the next 0x01).
	 * @param cursor the scan.
	 */
	static void SkipTagParameters(TokenCursor& cursor);
	/**
	 * @brief Skips the white space after a block tag or after a tag that follows white space.
	 * @param cursor the scan.
	 * @param Index the index of the tag in s_tags (0: no tag, nothing is skipped).
	 * @param EntryWhiteSpace the white space state before the token.
	 */
	static void SkipTrailingWhiteSpace(TokenCursor& cursor, int Index, BOOL EntryWhiteSpace);
	/**
	 * @brief Scans a word: up to the next white space or 0x01.
	 * @param cursor the scan.
	 */
	static void ScanWord(TokenCursor& cursor);
	/**
	 * @brief The value of a hexadecimal digit.
	 * @param ch the character.
	 * @return 0-15, or 0 when ch is no hexadecimal digit.
	 */
	static int HexDigit(TCHAR ch);
	/**
	 * @brief Parses a colour written as #rrggbb (optionally quoted).
	 * @param String the colour text.
	 * @return the colour.
	 */
	static COLORREF ParseColor(LPCTSTR String);
	/**
	 * @brief The font variant flag of a token.
	 * @param token the token id (without s_endFlag).
	 * @return the flag, 0 when the token is no style.
	 */
	static int StyleFlagOf(int token);

	/**
	 * @brief Sets the drawing origin and the maximum width and height.
	 * @param lpRect the formatting rectangle; NULL to use the current position of the DC.
	 */
	void SetDrawArea(LPRECT lpRect);
	/** @brief Takes the DC's font as the base font and resets the drawing state of a new text. */
	void BeginText();
	/**
	 * @brief Draws the text token by token until it ends or the maximum height is reached.
	 * @param lpString the text.
	 * @param nCount the text length in characters.
	 */
	void DrawTokens(LPCTSTR lpString, int nCount);
	/**
	 * @brief Applies one token: a style, paragraph, line break or font tag, or draws a word.
	 * @param Tag the token id from GetToken.
	 * @param Start the first character of the token.
	 * @param atTextStart whether the token is the first character of the text.
	 * @param TokenLength the token length in characters.
	 */
	void ApplyToken(int Tag, LPCTSTR Start, bool atTextStart, int TokenLength);
	/**
	 * @brief Starts a paragraph (unless single line).
	 * @param endTag whether the tag is an end tag (then nothing happens).
	 * @param atTextStart whether the tag starts the text (then no space is added above).
	 */
	void StartParagraph(bool endTag, bool atTextStart);
	/**
	 * @brief Starts a new line (unless single line).
	 * @param endTag whether the tag is an end tag (then nothing happens).
	 */
	void BreakLine(bool endTag);
	/**
	 * @brief Applies a font tag: a start tag pushes its colour, an end tag pops it.
	 * @param endTag whether the tag is an end tag.
	 * @param Start the first character of the tag.
	 */
	void ApplyFontTag(bool endTag, LPCTSTR Start);
	/**
	 * @brief Measures, wraps and (unless DT_CALCRECT) draws a word.
	 * @param Start the first character of the word.
	 * @param TokenLength the word length in characters.
	 */
	void DrawWord(LPCTSTR Start, int TokenLength);
	/** @brief Selects the font of the current styles (created on first use) into the DC. */
	void SelectStyleFont();
	/**
	 * @brief Wraps to the next line (DT_WORDBREAK) or widens the text for a word that does not fit.
	 * @param wordWidth the width of the word.
	 */
	void WrapOrWiden(int wordWidth);
	/**
	 * @brief Draws a word at the current position (and the underlined space before it).
	 * @param Start the first character of the word.
	 * @param TokenLength the word length in characters.
	 */
	void OutputWord(LPCTSTR Start, int TokenLength);
	/**
	 * @brief Sets a rectangle on the current line (negative tops grow upwards).
	 * @param rc receives the rectangle.
	 * @param left the left edge.
	 * @param right the right edge.
	 */
	void SetLineRect(RECT& rc, int left, int right) const;
	/** @brief Deletes the font variants created for the text (never the DC's own font). */
	void DeleteFontVariants();
	/**
	 * @brief Stores the size of the text into the rectangle (DT_CALCRECT only).
	 * @param lpRect the formatting rectangle; may be NULL.
	 */
	void StoreTextSize(LPRECT lpRect) const;
	/**
	 * @brief Pushes the DC's text colour and selects another.
	 * @param clr the new text colour.
	 * @return TRUE.
	 */
	BOOL PushColor(COLORREF clr);
	/**
	 * @brief Restores the text colour pushed last (the bottom entry when the stack is empty).
	 * @return whether the stack held a colour.
	 */
	BOOL PopColor();
	/**
	 * @brief Creates a variant of a font.
	 * @param hfontSource the font to vary.
	 * @param Styles the font variant flags.
	 * @return the new font (the caller deletes it), NULL when the font cannot be read.
	 */
	HFONT GetFontVariant(HFONT hfontSource, int Styles);

	/** @brief The text colours replaced by font tags. */
	std::array<COLORREF, s_colorStackSize> m_colorStack{};
	/** @brief The number of colours on m_colorStack. */
	int m_colorStackTop{};
	/** @brief The device context of the text being drawn. */
	HDC m_hdc{};
	/** @brief The DrawText flags of the text being drawn. */
	UINT m_format{};
	/** @brief The left edge of the text. */
	int m_left{};
	/** @brief The top edge of the text (negative: the text grows upwards). */
	int m_top{};
	/** @brief The maximum width of a line (widened for long words). */
	int m_maxWidth{};
	/** @brief The width of the longest line so far. */
	int m_minWidth{};
	/** @brief The height of the text above the current line. */
	int m_height{};
	/** @brief The maximum height of the text. */
	int m_maxHeight{};
	/** @brief The height of a line. */
	int m_lineHeight{};
	/** @brief The position of the next word on the current line. */
	int m_xPos{};
	/** @brief The width of a space in the current font. */
	int m_spaceWidth{};
	/** @brief The font variant flags of the next word. */
	int m_styles{};
	/** @brief The font variant flags of the font selected into the DC (-1: none yet). */
	int m_curStyles{};
	/** @brief Whether white space comes before the next word. */
	BOOL m_whiteSpace{};
	/** @brief The DC's font when the text started. */
	HFONT m_baseFont{};
	/** @brief The font variants by flags (created on demand; entry 0 is m_baseFont). */
	std::array<HFONT, s_fontVariants> m_fonts{};
};

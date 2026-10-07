#pragma once

class CTextConvert
{
public:

	CTextConvert();
	~CTextConvert();

	static bool ConvertFromUTF8(const CStringA &src, CString &dest);
	static bool ConvertToUTF8(const CString &src, CStringA &dest);

	static CStringA UnicodeStringToMultiByte(const CStringW &srcString);
	static CStringW MultiByteToUnicodeString(const CStringA &srcString);

	static CStringA ConvertToChar(const CString &src);
	static CStringW ConvertToUnicode(const CString &src);

protected:

private:
	/**
	 * @brief ConvertFromUTF8's step: decodes one UTF-8 character and appends it as UTF-16.
	 * @param src the UTF-8 text.
	 * @param i in: index of the lead byte; out: index after the character.
	 * @param dest the text to append to.
	 * @return false for a malformed sequence.
	 */
	static bool DecodeUtf8Char(const CStringA &src, int &i, CString &dest);
	/**
	 * @brief Counts the continuation bytes of a UTF-8 lead byte.
	 * @param c the lead byte (0xC0 or more).
	 * @return the number of continuation bytes (1 to 5).
	 */
	static int GetUtf8ExtraByteCount(BYTE c);
	/**
	 * @brief Reads the continuation bytes of a UTF-8 character into value.
	 * @param src the UTF-8 text.
	 * @param i in: index of the first continuation byte; out: index after the character.
	 * @param numAdds the number of continuation bytes.
	 * @param value in: the lead byte bits; out: the code point.
	 * @return false if the text ends early or a byte is no continuation byte.
	 */
	static bool ReadUtf8ExtraBytes(const CStringA &src, int &i, int numAdds, UINT &value);
	/**
	 * @brief Appends a code point as UTF-16 (a surrogate pair above 0xFFFF).
	 * @param value the code point.
	 * @param dest the text to append to.
	 * @return false for a code point above 0x10FFFF.
	 */
	static bool AppendUtf16(UINT value, CString &dest);
	/**
	 * @brief ConvertToUTF8's step: encodes one UTF-16 character (or surrogate pair) as UTF-8.
	 * @param src the UTF-16 text.
	 * @param i in: index of the character; out: index after it.
	 * @param dest the text to append to.
	 * @return false for a malformed surrogate pair.
	 */
	static bool EncodeUtf8Char(const CString &src, int &i, CStringA &dest);
	/**
	 * @brief Combines a high surrogate with the low surrogate that follows it.
	 * @param src the UTF-16 text.
	 * @param i in: index after the high surrogate; out: index after the low surrogate.
	 * @param value in: the high surrogate; out: the code point.
	 * @return false for a lone or misplaced surrogate.
	 */
	static bool ReadSurrogatePair(const CString &src, int &i, UINT &value);
	/**
	 * @brief Appends a code point as UTF-8.
	 * @param value the code point.
	 * @param dest the text to append to.
	 */
	static void AppendUtf8(UINT value, CStringA &dest);
};
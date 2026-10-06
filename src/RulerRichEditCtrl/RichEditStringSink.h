/**
 * @file RichEditStringSink.h
 * @brief Declares CRichEditStringSink.
 */
#pragma once

#include <string>

/**
 * @brief Receives the output of CRichEditCtrl::StreamOut.
 *
 * All bytes are collected first and converted from UTF-8 once, so a character split across
 * two callbacks stays intact.
 */
class CRichEditStringSink
{
public:
	/// @brief Creates an empty sink.
	CRichEditStringSink() = default;

	CRichEditStringSink(const CRichEditStringSink&) = delete;
	CRichEditStringSink& operator=(const CRichEditStringSink&) = delete;

	/**
	 * @brief The EDITSTREAM to pass to StreamOut.
	 * @return A stream that writes into this object; valid while this object lives.
	 */
	EDITSTREAM Stream();

	/**
	 * @brief Everything received so far.
	 * @return The received UTF-8 bytes as text.
	 */
	CString Text() const;

private:
	/**
	 * @brief EDITSTREAM callback: appends the next chunk.
	 * @param cookie This object.
	 * @param buffer The control's output.
	 * @param size Number of bytes in @p buffer.
	 * @param written Receives the bytes taken (all of them).
	 * @return 0 (no error).
	 */
	static DWORD CALLBACK Write(DWORD_PTR cookie, LPBYTE buffer, LONG size, LONG* written);

	/// The bytes received, UTF-8.
	std::string m_utf8{};
};

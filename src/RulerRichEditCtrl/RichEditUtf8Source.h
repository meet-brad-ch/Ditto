/**
 * @file RichEditUtf8Source.h
 * @brief Declares CRichEditUtf8Source.
 */
#pragma once

#include <cstddef>
#include <string>

/**
 * @brief Feeds text to CRichEditCtrl::StreamIn as UTF-8.
 *
 * The text is converted once and handed out in chunks of at most the size the control asks
 * for; the byte position is kept between callbacks.
 */
class CRichEditUtf8Source
{
public:
	/**
	 * @brief Converts the text to UTF-8.
	 * @param text The text to stream in.
	 */
	explicit CRichEditUtf8Source(const CString& text);

	CRichEditUtf8Source(const CRichEditUtf8Source&) = delete;
	CRichEditUtf8Source& operator=(const CRichEditUtf8Source&) = delete;

	/**
	 * @brief The EDITSTREAM to pass to StreamIn.
	 * @return A stream that reads from this object; valid while this object lives.
	 */
	EDITSTREAM Stream();

private:
	/**
	 * @brief EDITSTREAM callback: copies the next chunk.
	 * @param cookie This object.
	 * @param buffer The control's buffer.
	 * @param size Capacity of @p buffer in bytes.
	 * @param written Receives the bytes copied; 0 ends the stream.
	 * @return 0 (no error).
	 */
	static DWORD CALLBACK Read(DWORD_PTR cookie, LPBYTE buffer, LONG size, LONG* written);

	/// The text as UTF-8.
	std::string m_utf8{};
	/// Bytes already handed to the control.
	std::size_t m_offset{};
};

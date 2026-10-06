/**
 * @file RichEditUtf8Source.cpp
 * @brief Implements CRichEditUtf8Source.
 */
#include "stdafx.h"
#include "RichEditUtf8Source.h"
#include "..\..\Shared\TextConvert.h"

#include <algorithm>
#include <cstring>

CRichEditUtf8Source::CRichEditUtf8Source(const CString& text)
{
	const CStringA utf8{ CTextConvert::UnicodeToUTF8(text) };
	m_utf8.assign(utf8.GetString(), static_cast<std::size_t>(utf8.GetLength()));
}

EDITSTREAM CRichEditUtf8Source::Stream()
{
	EDITSTREAM stream{};
	stream.dwCookie = reinterpret_cast<DWORD_PTR>(this);
	stream.pfnCallback = &CRichEditUtf8Source::Read;
	return stream;
}

DWORD CALLBACK CRichEditUtf8Source::Read(DWORD_PTR cookie, LPBYTE buffer, LONG size, LONG* written)
{
	CRichEditUtf8Source* source{ reinterpret_cast<CRichEditUtf8Source*>(cookie) };
	const std::size_t remaining{ source->m_utf8.size() - source->m_offset };
	// (std::min) / (std::max): the MFC build defines min/max macros
	const std::size_t count{ (std::min)(remaining, static_cast<std::size_t>((std::max)(size, 0L))) };

	std::memcpy(buffer, source->m_utf8.data() + source->m_offset, count);
	source->m_offset += count;
	*written = static_cast<LONG>(count);  // 0 tells the control the stream has ended
	return 0;
}

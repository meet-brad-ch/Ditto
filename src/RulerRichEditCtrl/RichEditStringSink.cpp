/**
 * @file RichEditStringSink.cpp
 * @brief Implements CRichEditStringSink.
 */
#include "stdafx.h"
#include "RichEditStringSink.h"
#include "..\..\Shared\TextConvert.h"

EDITSTREAM CRichEditStringSink::Stream()
{
	EDITSTREAM stream{};
	stream.dwCookie = reinterpret_cast<DWORD_PTR>(this);
	stream.pfnCallback = &CRichEditStringSink::Write;
	return stream;
}

CString CRichEditStringSink::Text() const
{
	const CStringA utf8{ m_utf8.data(), static_cast<int>(m_utf8.size()) };
	return CTextConvert::Utf8ToUnicode(utf8);
}

DWORD CALLBACK CRichEditStringSink::Write(DWORD_PTR cookie, LPBYTE buffer, LONG size, LONG* written)
{
	CRichEditStringSink* sink{ reinterpret_cast<CRichEditStringSink*>(cookie) };
	const LONG count{ size > 0 ? size : 0 };
	sink->m_utf8.append(reinterpret_cast<const char*>(buffer), static_cast<std::size_t>(count));
	*written = count;
	return 0;
}

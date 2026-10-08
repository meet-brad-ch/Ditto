#include "stdafx.h"
#include "StringUtil.h"
#include <stdexcept>
#include <string>

CString CStringUtil::Format(const TCHAR* pszFormat, ...)
{
	ASSERT(AfxIsValidString(pszFormat));
	CString str{};
	va_list argList{};
	va_start(argList, pszFormat);
	str.FormatV(pszFormat, argList);
	va_end(argList);
	return str;
}

int CStringUtil::WordCount(const CString& text)
{
	constexpr int outsideWord = 0;
	constexpr int insideWord = 1;

	int state{ outsideWord };
	unsigned wc{}; // word count

	// Scan all characters one by one
	for (int pos{}; pos < text.GetLength(); pos++)
	{
		const auto str{ text[pos] };

		if (str == ' ' || str == '\r' || str == '\n' || str == '\t')
		{
			state = outsideWord;
		}
		else if (state == outsideWord)
		{
			state = insideWord;
			wc++;
		}
	}

	return wc;
}

CString CStringUtil::NewGuidString()
{
	CString guidString{};

	GUID guid{};
	const HRESULT hr = CoCreateGuid(&guid);
	if (FAILED(hr))
	{
		throw std::runtime_error("CoCreateGuid failed with HRESULT " + std::to_string(hr));
	}
	guidString.Format(_T("%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX"),
					  guid.Data1, guid.Data2, guid.Data3,
					  guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
					  guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);

	return guidString.MakeLower();
}

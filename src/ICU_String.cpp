#include "stdafx.h"
#include "ICU_String.h"
#include "Misc.h"

#include <cwctype>
#include <stdexcept>
#include <string>


CICU_String::CICU_String()
{
	m_dllHandle = NULL;
	u_isUUppercase = NULL;
	u_tolower = NULL;
	u_toupper = NULL;
	u_strToLower = NULL;
	u_strToUpper = NULL;
}

CICU_String::~CICU_String()
{
	if (m_dllHandle)
	{
		::FreeLibrary(m_dllHandle);
		m_dllHandle = NULL;
	}
}

bool CICU_String::Load()
{
	bool loaded = false;
	m_dllHandle = ::LoadLibrary(_T("icu.dll"));
	if (m_dllHandle != NULL)
	{
		u_isUUppercase = (bool(__cdecl*)(wchar_t c))GetProcAddress(m_dllHandle, "u_isUUppercase");
		u_tolower = (wchar_t(__cdecl*)(wchar_t c))GetProcAddress(m_dllHandle, "u_tolower");
		u_toupper = (wchar_t(__cdecl*)(wchar_t c))GetProcAddress(m_dllHandle, "u_toupper");
		u_strToLower = (int(__cdecl*)(wchar_t* dest, int destCapacity, const wchar_t* src, int srcLength, const char* locale, int* pErrorCode))GetProcAddress(m_dllHandle, "u_strToLower");
		u_strToUpper = (int(__cdecl*)(wchar_t* dest, int destCapacity, const wchar_t* src, int srcLength, const char* locale, int* pErrorCode))GetProcAddress(m_dllHandle, "u_strToUpper");

		Log(_T("Loaded icu.dll, this will be used for upper/lower case calls"));

		loaded = true;
	}
	else
	{
		Log(StrF(_T("Error loading icu.dll, LastError: %d"), ::GetLastError()));
	}

	return loaded;
}


bool CICU_String::IsUpperEx(wchar_t c)
{
	if (u_isUUppercase == NULL)
	{
		return ::iswupper(c) != 0;
	}

	return u_isUUppercase(c);
}

wchar_t CICU_String::ToLowerEx(wchar_t c)
{
	if (u_tolower == NULL)
	{
		return static_cast<wchar_t>(::towlower(c));
	}

	return u_tolower(c);
}

wchar_t CICU_String::ToUpperEx(wchar_t c)
{
	if (u_toupper == NULL)
	{
		return static_cast<wchar_t>(::towupper(c));
	}

	return u_toupper(c);
}

CString CICU_String::ToLowerStringEx(CString source)
{
	if (u_strToLower == NULL)
	{
		return CString(source).MakeLower();
	}

	return ConvertCase(u_strToLower, source);
}

CString CICU_String::ToUpperStringEx(CString source)
{
	if (u_strToUpper == NULL)
	{
		return CString(source).MakeUpper();
	}

	return ConvertCase(u_strToUpper, source);
}

CString CICU_String::ConvertCase(CaseFunction convert, const CString& source)
{
	// ask for the length first: the result can be longer than the source (German sharp s
	// becomes "SS"); upstream guessed 1.2 times the length and ignored the error, so a short
	// text could be cut off
	int errorCode = 0;
	const int length = convert(NULL, 0, source.GetString(), source.GetLength(), NULL, &errorCode);
	if (errorCode > 0 && errorCode != BufferOverflowError)
	{
		throw std::runtime_error("ICU case conversion failed with error " + std::to_string(errorCode));
	}

	CString dest;
	errorCode = 0;
	convert(dest.GetBufferSetLength(length), length, source.GetString(), source.GetLength(), NULL, &errorCode);
	dest.ReleaseBuffer(length);
	// U_STRING_NOT_TERMINATED_WARNING (-124) is expected: the buffer has no room for a null
	if (errorCode > 0)
	{
		throw std::runtime_error("ICU case conversion failed with error " + std::to_string(errorCode));
	}

	return dest;
}

#pragma once
class CICU_String
{
public:

	CICU_String();
	virtual ~CICU_String();

	bool Load();

	bool IsUpperEx(wchar_t c);
	wchar_t ToLowerEx(wchar_t c);
	wchar_t ToUpperEx(wchar_t c);

	CString ToLowerStringEx(CString source);
	CString ToUpperStringEx(CString source);

private:
	// u_strToLower and u_strToUpper have this signature
	typedef int(__cdecl* CaseFunction)(wchar_t* dest, int destCapacity, const wchar_t* src, int srcLength, const char* locale, int* pErrorCode);

	// U_BUFFER_OVERFLOW_ERROR: what a length query (capacity 0) reports
	static constexpr int BufferOverflowError = 15;

	// Converts with an ICU case function, sizing the result first; throws std::runtime_error
	// when ICU reports an error
	static CString ConvertCase(CaseFunction convert, const CString& source);

	HMODULE m_dllHandle;

	bool(__cdecl* u_isUUppercase)(wchar_t c);
	wchar_t(__cdecl* u_tolower)(wchar_t c);
	wchar_t(__cdecl* u_toupper)(wchar_t c);

	int(__cdecl* u_strToLower)(wchar_t* dest, int destCapacity, const wchar_t* src, int srcLength, const char* locale, int* pErrorCode);
	int(__cdecl* u_strToUpper)(wchar_t* dest, int destCapacity, const wchar_t* src, int srcLength, const char* locale, int* pErrorCode);
};


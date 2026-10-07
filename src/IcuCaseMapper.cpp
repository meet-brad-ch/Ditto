#include "stdafx.h"
#include "IcuCaseMapper.h"
#include "ICU_String.h"

CIcuCaseMapper::CIcuCaseMapper(CICU_String& icu) :
	m_icu(icu)
{
}

bool CIcuCaseMapper::IsUpper(wchar_t c) const
{
	return m_icu.IsUpperEx(c);
}

wchar_t CIcuCaseMapper::ToUpper(wchar_t c) const
{
	return m_icu.ToUpperEx(c);
}

wchar_t CIcuCaseMapper::ToLower(wchar_t c) const
{
	return m_icu.ToLowerEx(c);
}

std::wstring CIcuCaseMapper::ToUpper(std::wstring_view text) const
{
	const CString upper = m_icu.ToUpperStringEx(CString(text.data(), static_cast<int>(text.size())));
	return std::wstring(upper.GetString(), upper.GetLength());
}

std::wstring CIcuCaseMapper::ToLower(std::wstring_view text) const
{
	const CString lower = m_icu.ToLowerStringEx(CString(text.data(), static_cast<int>(text.size())));
	return std::wstring(lower.GetString(), lower.GetLength());
}

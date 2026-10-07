#pragma once

#include "ICaseMapper.h"

class CICU_String;

// DittoCore::ICaseMapper over Ditto's ICU wrapper (icu.dll, with the C runtime as its fallback)
class CIcuCaseMapper : public DittoCore::ICaseMapper
{
public:
	// icu: the wrapper; it must outlive the mapper
	explicit CIcuCaseMapper(CICU_String& icu);

	bool IsUpper(wchar_t c) const override;
	wchar_t ToUpper(wchar_t c) const override;
	wchar_t ToLower(wchar_t c) const override;
	std::wstring ToUpper(std::wstring_view text) const override;
	std::wstring ToLower(std::wstring_view text) const override;

private:
	CICU_String& m_icu;
};

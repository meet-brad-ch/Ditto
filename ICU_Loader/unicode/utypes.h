// Ditto's shim: SQLite's untouched ext/icu/icu.c includes the ICU project's headers
// (<unicode/utypes.h>, <unicode/uregex.h>, <unicode/ustring.h>, <unicode/ucol.h>); Ditto builds it
// against the ICU that ships with Windows 10 1703+, whose single header is <icu.h>.
#pragma once
#include <icu.h>

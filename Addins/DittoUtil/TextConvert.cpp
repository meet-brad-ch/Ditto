#include "StdAfx.h"
#include "TextConvert.h"

static BYTE kUtf8Limits[5] = { 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

CTextConvert::CTextConvert()
{
}

CTextConvert::~CTextConvert()
{
}

CStringW CTextConvert::MultiByteToUnicodeString(const CStringA &srcString)
{
	CStringW resultString;

	if(!srcString.IsEmpty())
	{
		int numChars = MultiByteToWideChar(CP_ACP, 0, srcString, 
						srcString.GetLength(), resultString.GetBuffer(srcString.GetLength()), 
						srcString.GetLength() + 1);

		resultString.ReleaseBuffer(numChars);
	}
	return resultString;
}

CStringA CTextConvert::UnicodeStringToMultiByte(const CStringW &srcString)
{
	CStringA resultString;
	if(!srcString.IsEmpty())
	{
		int numRequiredBytes = srcString.GetLength() * sizeof(wchar_t);
		int numChars = WideCharToMultiByte(CP_ACP, 0, srcString, 
							srcString.GetLength(), resultString.GetBuffer(numRequiredBytes), 
							numRequiredBytes + 1, NULL, NULL);

		resultString.ReleaseBuffer(numChars);
	}

	return resultString;
}

CStringA CTextConvert::ConvertToChar(const CString &src)
{
#ifdef _UNICODE
	return UnicodeStringToMultiByte(src);
#else
	return src;
#endif
}

CStringW CTextConvert::ConvertToUnicode(const CString &src)
{
#ifdef _UNICODE
	return src;
#else
	return MultiByteToUnicodeString(src);
#endif
}

bool CTextConvert::ConvertFromUTF8(const CStringA &src, CString &dest)
{
#ifdef _UNICODE	
	dest.Empty();
	for(int i = 0; i < src.GetLength();)
	{
		if (DecodeUtf8Char(src, i, dest) == false)
		{
			dest = src;
			return false;
		}
	}
#else
	dest = src;
#endif
	return true;
}

bool CTextConvert::DecodeUtf8Char(const CStringA &src, int &i, CString &dest)
{
	BYTE c = (BYTE)src[i++];
	if (c < 0x80)
	{
		dest += (wchar_t)c;
		return true;
	}
	if(c < 0xC0)
	{
		return false;
	}
	int numAdds = GetUtf8ExtraByteCount(c);

	UINT value = (c - kUtf8Limits[numAdds - 1]);
	if (ReadUtf8ExtraBytes(src, i, numAdds, value) == false)
	{
		return false;
	}

	return AppendUtf16(value, dest);
}

int CTextConvert::GetUtf8ExtraByteCount(BYTE c)
{
	int numAdds;
	for (numAdds = 1; numAdds < 5; numAdds++)
		if (c < kUtf8Limits[numAdds])
			break;
	return numAdds;
}

bool CTextConvert::ReadUtf8ExtraBytes(const CStringA &src, int &i, int numAdds, UINT &value)
{
	do
	{
		if (i >= src.GetLength())
		{
			return false;
		}
		BYTE c2 = (BYTE)src[i++];
		if (c2 < 0x80 || c2 >= 0xC0)
		{
			return false;
		}
		value <<= 6;
		value |= (c2 - 0x80);
		numAdds--;
	}while(numAdds > 0);

	return true;
}

bool CTextConvert::AppendUtf16(UINT value, CString &dest)
{
	if (value < 0x10000)
	{
		dest += (wchar_t)(value);
	}
	else
	{
		value -= 0x10000;
		if (value >= 0x100000)
		{
			return false;
		}
		dest += (wchar_t)(0xD800 + (value >> 10));
		dest += (wchar_t)(0xDC00 + (value & 0x3FF));
	}
	return true;
}

bool CTextConvert::ConvertToUTF8(const CString &src, CStringA &dest)
{
#ifdef _UNICODE
	dest.Empty();
	for(int i = 0; i < src.GetLength();)
	{
		if (EncodeUtf8Char(src, i, dest) == false)
		{
			dest = src;
			return false;
		}
	}
#else
	dest = src;
#endif
	return true;
}

bool CTextConvert::EncodeUtf8Char(const CString &src, int &i, CStringA &dest)
{
	UINT value = (UINT)src[i++];
	if (value < 0x80)
	{
		dest += (char)value;
		return true;
	}
	if (value >= 0xD800 && value < 0xE000)
	{
		if (ReadSurrogatePair(src, i, value) == false)
		{
			return false;
		}
	}
	AppendUtf8(value, dest);
	return true;
}

bool CTextConvert::ReadSurrogatePair(const CString &src, int &i, UINT &value)
{
	if (value >= 0xDC00)
	{
		return false;
	}
	if (i >= src.GetLength())
	{
		return false;
	}
	UINT c2 = (UINT)src[i++];
	if (c2 < 0xDC00 || c2 >= 0xE000)
	{
		return false;
	}
	value = ((value - 0xD800) << 10) | (c2 - 0xDC00);
	return true;
}

void CTextConvert::AppendUtf8(UINT value, CStringA &dest)
{
	int numAdds;
	for (numAdds = 1; numAdds < 5; numAdds++)
		if (value < (((UINT)1) << (numAdds * 5 + 6)))
			break;
	dest += (char)(kUtf8Limits[numAdds - 1] + (value >> (6 * numAdds)));
	do
	{
		numAdds--;
		dest += (char)(0x80 + ((value >> (6 * numAdds)) & 0x3F));
	}
	while(numAdds > 0);
}
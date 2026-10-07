#pragma once

#define md5StringLength 32

class CMd5
{
public:
	char* CalcMD5FromString(const char *s8_Input, int len);

	void MD5Init();
	void MD5Update(unsigned char *buf, unsigned len);
	char* MD5FinalToString();

private:
	struct MD5Context 
	{
		unsigned long buf[4];
		unsigned long bits[2];
		unsigned char in[64];
	};
		
	void MD5Final (unsigned char digest[16]);
	void MD5Transform(unsigned long buf[4], unsigned long in[16]);
	void byteReverse (unsigned char *buf, unsigned longs);

	MD5Context ctx{};
	char   ms8_MD5[40]{}; // Output buffer
};
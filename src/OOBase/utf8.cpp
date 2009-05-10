///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "utf8.h"

#include "SmartPtr.h"

#if defined(_WIN32)

std::wstring OOBase::from_utf8(const char* sz)
{
	if (!sz || sz[0]=='\0')
		return std::wstring();

	int len = MultiByteToWideChar(CP_UTF8,0,sz,-1,NULL,0);
	if (!len)
		OOBase_CallCriticalFailure(GetLastError());

	SmartPtr<wchar_t,ArrayDestructor<wchar_t> > ptrBuf;
	OOBASE_NEW(ptrBuf,wchar_t[len+1]);
	if (!ptrBuf)
		OOBase_OutOfMemory();

	if (!MultiByteToWideChar(CP_UTF8,0,sz,-1,ptrBuf.value(),len))
		OOBase_CallCriticalFailure(GetLastError());

	return ptrBuf.value();
}

std::string OOBase::to_utf8(const wchar_t* wsz)
{
	if (!wsz || wsz[0]==L'\0')
		return std::string();

	int len = WideCharToMultiByte(CP_UTF8,0,wsz,-1,NULL,0,NULL,NULL);
	if (!len)
		OOBase_CallCriticalFailure(GetLastError());

	SmartPtr<char,ArrayDestructor<char> > ptrBuf;
	OOBASE_NEW(ptrBuf,char[len+1]);
	if (!ptrBuf)
		OOBase_OutOfMemory();

	if (!WideCharToMultiByte(CP_UTF8,0,wsz,-1,ptrBuf.value(),len,NULL,NULL))
		OOBase_CallCriticalFailure(GetLastError());

	return ptrBuf.value();
}

std::wstring OOBase::from_native(const char* sz)
{
	if (!sz || sz[0]=='\0')
		return std::wstring();

	int len = MultiByteToWideChar(CP_THREAD_ACP,0,sz,-1,NULL,0);
	if (!len)
		OOBase_CallCriticalFailure(GetLastError());

	SmartPtr<wchar_t,ArrayDestructor<wchar_t> > ptrBuf;
	OOBASE_NEW(ptrBuf,wchar_t[len+1]);
	if (!ptrBuf)
		OOBase_OutOfMemory();

	if (!MultiByteToWideChar(CP_THREAD_ACP,0,sz,-1,ptrBuf.value(),len))
		OOBase_CallCriticalFailure(GetLastError());

	return ptrBuf.value();
}

#else

std::wstring OOBase::from_utf8(const char* sz)
{
    if (!sz || *sz=='\0')
        return L"";

	static const int trailingBytesForUTF8[256] =
	{
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 0x00 - 0x1F
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 0x20 - 0x3F
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 0x40 - 0x5F
		0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,	// 0x60 - 0x7F
		9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,	// 0x80 - 0x9F
		9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, 9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,	// 0xA0 - 0xBF
		9,9,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	// 0xC0 - 0xDF
		2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,11,11,11,4,4,4,4,5,5,6,7	// 0xE0 - 0xFF
	};

	std::wstring strRet;
	strRet.reserve(strlen(sz));
	for (const char* p=sz;*p!='\0';)
	{
		unsigned int c;
		unsigned char v = *p++;
		int trailers = trailingBytesForUTF8[v];
		switch (trailers)
		{
		case 0:
			c = v;
			break;

		case 1:
			c = v & 0x1f;
			break;

		case 2:
			c = v & 0xf;
			break;

		case 3:
			c = v & 0x7;
			break;

		default:
			trailers &= 7;
			c = L'\xFFFD';
		}

		for (int i=0;i<trailers;++i,++p)
		{
			if (*p == '\0')
			{
				c = L'\xFFFD';
				break;
			}

			if (c != L'\xFFFD')
			{
				c <<= 6;
				c += (*p & 0x3f);
			}
		}

		if (sizeof(wchar_t) == 2 && (c & 0xFFFF0000))
		{
			// Oh god.. we're big and going to UCS-16
			c -= 0x10000;

#if (OMEGA_BYTE_ORDER == OMEGA_BIG_ENDIAN)
			strRet += static_cast<wchar_t>((c >> 10) | 0xD800);
			strRet += static_cast<wchar_t>((c & 0x3ff) | 0xDC00);
#else
			strRet += static_cast<wchar_t>((c & 0x3ff) | 0xDC00);
			strRet += static_cast<wchar_t>((c >> 10) | 0xD800);
#endif
			continue;
		}

		strRet += static_cast<wchar_t>(c);
	}

	return strRet;
}

std::string OOBase::to_utf8(const wchar_t* wsz)
{
    if (!wsz || *wsz==L'\0')
        return "";

	std::string strRet;
	strRet.reserve(wcslen(wsz)*2);
	for (const wchar_t* p=wsz;*p!=0;)
	{
		char c;
		unsigned int v = *p++;

		if (v <= 0x7f)
			strRet += static_cast<char>(v);
		else if (v <= 0x7FF)
		{
			c = static_cast<char>(v >> 6) | 0xc0;
			strRet += c;
			c = static_cast<char>(v & 0x3f) | 0x80;
			strRet += c;
		}
		else if (v <= 0xFFFF)
		{
			// Invalid range
			if (v > 0xD7FF && v < 0xE00)
				strRet += "\xEF\xBF\xBD";
			else
			{
				c = static_cast<char>(v >> 12) | 0xe0;
				strRet += c;
				c = static_cast<char>((v & 0xfc0) >> 6) | 0x80;
				strRet += c;
				c = static_cast<char>(v & 0x3f) | 0x80;
				strRet += c;
			}
		}
		else if (v <= 0x10FFFF)
		{
			c = static_cast<char>(v >> 18) | 0xf0;
			strRet += c;
			c = static_cast<char>((v & 0x3f000) >> 12) | 0x80;
			strRet += c;
			c = static_cast<char>((v & 0xfc0) >> 6) | 0x80;
			strRet += c;
			c = static_cast<char>(v & 0x3f) | 0x80;
			strRet += c;
		}
		else
			strRet += "\xEF\xBF\xBD";
	}

	return strRet;
}

std::wstring OOBase::from_native(const char* sz)
{
	if (!sz || sz[0]=='\0')
		return std::wstring();

	size_t in_len = strlen(sz) + 1;
	size_t out_len = mbstowcs(NULL,sz,in_len);
	if (out_len == (size_t)-1)
		OOBase_CallCriticalFailureErrno(errno);

	// Always allow room for the NULL terminator
	++out_len;

	SmartPtr<wchar_t,ArrayDestructor<wchar_t> > buf;
	OOBASE_NEW(buf,wchar_t[out_len]);
	if (!buf)
		OOBase_OutOfMemory();

	if (mbstowcs(buf.value(),sz,in_len) == (size_t)-1)
		OOBase_CallCriticalFailureErrno(errno);

	return buf.value();
}

#endif

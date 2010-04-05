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

std::wstring OOBase::from_utf8(const char* sz, size_t len)
{
	if (!sz)
		return std::wstring();

	size_t actual_len = measure_utf8(sz,len);

	SmartPtr<wchar_t,ArrayDestructor<wchar_t> > ptrBuf;
	OOBASE_NEW(ptrBuf,wchar_t[actual_len]);
	if (!ptrBuf)
		OOBase_OutOfMemory();

	from_utf8(ptrBuf,actual_len,sz,len);

	if (len == size_t(-1))
		--actual_len;

	return std::wstring(ptrBuf,actual_len);
}

std::wstring OOBase::from_native(const char* sz, size_t len)
{
	if (!sz)
		return std::wstring();

	size_t actual_len = measure_native(sz,len);

	SmartPtr<wchar_t,ArrayDestructor<wchar_t> > ptrBuf;
	OOBASE_NEW(ptrBuf,wchar_t[actual_len]);
	if (!ptrBuf)
		OOBase_OutOfMemory();

	from_native(ptrBuf,actual_len,sz,len);

	if (len == size_t(-1))
		--actual_len;

	return std::wstring(ptrBuf,actual_len);
}

std::string OOBase::to_utf8(const wchar_t* wsz, size_t len)
{
	if (!wsz)
		return std::string();

	size_t actual_len = measure_utf8(wsz,len);

	SmartPtr<char,ArrayDestructor<char> > ptrBuf;
	OOBASE_NEW(ptrBuf,char[actual_len]);
	if (!ptrBuf)
		OOBase_OutOfMemory();

	to_utf8(ptrBuf,actual_len,wsz,len);

	if (len == size_t(-1))
		--actual_len;

	return std::string(ptrBuf,actual_len);
}

#if defined(_WIN32)

size_t OOBase::measure_utf8(const char* sz, size_t len)
{
	if (!sz)
		return (len == size_t(-1) ? 1 : 0);

	int actual_len = MultiByteToWideChar(CP_UTF8,0,sz,static_cast<int>(len),NULL,0);
	if (actual_len < 1)
		OOBase_CallCriticalFailure(GetLastError());
	
	return static_cast<size_t>(actual_len);
}

size_t OOBase::from_utf8(wchar_t* wsz, size_t wlen, const char* sz, size_t len)
{
	if (!sz)
	{
		if (wlen)
			*wsz = L'\0';
		return (len == size_t(-1) ? 1 : 0);
	}

	int actual_len = MultiByteToWideChar(CP_UTF8,0,sz,static_cast<int>(len),wsz,static_cast<int>(wlen));
	if (actual_len < 1)
		OOBase_CallCriticalFailure(GetLastError());
	
	return static_cast<size_t>(actual_len);
}

size_t OOBase::measure_native(const char* sz, size_t len)
{
	if (!sz)
		return (len == size_t(-1) ? 1 : 0);

	int actual_len = MultiByteToWideChar(CP_THREAD_ACP,0,sz,static_cast<int>(len),NULL,0);
	if (actual_len < 1)
		OOBase_CallCriticalFailure(GetLastError());
	
	return static_cast<size_t>(actual_len);
}

size_t OOBase::from_native(wchar_t* wsz, size_t wlen, const char* sz, size_t len)
{
	if (!sz)
	{
		if (wlen)
			*wsz = L'\0';
		return (len == size_t(-1) ? 1 : 0);
	}

	int actual_len = MultiByteToWideChar(CP_THREAD_ACP,0,sz,static_cast<int>(len),wsz,static_cast<int>(wlen));
	if (actual_len < 1)
		OOBase_CallCriticalFailure(GetLastError());
	
	return static_cast<size_t>(actual_len);
}

size_t OOBase::measure_utf8(const wchar_t* wsz, size_t len)
{
	if (!wsz)
		return (len == size_t(-1) ? 1 : 0);

	int actual_len = WideCharToMultiByte(CP_UTF8,0,wsz,static_cast<int>(len),NULL,0,NULL,NULL);
	if (actual_len < 1)
		OOBase_CallCriticalFailure(GetLastError());
	
	return static_cast<size_t>(actual_len);
}

size_t OOBase::to_utf8(char* sz, size_t len, const wchar_t* wsz, size_t wlen)
{
	if (!wsz)
	{
		if (len)
			*sz = '\0';
		return (wlen == size_t(-1) ? 1 : 0);
	}

	int actual_len = WideCharToMultiByte(CP_UTF8,0,wsz,static_cast<int>(wlen),sz,static_cast<int>(len),NULL,NULL);
	if (actual_len < 1)
	{
		DWORD err = GetLastError();
		if (err == ERROR_INSUFFICIENT_BUFFER)
			return measure_utf8(wsz,wlen);

		OOBase_CallCriticalFailure(err);
	}
	
	return static_cast<size_t>(actual_len);
}

#else

namespace
{
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
}

size_t OOBase::measure_utf8(const char* sz, size_t len)
{
	if (!sz)
		return (len == size_t(-1) ? 1 : 0);

	size_t required_len = 0;
	for (const char* p=sz;len == size_t(-1) ? *p!='\0' : size_t(p-sz)<len;)
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

		for (int i=trailers;i>0;--i,++p)
		{
			if (len == size_t(-1) ? *p=='\0' : size_t(p-sz)>=len)
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
			required_len += 2;
		}
		else
			++required_len;
	}

	if (len == size_t(-1))
		++required_len;
		
	return required_len;
}

size_t OOBase::from_utf8(wchar_t* wsz, size_t wlen, const char* sz, size_t len)
{
    if (!sz)
	{
		if (wlen)
			*wsz = L'\0';
		return (len == size_t(-1) ? 1 : 0);
	}

	wchar_t* wp = wsz;
	size_t required_len = 0;
	for (const char* p=sz;len == size_t(-1) ? *p!='\0' : size_t(p-sz)<len;)
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
			if (len == size_t(-1) ? *p=='\0' : size_t(p-sz)>=len)
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
			required_len += 2;
			c -= 0x10000;

			if (required_len < len)
			{
#if (OMEGA_BYTE_ORDER != OMEGA_BIG_ENDIAN)
				*wp++ = static_cast<wchar_t>((c >> 10) | 0xD800);
				*wp++ = static_cast<wchar_t>((c & 0x3ff) | 0xDC00);
#else
				*wp++ = static_cast<wchar_t>((c & 0x3ff) | 0xDC00);
				*wp++ = static_cast<wchar_t>((c >> 10) | 0xD800);
#endif
			}
		}
		else
		{
			++required_len;
			if (required_len < len)
				*wp++ = static_cast<wchar_t>(c);
		}
	}

	if (len == size_t(-1))
	{
		++required_len;
		if (required_len < len)
			*wp++ = L'\0';
	}

	return required_len;
}

size_t OOBase::measure_utf8(const wchar_t* wsz, size_t len)
{
	if (!wsz)
		return (len == size_t(-1) ? 1 : 0);

	size_t required_len = 0;
	for (const wchar_t* p=wsz;len == size_t(-1) ? *p!=L'\0' : size_t(p-wsz)<len;)
	{
		unsigned int v = *p++;

		if (sizeof(wchar_t) == 2)
		{
			if (v >= 0xD800 && v <= 0xDBFF)
			{
				// Surrogate pair
				v = (v & 0x27FF) << 10;
				if (len == size_t(-1) ? *p==L'\0' : size_t(p-wsz)>=len)
					break;

				v += ((*p++ & 0x23FF) >> 10) + 0x10000;
			}
			else if (v >= 0xDC00 && v <= 0xDFFF)
			{
				// Surrogate pair
				v = (v & 0x23FF) >> 10;
				if (len == size_t(-1) ? *p==L'\0' : size_t(p-wsz)>=len)
					break;

				v += ((*p++ & 0x27FF) << 10) + 0x10000;
			}
		}

		if (v <= 0x7f)
			++required_len;
		else if (v <= 0x7FF)
			required_len += 2;
		else if (v <= 0xFFFF)
			required_len += 3;
		else if (v <= 0x10FFFF)
			required_len += 4;
		else
			required_len += 3;
	}

	if (len == size_t(-1))
		++required_len;

	return required_len;
}

size_t OOBase::to_utf8(char* sz, size_t len, const wchar_t* wsz, size_t wlen)
{
    if (!wsz)
	{
		if (len)
			*sz = '\0';
		return (wlen == size_t(-1) ? 1 : 0);
	}

	char* cp = sz;
	size_t required_len = 0;
	for (const wchar_t* p=wsz;wlen == size_t(-1) ? *p!=L'\0' : size_t(p-wsz)<wlen;)
	{
		unsigned int v = *p++;
		if (sizeof(wchar_t) == 2)
		{
			if (v >= 0xD800 && v <= 0xDBFF)
			{
				// Surrogate pair
				unsigned int hi = (v & 0x3FF);
				if (wlen == size_t(-1) ? *p==L'\0' : size_t(p-wsz)>=wlen)
					break;

				unsigned int lo = (*p++ & 0x3FF);

				v = ((hi << 10) | lo) + 0x10000;
			}
			else if (v >= 0xDC00 && v <= 0xDFFF)
			{
				// Surrogate pair
				unsigned int lo = (v & 0x3FF);
				if (wlen == size_t(-1) ? *p==L'\0' : size_t(p-wsz)>=wlen)
					break;

				unsigned int hi = (*p++ & 0x3FF);

				v = ((hi << 10) | lo) + 0x10000;
			}
		}

		if (v <= 0x7f)
		{
			++required_len;
			if (required_len < len)
				*cp++ = static_cast<char>(v);
		}
		else if (v <= 0x7FF)
		{
			required_len += 2;
			if (required_len < len)
			{
				*cp++ = static_cast<char>(v >> 6) | 0xc0;
				*cp++ = static_cast<char>(v & 0x3f) | 0x80;
			}
		}
		else if (v <= 0xFFFF)
		{
			required_len += 3;
			if (required_len < len)
			{
				// Invalid range
				if (v > 0xD7FF && v < 0xE00)
				{
					*cp++ = '\xEF';
					*cp++ = '\xBF';
					*cp++ = '\xBD';
				}
				else
				{
					*cp++ = static_cast<char>(v >> 12) | 0xe0;
					*cp++ = static_cast<char>((v & 0xfc0) >> 6) | 0x80;
					*cp++ = static_cast<char>(v & 0x3f) | 0x80;
				}
			}
		}
		else if (v <= 0x10FFFF)
		{
			required_len += 4;
			if (required_len < len)
			{
				*cp++ = static_cast<char>(v >> 18) | 0xf0;
				*cp++ = static_cast<char>((v & 0x3f000) >> 12) | 0x80;
				*cp++ = static_cast<char>((v & 0xfc0) >> 6) | 0x80;
				*cp++ = static_cast<char>(v & 0x3f) | 0x80;
			}
		}
		else
		{
			required_len += 3;
			if (required_len < len)
			{
				*cp++ = '\xEF';
				*cp++ = '\xBF';
				*cp++ = '\xBD';
			}
		}
	}

	if (len == size_t(-1))
	{
		++required_len;
		if (required_len < len)
			*cp++ = '\0';
	}

	return required_len;
}

size_t OOBase::measure_native(const char* sz, size_t len)
{
	if (!sz)
		return (len == size_t(-1) ? 1 : 0);

	wchar_t wc[2];
	mbstate_t state = {0};
	size_t required_len = 0;
	for (const char* p=sz;size_t(p-sz)<len;)
	{
		size_t c = (len == size_t(-1) ? MB_CUR_MAX : size_t(p-sz));
		size_t count = mbrtowc(wc,p,c,&state);
		if (count == 0)
		{
			// Null
			++required_len;
			if (len == size_t(-1))
				break;

			++p;
		}
		else if (count > 0)
		{
			required_len += count;
			p += count;
		}
		else if (count == (size_t)-2)
			p += c;
		else
			break;
	}

	return required_len;
}

size_t OOBase::from_native(wchar_t* wsz, size_t wlen, const char* sz, size_t len)
{
	if (!sz)
	{
		if (wlen)
			*wsz = L'\0';
		return (len == size_t(-1) ? 1 : 0);
	}

	wchar_t* wp = wsz;
	mbstate_t state = {0};
	size_t required_len = 0;
	for (const char* p=sz;size_t(p-sz)<len && size_t(wp-wsz)<wlen;)
	{
		size_t c = (len == size_t(-1) ? MB_CUR_MAX : size_t(p-sz));
		size_t count = mbrtowc(wp++,p,c,&state);
		if (count == 0)
		{
			// Null
			++required_len;
			if (len == size_t(-1))
				break;

			++p;
		}
		else if (count > 0)
		{
			required_len += count;
			p += count;
		}
		else if (count == (size_t)-2)
			p += c;
		else
			break;
	}

	return required_len;
}

#endif

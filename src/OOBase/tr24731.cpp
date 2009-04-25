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

#include "config-base.h"

#include <stdarg.h>

#if !defined(HAVE_TR_24731)

int sprintf_s(char* s, size_t n, const char* format, ...)
{
	va_list list;
	va_start(list,format);
	int r = vsnprintf(s,n-1,format,list);
	s[n-1] = '\0';
	va_end(list);
	return r;
}

int swprintf_s(wchar_t* s, size_t n, const wchar_t* format, ...)
{
	va_list list;
	va_start(list,format);
	int r = vsnwprintf(s,n-1,format,list);
	s[n-1] = '\0';
	va_end(list);
	return r;
}

int strerror_s(char *s, size_t maxsize, int errnum)
{
	sprintf_s("%s",maxsize,strerror(errnum));
	return 0;
}

int mbstowcs_s(size_t* retval, wchar_t* dst, size_t dstmax, const char* src, size_t len)
{
	*retval = mbstowcs(dst,src,len);
	return 0;
}

int vsnprintf_s(char* s, size_t n, const char* format, va_list arg)
{
	int r = vsnprintf(s,n-1,format,arg);
	s[n-1] = '\0';
	return r;
}

int wcscpy_s(wchar_t* s1, size_t s1max, const wchar_t* s2)
{
	wcsncpy(s1,s2,s1max-1);
	s1[s1max-1] = L'\0';
	return 0;
}

int strcpy_s(char* s1, size_t s1max, const char* s2)
{
	strncpy(s1,s2,s1max-1);
	s1[s1max-1] = '\0';
	return 0;
}

int vswprintf_s(wchar_t* s, size_t n, const wchar_t* format, va_list arg)
{
	return vswprintf(s,format,arg);
}

int swscanf_s(const wchar_t* s, const wchar_t* format, ...)
{
	va_list list;
	va_start(list,format);
	int r = swscanf(s,format,list);
	va_end(list);
	return r;
}

#endif

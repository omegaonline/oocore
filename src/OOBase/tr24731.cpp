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

int vsnprintf_s(char* s, size_t n, const char* format, va_list arg)
{
	if (!s || !format || !n)
	{
		errno = EINVAL;
		return -1;
	}

	s[0] = '\0';
	int r = vsnprintf(s,n-1,format,arg);
	s[n-1] = '\0';
	return r;
}

int strcpy_s(char* dest, size_t n, const char* src)
{
	if (!dest || !src || !n)
		return EINVAL;

	strncpy(dest,src,n-1);
	dest[n-1] = '\0';
	return 0;
}

#endif

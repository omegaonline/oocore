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

#ifndef OOBASE_TR24731_H_INCLUDED_
#define OOBASE_TR24731_H_INCLUDED_

#if !defined(HAVE_STRING_H)
#error No <string.h>?
#endif

#define __STDC_WANT_LIB_EXT1__ 1
#define __STDC_WANT_SECURE_LIB__ 1
#include <string.h>

#ifndef HAVE_TR_24731
#if (defined(__STDC_LIB_EXT1__) && (__STDC_LIB_EXT1__ >= 200509L)) || (defined(__STDC_SECURE_LIB__) && (__STDC_SECURE_LIB__ >= 200411L))
#define HAVE_TR_24731 1
#endif
#endif

// These are missing from the earlier draft...
#if defined(HAVE_TR_24731) && (!defined(__STDC_LIB_EXT1__) || (__STDC_LIB_EXT1__ < 200509L))

inline int vsnprintf_s_fixed(char* s, rsize_t n, const char* format, va_list arg)
{
	return _vsnprintf_s(s,n,n-1,format,arg);
}
#define vsnprintf_s vsnprintf_s_fixed
#endif

#if !defined(HAVE_TR_24731)

int vsnprintf_s(char* s, size_t n, const char* format, va_list arg);
int vswprintf_s(wchar_t* s, size_t n, const wchar_t* format, va_list arg);

#endif

#endif // OOBASE_TR24731_H_INCLUDED_

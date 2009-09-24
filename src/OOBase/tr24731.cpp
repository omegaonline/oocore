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
	if (!s)
		OOBase_CallCriticalFailure("Null pointer passed to vsnprintf_s");

	s[0] = '\0';

	if (!format)
		OOBase_CallCriticalFailure("Null pointer passed to vsnprintf_s");

	if (n == 0)
		OOBase_CallCriticalFailure("Empty buffer passed to vsnprintf_s");

	if (n == 1)
		OOBase_CallCriticalFailure("Single character buffer passed to vsnprintf_s");
	
	int r = vsnprintf(s,n-1,format,arg);
	s[n-1] = '\0';
	return r;
}

#endif

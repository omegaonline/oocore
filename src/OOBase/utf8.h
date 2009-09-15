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

#ifndef OOBASE_UTF8_H_INCLUDED_
#define OOBASE_UTF8_H_INCLUDED_

#include "config-base.h"

namespace OOBase
{
	size_t measure_utf8(const char* sz, size_t len = (size_t)-1);
	size_t from_utf8(wchar_t* wsz, size_t wlen, const char* sz, size_t len = size_t(-1));
	std::wstring from_utf8(const char* sz, size_t len = size_t(-1));

	size_t measure_utf8(const wchar_t* wsz, size_t len = (size_t)-1);
	size_t to_utf8(char* sz, size_t len, const wchar_t* wsz, size_t wlen = size_t(-1));
	std::string to_utf8(const wchar_t* wsz, size_t len = size_t(-1));

	size_t measure_native(const char* sz, size_t len = (size_t)-1);
	size_t from_native(wchar_t* wsz, size_t wlen, const char* sz, size_t len = size_t(-1));
	std::wstring from_native(const char* sz, size_t len = size_t(-1));
}

#endif // OOBASE_UTF8_H_INCLUDED_

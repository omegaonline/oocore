///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOCORE_FORMATTING_H_INCLUDED_
#define OOCORE_FORMATTING_H_INCLUDED_

namespace OOCore
{
	unsigned int parse_uint_hex(wchar_t c);
	unsigned int parse_uint(wchar_t c);
	unsigned int parse_uint(const wchar_t* sz);
	int parse_int(const wchar_t* sz);
}

#endif // OOCORE_FORMATTING_H_INCLUDED_
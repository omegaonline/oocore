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
	long strtol(const char* sz, char const*& endptr, unsigned int base);
	unsigned long strtoul(const char* sz, char const*& endptr, unsigned int base);
	Omega::int64_t strto64(const char* sz, char const*& endptr, unsigned int base);
	Omega::uint64_t strtou64(const char* sz, char const*& endptr, unsigned int base);
	Omega::float8_t strtod(const char* sz, char const*& endptr);
}

#endif // OOCORE_FORMATTING_H_INCLUDED_

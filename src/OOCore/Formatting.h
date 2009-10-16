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
	Omega::int32_t wcsto32(const wchar_t* sz, wchar_t const*& endptr, unsigned int base);
	Omega::uint32_t wcstou32(const wchar_t* sz, wchar_t const*& endptr, unsigned int base);
	Omega::int64_t wcsto64(const wchar_t* sz, wchar_t const*& endptr, unsigned int base);
	Omega::uint64_t wcstou64(const wchar_t* sz, wchar_t const*& endptr, unsigned int base);
	Omega::float8_t wcstod(const wchar_t* sz, wchar_t const*& endptr);
}

#endif // OOCORE_FORMATTING_H_INCLUDED_

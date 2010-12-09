///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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

#ifndef OMEGA_IO_H_INCLUDED_
#define OMEGA_IO_H_INCLUDED_

namespace Omega
{
	namespace IO
	{
		interface IInputStream : public IObject
		{
			virtual uint32_t ReadBytes(uint32_t lenBytes, byte_t* data) = 0;

			static IInputStream* Create(size_t lenBytes, const byte_t* data, bool_t bCopy = true);
			
#if defined(_WIN32)
			static IInputStream* Create(HANDLE hFile);
#else
			static IInputStream* Create(int fd);
#endif
		};
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE
(
	Omega::IO, IInputStream, "{FFF9E2DF-F364-4CA5-9418-CFBF9217D60F}",

	OMEGA_METHOD(uint32_t,ReadBytes,2,((in),uint32_t,lenBytes,(out)(size_is(lenBytes)),byte_t*,data))
)

OOCORE_EXPORTED_FUNCTION(Omega::IO::IInputStream*,OOCore_IO_CreateInputStream_1,3,((in),size_t,lenBytes,(in)(size_is(lenBytes)),const Omega::byte_t*,data,(in),Omega::bool_t,bCopy))
inline Omega::IO::IInputStream* Omega::IO::IInputStream::Create(size_t lenBytes, const byte_t* data, bool_t bCopy)
{
	return OOCore_IO_CreateInputStream_1(lenBytes,data,bCopy);
}

#if defined(_WIN32)
OOCORE_EXPORTED_FUNCTION(Omega::IO::IInputStream*,OOCore_IO_CreateInputStream_2,1,((in),HANDLE,hFile))
inline Omega::IO::IInputStream* Omega::IO::IInputStream::Create(HANDLE hFile)
{
	return OOCore_IO_CreateInputStream_2(hFile);
}
#else
OOCORE_EXPORTED_FUNCTION(Omega::IO::IInputStream*,OOCore_IO_CreateInputStream_2,1,((in),int,fd))
inline Omega::IO::IInputStream* Omega::IO::IInputStream::Create(int fd)
{
	return OOCore_IO_CreateInputStream_2(fd);
}
#endif

#endif // !defined(DOXYGEN)

#endif // OMEGA_IO_H_INCLUDED_

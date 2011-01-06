///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
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

#ifndef OMEGA_MEMORY_H_INCLUDED_
#define OMEGA_MEMORY_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		// flags: 0 - C++ object - align to size
		//        1 - Buffer - align 32
		//        2 - Thread-local buffer - align 32
		void* Allocate(size_t len, int flags, const char* file = 0, unsigned int line = 0);
		void Free(void* mem, int flags);
	}
}

#define OMEGA_NEW_T2(TYPE,POINTER,CONSTRUCTOR) \
	POINTER = new (::Omega::System::Allocate(sizeof(TYPE),0,__FILE__,__LINE__)) CONSTRUCTOR

#define OMEGA_NEW_T(TYPE,POINTER,CONSTRUCTOR) \
	do { \
		void* OMEGA_NEW_ptr = ::Omega::System::Allocate(sizeof(TYPE),0,__FILE__,__LINE__); \
		try { POINTER = new (OMEGA_NEW_ptr) CONSTRUCTOR; } catch (...) { ::Omega::System::Free(OMEGA_NEW_ptr,0); throw; } \
	} while ((void)0,false)

#define OMEGA_NEW_T_RETURN(TYPE,CONSTRUCTOR) \
	do { \
		void* OMEGA_NEW_ptr = ::Omega::System::Allocate(sizeof(TYPE),0,__FILE__,__LINE__); \
		try { return new (OMEGA_NEW_ptr) CONSTRUCTOR; } catch (...) { ::Omega::System::Free(OMEGA_NEW_ptr,0); throw; } \
	} while ((void)0,false)

#define OMEGA_DELETE(TYPE,POINTER) \
	do { \
		if (POINTER) \
		{ \
			POINTER->~TYPE(); \
			::Omega::System::Free(POINTER,0); \
		} \
	} while ((void)0,false)

#endif // OMEGA_MEMORY_H_INCLUDED_

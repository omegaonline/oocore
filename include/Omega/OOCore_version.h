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

#ifndef OOCORE_VERSION_H_INCLUDED_
#define OOCORE_VERSION_H_INCLUDED_

#define OOCORE_MAJOR_VERSION  0
#define OOCORE_MINOR_VERSION  7
#define OOCORE_PATCH_VERSION  1

#define OOCORE_VERSION_III(n)        #n
#define OOCORE_VERSION_II(a,b,c)     OOCORE_VERSION_III(a.b.c)
#define OOCORE_VERSION_I(a,b,c)      OOCORE_VERSION_II(a,b,c)
#define OOCORE_VERSION               OOCORE_VERSION_I(OOCORE_MAJOR_VERSION,OOCORE_MINOR_VERSION,OOCORE_PATCH_VERSION)

#if defined(_MSC_VER) || defined(RC_INVOKED)
	#define OOCORE_EXTERN __declspec(dllimport)
#elif defined (__GNUC__)
	#if defined(_WIN32)
		#define OOCORE_EXTERN  __attribute__((dllimport))
	#else
		#define OOCORE_EXTERN
	#endif
#else
#error Failed to guess your compiler.  Please contact the omegaonline developers.
#endif

#if !defined(OOCORE_INTERNAL)

#if defined(__cplusplus)
extern "C"
{
#endif // __cplusplus

	OOCORE_EXTERN unsigned int OOCore_GetMajorVersion();
	OOCORE_EXTERN unsigned int OOCore_GetMinorVersion();
	OOCORE_EXTERN unsigned int OOCore_GetPatchVersion();
	OOCORE_EXTERN const char* OOCore_GetVersion();

#if defined(__cplusplus)
} // extern "C"

namespace OOCore
{
	inline const char* GetVersion()
	{
		return OOCore_GetVersion();
	}

	inline unsigned int GetMajorVersion()
	{
		return OOCore_GetMajorVersion();
	}

	inline unsigned int GetMinorVersion()
	{
		return OOCore_GetMinorVersion();
	}

	inline unsigned int GetPatchVersion()
	{
		return OOCore_GetPatchVersion();
	}
}
#endif // __cplusplus

#endif // !OOCORE_INTERNAL

#endif // OOCORE_VERSION_H_INCLUDED_

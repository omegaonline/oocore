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

#ifndef OOBASE_DLL_H_INCLUDED_
#define OOBASE_DLL_H_INCLUDED_

#include "Mutex.h"

#if !defined(_WIN32)
#include <ltdl.h>
#endif

namespace OOBase
{
	class DLL
	{
	public:
		DLL();
		~DLL();

		int load(const char* full_path);
		bool unload();
		void* symbol(const char* sym_name);

	private:
		DLL(const DLL&);
		DLL& operator = (const DLL&);

		/** \var m_module
		 *  The platform specific DLL/SO variable.
		 */
#if defined(_WIN32)
		HMODULE     m_module;
#else
		lt_dlhandle m_module;
#endif
	};
}

#endif // OOBASE_DLL_H_INCLUDED_

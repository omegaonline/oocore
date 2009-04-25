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

#include "DLL.h"

#if defined(_WIN32)

OOBase::DLL::DLL() :
	m_module(0)
{
}

OOBase::DLL::~DLL()
{
	unload();
}

int OOBase::DLL::load(const wchar_t* full_path)
{
	m_module = LoadLibraryW(full_path);
	if (!m_module)
		return GetLastError();

	return 0;
}

bool OOBase::DLL::unload()
{
	if (!m_module)
		return false;
	else if (FreeLibrary(m_module))
	{
		m_module = 0;
		return true;
	}
	else
		return false;
}

void* OOBase::DLL::symbol(const char* sym_name)
{
	if (!m_module)
		return 0;

	return (void*)GetProcAddress(m_module,sym_name);
}

#else

#error Fix me!

#endif


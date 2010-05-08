///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrBase, the Omega Online Base library.
//
// OOSvrBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOSvrBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "Proactor.h"

#if defined(_WIN32)
#include "ProactorWin32.h"
#endif

#if defined(HAVE_EV_H)
#include "ProactorEv.h"
#endif

OOSvrBase::Proactor::Proactor() :
		m_impl(0)
{
#if defined(_WIN32)
	OOBASE_NEW(m_impl,Win32::ProactorImpl());
#elif defined(HAVE_EV_H)
	OOBASE_NEW(m_impl,Ev::ProactorImpl());
#else
#error Fix me!
#endif

	if (!m_impl)
		OOBase_OutOfMemory();
}

OOSvrBase::Proactor::~Proactor()
{
	delete m_impl;
}

OOBase::Socket* OOSvrBase::Proactor::accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES* psa)
{
	return m_impl->accept_local(handler,path,perr,psa);
}

OOSvrBase::AsyncSocket* OOSvrBase::Proactor::attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock)
{
	return m_impl->attach_socket(handler,perr,sock);
}

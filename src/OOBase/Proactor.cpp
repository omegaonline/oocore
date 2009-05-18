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
#else
#error Fix me!
#endif

OOSvrBase::Proactor::Proactor() :
	m_impl(0)
{
	OOBASE_NEW(m_impl,ProactorImpl());
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

OOSvrBase::AsyncSocket* OOSvrBase::Proactor::accept_shared_mem_socket(const std::string& strName, IOHandler* handler, int* perr, OOBase::LocalSocket* via, OOBase::timeval_t* timeout, SECURITY_ATTRIBUTES* psa)
{
	return m_impl->accept_shared_mem_socket(strName,handler,perr,via,timeout,psa);
}

OOSvrBase::AsyncSocket* OOSvrBase::Proactor::connect_shared_mem_socket(IOHandler* handler, int* perr, OOBase::LocalSocket* via, OOBase::timeval_t* timeout)
{
	return m_impl->connect_shared_mem_socket(handler,perr,via,timeout);
}
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

#if defined(HAVE_EV_H)

#include "ProactorEv.h"

OOSvrBase::ProactorImpl::ProactorImpl()
{
}

OOSvrBase::ProactorImpl::~ProactorImpl()
{
}

OOBase::Socket* OOSvrBase::ProactorImpl::accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES*)
{
	void* POSIX_TODO;
	return 0;
}

OOSvrBase::AsyncSocket* OOSvrBase::ProactorImpl::attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock)
{
	void* POSIX_TODO;
	return 0;
}

#endif // HAVE_EV_H

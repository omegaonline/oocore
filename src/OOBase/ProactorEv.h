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

#ifndef OOSVRBASE_PROACTOR_EV_H_INCLUDED_
#define OOSVRBASE_PROACTOR_EV_H_INCLUDED_

#if !defined(OOSVRBASE_PROACTOR_H_INCLUDED_)
#error include "Proactor.h" instead
#endif

#if !defined(HAVE_EV_H)
#error Includes have got confused, check Proactor.h
#endif

namespace OOSvrBase
{
	namespace Ev
	{
		class ProactorImpl : public detail::ProactorImpl
		{
		public:
			ProactorImpl();
			~ProactorImpl();

			OOBase::Socket* accept_local(Acceptor* handler, const std::string& path, int* perr, SECURITY_ATTRIBUTES*);

			AsyncSocket* attach_socket(IOHandler* handler, int* perr, OOBase::Socket* sock);
		};
	}
}

#endif // OOSVRBASE_PROACTOR_EV_H_INCLUDED_

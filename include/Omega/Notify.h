///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2012 Rick Taylor
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

#ifndef OMEGA_NOTIFY_H_INCLUDED_
#define OMEGA_NOTIFY_H_INCLUDED_

#include "./Omega.h"

namespace Omega
{
	namespace Notify
	{
		interface INotifier : public IObject
		{
			typedef std::vector<guid_t,System::STLAllocator<guid_t> > iid_list_t;

			virtual uint32_t RegisterNotify(const guid_t& iid, IObject* pObject) = 0;
			virtual void UnregisterNotify(uint32_t cookie) = 0;

			virtual iid_list_t ListNotifyInterfaces() = 0;
		};
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE
(
	Omega::Notify, INotifier, "{A513378E-AA41-9F7A-8A97-7A7387B83001}",

	// Methods
	OMEGA_METHOD(uint32_t,RegisterNotify,2,((in),const guid_t&,iid,(in)(iid_is(iid)),IObject*,pObject))
	OMEGA_METHOD_VOID(UnregisterNotify,1,((in),uint32_t,cookie))
	OMEGA_METHOD(Notify::INotifier::iid_list_t,ListNotifyInterfaces,0,())
)

#endif // !defined(DOXYGEN)

#endif // OMEGA_NOTIFY_H_INCLUDED_

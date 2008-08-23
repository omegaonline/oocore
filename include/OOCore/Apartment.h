///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OOCORE_APARTMENT_H_INCLUDED_
#define OOCORE_APARTMENT_H_INCLUDED_

#include <OOCore/Remoting.h>

namespace Omega
{
	namespace Apartment
	{
		interface IApartment : public IObject
		{
			virtual void CreateInstance(IObject* pOuter, const guid_t& iid, IObject*& pObject) = 0;

			inline static IApartment* Create(const guid_t& om_oid);
		};
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE_LOCAL
(
	Omega::Apartment, IApartment, "{9D92BFD7-631C-46dd-A123-E9CEB18A2285}",

	OMEGA_METHOD_VOID(CreateInstance,3,((in),IObject*,pOuter,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

OMEGA_EXPORTED_FUNCTION(Omega::Apartment::IApartment*,IApartment_Create,1,((in),const Omega::guid_t&, om_oid));
Omega::Apartment::IApartment* Omega::Apartment::IApartment::Create(const Omega::guid_t& om_oid)
{
	return IApartment_Create(om_oid);
}

#endif // !defined(DOXYGEN)

#endif // OOCORE_APARTMENT_H_INCLUDED_

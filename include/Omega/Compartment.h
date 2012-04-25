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

#ifndef OMEGA_COMPARTMENT_H_INCLUDED_
#define OMEGA_COMPARTMENT_H_INCLUDED_

#include "Omega.h"

namespace Omega
{
	namespace Compartment
	{
		interface ICompartment : public IObject
		{
			virtual void CreateInstance(const any_t& oid, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject) = 0;
		};
		
		// {3BE419D7-52D9-4873-95E7-836D33523C51}
		OOCORE_DECLARE_OID(OID_Compartment);
	}
}

#if !defined(DOXYGEN)

OMEGA_DEFINE_INTERFACE
(
	Omega::Compartment, ICompartment, "{9D92BFD7-631C-46DD-A123-E9CEB18A2285}",

	OMEGA_METHOD_VOID(CreateInstance,4,((in),const any_t&,oid,(in),Activation::Flags_t,flags,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
)

#endif // !defined(DOXYGEN)

#endif // OMEGA_COMPARTMENT_H_INCLUDED_

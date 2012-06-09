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

#ifndef OMEGA_SERVICE_H_INCLUDED_
#define OMEGA_SERVICE_H_INCLUDED_

#include "Omega.h"

namespace Omega
{
	namespace System
	{
		interface IService : public IObject
		{
			virtual void Start(Registry::IKey* pKey) = 0;
			virtual void Stop() = 0;
		};
	}
}

OMEGA_DEFINE_INTERFACE
(
	Omega::System, IService, "{C5BA215C-C59C-4471-96F8-9169F122150A}",

	OMEGA_METHOD_VOID(Start,1,((in),Registry::IKey*,pKey))
	OMEGA_METHOD_VOID(Stop,0,())
)

#endif // OMEGA_SERVICE_H_INCLUDED_

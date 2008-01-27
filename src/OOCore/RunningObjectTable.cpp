///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	ObjectPtr<Remoting::IInterProcessService> GetInterProcessService();
}

ObjectPtr<Remoting::IInterProcessService> OOCore::GetInterProcessService()
{
	try
	{
		ObjectPtr<Remoting::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(Activation::GetRegisteredObject(Remoting::OID_InterProcessService,Activation::InProcess | Activation::DontLaunch,OMEGA_UUIDOF(Remoting::IInterProcessService))));
		return ptrIPS;
	}
	catch (IException* pE2)
	{
		IException* pE = IException::Create(L"Omega::Initialize not called",L"",pE2);
		pE2->Release();
		throw pE;
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IRunningObjectTable*,Activation_GetRunningObjectTable,0,())
{
	return OOCore::GetInterProcessService()->GetRunningObjectTable();
}

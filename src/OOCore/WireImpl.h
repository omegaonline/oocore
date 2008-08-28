///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOCORE_WIREIMPL_H_INCLUDED_
#define OOCORE_WIREIMPL_H_INCLUDED_

namespace OOCore
{
	Omega::System::MetaInfo::IObject_Safe* CreateProxy(const Omega::guid_t& iid, Omega::System::MetaInfo::IProxy_Safe* pProxy, Omega::System::MetaInfo::IMarshaller_Safe* pManager);
	Omega::System::MetaInfo::IStub_Safe* CreateStub(const Omega::guid_t& iid, Omega::System::MetaInfo::IStubController_Safe* pController, Omega::System::MetaInfo::IMarshaller_Safe* pManager, Omega::System::MetaInfo::IObject_Safe* pObject);

	Omega::System::MetaInfo::ITypeInfo_Safe* GetTypeInfo(const Omega::guid_t& iid);
}

#endif // OOCORE_WIREIMPL_H_INCLUDED_

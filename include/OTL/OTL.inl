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

#ifndef OTL_BASE_INL_INCLUDED_
#define OTL_BASE_INL_INCLUDED_

inline bool OTL::ModuleBase::HaveLocks() const
{
	return !m_lockCount.IsZero();
}

inline void OTL::ModuleBase::IncLockCount()
{
	m_lockCount.AddRef();
}

inline void OTL::ModuleBase::DecLockCount()
{
	m_lockCount.Release();
}

inline Omega::Threading::Mutex& OTL::ModuleBase::GetLock()
{
	return m_csMain;
}

inline Omega::IObject* OTL::LibraryModule::GetLibraryObject(const Omega::guid_t& oid, const Omega::guid_t& iid)
{
	const CreatorEntry* g=getCreatorEntries();
	for (size_t i=0; g[i].pfnOid!=0; ++i)
	{
		if (*(g[i].pfnOid)() == oid)
		{
			return g[i].pfnCreate(iid);
		}
	}
	return 0;
}

inline void OTL::ProcessModule::RegisterObjectFactories()
{
	ObjectPtr<Omega::Activation::IRunningObjectTable> ptrROT(Omega::Activation::OID_RunningObjectTable);

	CreatorEntry* g=getCreatorEntries();
	for (size_t i=0; g[i].pfnOid!=0; ++i)
	{
		ObjectPtr<Omega::Activation::IObjectFactory> ptrOF;
		ptrOF.Attach(static_cast<Omega::Activation::IObjectFactory*>(g[i].pfnCreate(OMEGA_GUIDOF(Omega::Activation::IObjectFactory))));

		g[i].cookie = ptrROT->RegisterObject(*(g[i].pfnOid)(),ptrOF,(*g[i].pfnRegistrationFlags)());
	}
}

inline void OTL::ProcessModule::UnregisterObjectFactories()
{
	ObjectPtr<Omega::Activation::IRunningObjectTable> ptrROT(Omega::Activation::OID_RunningObjectTable);

	CreatorEntry* g=getCreatorEntries();
	for (size_t i=0; g[i].pfnOid!=0; ++i)
	{
		ptrROT->RevokeObject(g[i].cookie);
		g[i].cookie = 0;
	}
}

inline void OTL::ProcessModule::Run()
{
	RegisterObjectFactories();

	try
	{
		while (Omega::HandleRequest(30000) || HaveLocks())
		{}
	}
	catch (...)
	{
		UnregisterObjectFactories();
		throw;
	}

	UnregisterObjectFactories();
}

#endif  // OTL_BASE_INL_INCLUDED_

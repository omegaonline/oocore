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

inline Omega::IObject* OTL::LibraryModule::GetLibraryObject(const Omega::guid_t& oid, const Omega::guid_t& iid)
{
	for (const CreatorEntry* g=getCreatorEntries();g->pfnOid != NULL;++g)
	{
		if (*(g->pfnOid)() == oid)
			return g->pfnCreate(iid);
	}
	return NULL;
}

inline void OTL::ProcessModule::RegisterObjectFactories(Omega::Activation::IRunningObjectTable* pROT)
{
	ObjectPtr<Omega::Activation::IRunningObjectTable> ptrROT = pROT;
	if (ptrROT)
		ptrROT.AddRef();
	else
		ptrROT.GetObject(Omega::Activation::OID_RunningObjectTable_Instance);

	Omega::Threading::Guard guard(m_lock);

	for (CreatorEntry* g=getCreatorEntries();g->pfnOid != NULL; ++g)
	{
		ObjectPtr<Omega::Activation::IObjectFactory> ptrOF = static_cast<Omega::Activation::IObjectFactory*>(g->pfnCreate(OMEGA_GUIDOF(Omega::Activation::IObjectFactory)));
		g->cookie = ptrROT->RegisterObject(*(g->pfnOid)(),ptrOF,(*g->pfnRegistrationFlags)());
	}
}

inline void OTL::ProcessModule::RegisterObjectFactory(const Omega::guid_t& oid, Omega::Activation::IRunningObjectTable* pROT)
{
	for (CreatorEntry* g=getCreatorEntries();g->pfnOid != NULL; ++g)
	{
		if (*(g->pfnOid)() == oid)
		{
			ObjectPtr<Omega::Activation::IRunningObjectTable> ptrROT = pROT;
			if (ptrROT)
				ptrROT.AddRef();
			else
				ptrROT.GetObject(Omega::Activation::OID_RunningObjectTable_Instance);

			Omega::Threading::Guard guard(m_lock);

			ObjectPtr<Omega::Activation::IObjectFactory> ptrOF = static_cast<Omega::Activation::IObjectFactory*>(g->pfnCreate(OMEGA_GUIDOF(Omega::Activation::IObjectFactory)));

			g->cookie = ptrROT->RegisterObject(*(g->pfnOid)(),ptrOF,(*g->pfnRegistrationFlags)());
			break;
		}
	}
}

inline void OTL::ProcessModule::UnregisterObjectFactories(Omega::Activation::IRunningObjectTable* pROT)
{
	ObjectPtr<Omega::Activation::IRunningObjectTable> ptrROT = pROT;
	if (ptrROT)
		ptrROT.AddRef();
	else
		ptrROT.GetObject(Omega::Activation::OID_RunningObjectTable_Instance);

	Omega::Threading::Guard guard(m_lock);

	for (CreatorEntry* g=getCreatorEntries();g->pfnOid != NULL; ++g)
	{
		if (g->cookie)
		{
			ptrROT->RevokeObject(g->cookie);
			g->cookie = 0;
		}
	}
}

inline void OTL::ProcessModule::UnregisterObjectFactory(const Omega::guid_t& oid, Omega::Activation::IRunningObjectTable* pROT)
{
	for (CreatorEntry* g=getCreatorEntries(); g->pfnOid != NULL; ++g)
	{
		if (*(g->pfnOid)() == oid)
		{
			ObjectPtr<Omega::Activation::IRunningObjectTable> ptrROT = pROT;
			if (ptrROT)
				ptrROT.AddRef();
			else
				ptrROT.GetObject(Omega::Activation::OID_RunningObjectTable_Instance);

			Omega::Threading::Guard guard(m_lock);

			ptrROT->RevokeObject(g->cookie);
			g->cookie = 0;
			break;
		}
	}
}

inline void OTL::ProcessModule::Run()
{
	RegisterObjectFactories();

	try
	{
		while (Omega::HandleRequest(30000) || HaveLocks() || !Omega::CanUnload())
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

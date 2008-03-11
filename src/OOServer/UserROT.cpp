///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOServer, the OmegaOnline Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer.h"
#include "./UserROT.h"

using namespace Omega;
using namespace OTL;

void User::RunningObjectTable::Init(ObjectPtr<Remoting::IObjectManager> ptrOM)
{
	if (ptrOM)
	{
		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOM->CreateRemoteInstance(Remoting::OID_InterProcessService,OMEGA_UUIDOF(Remoting::IInterProcessService),0,pIPS);
		ObjectPtr<Remoting::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));

		// Get the running object table
		m_ptrROT.Attach(ptrIPS->GetRunningObjectTable());
	}
}

void User::RunningObjectTable::Register(const guid_t& oid, Activation::IRunningObjectTable::Flags_t flags, IObject* pObject)
{
	if (m_ptrROT && (flags & Activation::IRunningObjectTable::AllowAnyUser))
	{
		// Route to sandbox!
		m_ptrROT->Register(oid,flags,pObject);
	}
	else
	{
		try
		{
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapObjects.find(oid);
			if (i != m_mapObjects.end())
			{
				bool bOk = true;
			
				// QI for IWireProxy and check its still there!
				ObjectPtr<System::MetaInfo::IWireProxy> ptrProxy = i->second;
				if (ptrProxy)
				{
					if (!ptrProxy->IsAlive())
						bOk = false;
				}

				if (bOk)
					OMEGA_THROW(EALREADY);
			}

			m_mapObjects.insert(std::map<guid_t,ObjectPtr<IObject> >::value_type(oid,pObject));
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(string_t(e.what(),false));
		}
	}
}

void User::RunningObjectTable::Revoke(const guid_t& oid)
{
	bool bFound = false;

	try
	{
		OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapObjects.find(oid);
		if (i != m_mapObjects.end())
		{
			m_mapObjects.erase(i);
			bFound = true;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	if (!bFound && m_ptrROT)
	{
		// Route to sandbox
		m_ptrROT->Revoke(oid);
	}
}

IObject* User::RunningObjectTable::GetObject(const guid_t& oid)
{
	try
	{
		bool bOk = true;
		{
			OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapObjects.find(oid);
			if (i != m_mapObjects.end())
			{
				// QI for IWireProxy and check its still there!
				ObjectPtr<System::MetaInfo::IWireProxy> ptrProxy = i->second;
				if (ptrProxy)
				{
					if (!ptrProxy->IsAlive())
						bOk = false;
				}

				if (bOk)
					return i->second.AddRef();
			}
		}

		if (!bOk)
		{
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			m_mapObjects.erase(oid);
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	if (m_ptrROT)
	{
		// Route to sandbox
		return m_ptrROT->GetObject(oid);
	}

	return 0;
}

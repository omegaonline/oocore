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
		ptrOM->CreateUnboundProxy(Remoting::OID_InterProcess,OMEGA_UUIDOF(Remoting::IInterProcessService),pIPS);
		ObjectPtr<Remoting::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));

		// Get the service table
		m_ptrSIP.Attach(ptrIPS->GetRunningObjectTable());
	}
}

void User::RunningObjectTable::Register(const guid_t& oid, Activation::IRunningObjectTable::Flags_t flags, IObject* pObject)
{
	if (m_ptrSIP && (flags & Activation::IRunningObjectTable::AllowAnyUser))
	{
		// Route to sandbox!
		m_ptrSIP->Register(oid,flags,pObject);
	}
	else
	{
		try
		{
			OOSERVER_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			if (m_mapServices.find(oid) != m_mapServices.end())
				OOSERVER_THROW_ERRNO(EALREADY);

			m_mapServices.insert(std::map<guid_t,ObjectPtr<IObject> >::value_type(oid,pObject));
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

		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i != m_mapServices.end())
		{
			m_mapServices.erase(i);
			bFound = true;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	if (!bFound && m_ptrSIP)
	{
		// Route to sandbox
		m_ptrSIP->Revoke(oid);
	}
}

void User::RunningObjectTable::GetObject(const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	bool bFound = false;

	try
	{
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i != m_mapServices.end())
		{
			pObject = i->second->QueryInterface(iid);
			bFound = true;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	if (!bFound && m_ptrSIP)
	{
		// Route to sandbox
		m_ptrSIP->GetObject(oid,iid,pObject);
	}
}

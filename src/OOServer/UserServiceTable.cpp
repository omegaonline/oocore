#include "OOServer.h"
#include ".\UserServiceTable.h"

using namespace Omega;
using namespace OTL;

void User::ServiceTable::Init(ObjectPtr<Remoting::IObjectManager> ptrOM)
{
	if (ptrOM)
	{
		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOM->CreateUnboundProxy(Remoting::OID_InterProcess,Remoting::IID_IInterProcessService,pIPS);
		ObjectPtr<Remoting::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<Remoting::IInterProcessService*>(pIPS));

		// Get the service table
		m_ptrSIP.Attach(ptrIPS->GetServiceTable());
	}
}

void User::ServiceTable::Register(const guid_t& oid, Activation::IServiceTable::Flags_t flags, IObject* pObject)
{
	if (m_ptrSIP && (flags & Activation::IServiceTable::AllowAnyUser))
	{
		// Route to sandbox!
		m_ptrSIP->Register(oid,flags,pObject);
	}
	else
	{
		ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

		if (m_mapServices.find(oid) != m_mapServices.end())
			OOSERVER_THROW_ERRNO(EALREADY);

		m_mapServices.insert(std::map<guid_t,ObjectPtr<IObject> >::value_type(oid,pObject));
	}
}

void User::ServiceTable::Revoke(const guid_t& oid)
{
	bool bFound = false;
	{
		ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i != m_mapServices.end())
		{
			m_mapServices.erase(i);
			bFound = true;
		}
	}

	if (!bFound && m_ptrSIP)
	{
		// Route to sandbox
		m_ptrSIP->Revoke(oid);
	}
}

void User::ServiceTable::GetObject(const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	bool bFound = false;
	{
		ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i != m_mapServices.end())
		{
			pObject = i->second->QueryInterface(iid);
			bFound = true;
		}
	}

	if (!bFound && m_ptrSIP)
	{
		// Route to sandbox
		m_ptrSIP->GetObject(oid,iid,pObject);
	}
}
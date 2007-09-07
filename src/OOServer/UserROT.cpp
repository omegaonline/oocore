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

			if (m_mapServices.find(oid) != m_mapServices.end())
			{
				// QI for IWireProxy and check its still there!

				void* TODO;

				OOSERVER_THROW_ERRNO(EALREADY);
			}

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
		OOSERVER_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i != m_mapServices.end())
		{
			return i->second.AddRefReturn();
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

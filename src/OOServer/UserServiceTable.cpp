#include "OOServer.h"
#include ".\UserServiceTable.h"
#include ".\UserManager.h"

using namespace Omega;
using namespace OTL;

void UserServiceTable::Init(UserManager* pManager, bool bIsSandbox)
{
	m_pManager = pManager;
	m_bIsSandbox = bIsSandbox;
}

void UserServiceTable::Register(const guid_t& oid, Activation::IServiceTable::Flags_t flags, IObject* pObject)
{
	if (!m_bIsSandbox && (flags & Activation::IServiceTable::AllowAnyUser))
	{
		// Route to sandbox!
	}
	else
	{
		ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOSERVER_THROW_LASTERROR());

		if (m_mapServices.find(oid) != m_mapServices.end())
			OOSERVER_THROW_ERRNO(EALREADY);

		m_mapServices.insert(std::map<guid_t,ObjectPtr<IObject> >::value_type(oid,pObject));
	}
}

void UserServiceTable::Revoke(const guid_t& oid)
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

	if (!bFound && !m_bIsSandbox)
	{
		// Route to sandbox
	}
}

void UserServiceTable::GetObject(const guid_t& oid, const guid_t& iid, IObject*& pObject)
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

	if (!bFound && !m_bIsSandbox)
	{
		// Route to sandbox
	}
}

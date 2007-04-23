#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class ServiceTableImpl :
		public ObjectBase,
		public Activation::IServiceTable
	{
	public:
		void Register(const guid_t& oid, Activation::IServiceTable::Flags_t flags, IObject* pObject);
		void Revoke(const guid_t& oid);
		void GetObject(const guid_t& oid, const guid_t& iid, IObject*& pObject);

		BEGIN_INTERFACE_MAP(ServiceTableImpl)
			INTERFACE_ENTRY(Activation::IServiceTable)
		END_INTERFACE_MAP()

	private:
		ACE_Thread_Mutex                      m_lock;
		std::map<guid_t,ObjectPtr<IObject> >  m_mapServices;
	};

	// The instance wide ServiceTable instance
	struct ServiceTable
	{
		ObjectPtr<Activation::IServiceTable>  m_ptrSystemServiceTable;
		ACE_Thread_Mutex                      m_lock;
	};
	ServiceTable	g_ServiceTable;
}

// ServiceTable
void SetServiceTable(Activation::IServiceTable* pNewTable)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,OOCore::g_ServiceTable.m_lock,OOCORE_THROW_LASTERROR());

	OOCore::g_ServiceTable.m_ptrSystemServiceTable = pNewTable;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IServiceTable*,Activation_GetServiceTable,0,())
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,OOCore::g_ServiceTable.m_lock,OOCORE_THROW_LASTERROR());

	// If we have no set ServiceTable, use a default
	if (!OOCore::g_ServiceTable.m_ptrSystemServiceTable)
	{
		ObjectPtr<ObjectImpl<OOCore::ServiceTableImpl> > ptrServiceTable = ObjectImpl<OOCore::ServiceTableImpl>::CreateObjectPtr();

		OOCore::g_ServiceTable.m_ptrSystemServiceTable.Attach(ptrServiceTable.Detach());
	}

	return OOCore::g_ServiceTable.m_ptrSystemServiceTable.AddRefReturn();
}

void OOCore::ServiceTableImpl::Register(const guid_t& oid, Activation::IServiceTable::Flags_t, IObject* pObject)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

	if (m_mapServices.find(oid) != m_mapServices.end())
		OOCORE_THROW_ERRNO(EALREADY);

	m_mapServices.insert(std::map<guid_t,ObjectPtr<IObject> >::value_type(oid,pObject));
}

void OOCore::ServiceTableImpl::Revoke(const guid_t& oid)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

	std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
	if (i == m_mapServices.end())
		OOCORE_THROW_ERRNO(EINVAL);

	m_mapServices.erase(i);
}

void OOCore::ServiceTableImpl::GetObject(const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	ACE_GUARD_REACTION(ACE_Thread_Mutex,guard,m_lock,OOCORE_THROW_LASTERROR());

	std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
	if (i == m_mapServices.end())
		OOCORE_THROW_ERRNO(EINVAL);

	pObject = i->second->QueryInterface(iid);
}

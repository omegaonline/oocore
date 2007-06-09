#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class ServiceTable :
		public ObjectBase,
		public Activation::IServiceTable
	{
	public:
		ServiceTable() {}

		void Register(const guid_t& oid, Activation::IServiceTable::Flags_t flags, IObject* pObject);
		void Revoke(const guid_t& oid);
		void GetObject(const guid_t& oid, const guid_t& iid, IObject*& pObject);

		BEGIN_INTERFACE_MAP(ServiceTable)
			INTERFACE_ENTRY(Activation::IServiceTable)
		END_INTERFACE_MAP()

	private:
		ServiceTable(const ServiceTable&) :
            ObjectBase(), Activation::IServiceTable()
        {}
		ServiceTable& operator = (const ServiceTable&) { return *this; }

		ACE_RW_Thread_Mutex                   m_lock;
		std::map<guid_t,ObjectPtr<IObject> >  m_mapServices;
	};

	// The instance wide ServiceTable instance
	struct GlobalServiceTable
	{
		ObjectPtr<Activation::IServiceTable>  m_ptrSystemServiceTable;
		ACE_Thread_Mutex                      m_lock;
	};
	GlobalServiceTable	g_ServiceTable;

	void SetServiceTable(Activation::IServiceTable* pNewTable);
}

// ServiceTable
void OOCore::SetServiceTable(Activation::IServiceTable* pNewTable)
{
	OOCORE_GUARD(ACE_Thread_Mutex,guard,OOCore::g_ServiceTable.m_lock);

	OOCore::g_ServiceTable.m_ptrSystemServiceTable = pNewTable;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IServiceTable*,Activation_GetServiceTable,0,())
{
	// If we have no set ServiceTable, use a default
	if (!OOCore::g_ServiceTable.m_ptrSystemServiceTable)
	{
		// Do a double lock here...
		OOCORE_GUARD(ACE_Thread_Mutex,guard,OOCore::g_ServiceTable.m_lock);

		if (!OOCore::g_ServiceTable.m_ptrSystemServiceTable)
		{
			ObjectPtr<ObjectImpl<OOCore::ServiceTable> > ptrServiceTable = ObjectImpl<OOCore::ServiceTable>::CreateInstancePtr();

			OOCore::g_ServiceTable.m_ptrSystemServiceTable.Attach(ptrServiceTable.Detach());
		}
	}

	return OOCore::g_ServiceTable.m_ptrSystemServiceTable.AddRefReturn();
}

void OOCore::ServiceTable::Register(const guid_t& oid, Activation::IServiceTable::Flags_t, IObject* pObject)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		if (m_mapServices.find(oid) != m_mapServices.end())
			OOCORE_THROW_ERRNO(EALREADY);

		m_mapServices.insert(std::map<guid_t,ObjectPtr<IObject> >::value_type(oid,pObject));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

void OOCore::ServiceTable::Revoke(const guid_t& oid)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i == m_mapServices.end())
			OOCORE_THROW_ERRNO(EINVAL);

		m_mapServices.erase(i);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

void OOCore::ServiceTable::GetObject(const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i == m_mapServices.end())
			OOCORE_THROW_ERRNO(EINVAL);

		pObject = i->second->QueryInterface(iid);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

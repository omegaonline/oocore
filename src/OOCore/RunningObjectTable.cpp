#include "OOCore_precomp.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class RunningObjectTable :
		public ObjectBase,
		public Activation::IRunningObjectTable
	{
	public:
		RunningObjectTable() {}

		void Register(const guid_t& oid, Activation::IRunningObjectTable::Flags_t flags, IObject* pObject);
		void Revoke(const guid_t& oid);
		IObject* GetObject(const guid_t& oid);

		BEGIN_INTERFACE_MAP(RunningObjectTable)
			INTERFACE_ENTRY(Activation::IRunningObjectTable)
		END_INTERFACE_MAP()

	private:
		RunningObjectTable(const RunningObjectTable&) : ObjectBase(), Activation::IRunningObjectTable() {}
		RunningObjectTable& operator = (const RunningObjectTable&) { return *this; }

		ACE_RW_Thread_Mutex                   m_lock;
		std::map<guid_t,ObjectPtr<IObject> >  m_mapServices;
	};

	// The instance wide RunningObjectTable instance
	struct GlobalRunningObjectTable
	{
		ObjectPtr<Activation::IRunningObjectTable>  m_ptrSystemROT;
		ACE_Thread_Mutex                            m_lock;
	};
	GlobalRunningObjectTable	g_ROT;

	void SetRunningObjectTable(Activation::IRunningObjectTable* pNewTable);
}

// RunningObjectTable
void OOCore::SetRunningObjectTable(Activation::IRunningObjectTable* pNewTable)
{
	OOCORE_GUARD(ACE_Thread_Mutex,guard,OOCore::g_ROT.m_lock);

	OOCore::g_ROT.m_ptrSystemROT = pNewTable;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Activation::IRunningObjectTable*,Activation_GetRunningObjectTable,0,())
{
	// If we have no set RunningObjectTable, use a default
	if (!OOCore::g_ROT.m_ptrSystemROT)
	{
		// Do a double lock here...
		OOCORE_GUARD(ACE_Thread_Mutex,guard,OOCore::g_ROT.m_lock);

		if (!OOCore::g_ROT.m_ptrSystemROT)
		{
			ObjectPtr<ObjectImpl<OOCore::RunningObjectTable> > ptrRunningObjectTable = ObjectImpl<OOCore::RunningObjectTable>::CreateInstancePtr();

			OOCore::g_ROT.m_ptrSystemROT.Attach(ptrRunningObjectTable.Detach());
		}
	}

	return OOCore::g_ROT.m_ptrSystemROT.AddRefReturn();
}

void OOCore::RunningObjectTable::Register(const guid_t& oid, Activation::IRunningObjectTable::Flags_t, IObject* pObject)
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
		OMEGA_THROW(string_t(e.what(),false));
	}
}

void OOCore::RunningObjectTable::Revoke(const guid_t& oid)
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
		OMEGA_THROW(string_t(e.what(),false));
	}
}

IObject* OOCore::RunningObjectTable::GetObject(const guid_t& oid)
{
	OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	try
	{
		std::map<guid_t,ObjectPtr<IObject> >::iterator i=m_mapServices.find(oid);
		if (i == m_mapServices.end())
			OOCORE_THROW_ERRNO(EINVAL);

		return i->second.AddRefReturn();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

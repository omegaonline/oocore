#ifndef OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_
#define OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_

namespace User
{
	class RunningObjectTable :
		public OTL::ObjectBase,
		public Omega::Activation::IRunningObjectTable
	{
	public:
		RunningObjectTable() {}

		void Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM);

		void Register(const Omega::guid_t& oid, Omega::Activation::IRunningObjectTable::Flags_t flags, Omega::IObject* pObject);
		void Revoke(const Omega::guid_t& oid);
		void GetObject(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject);

		BEGIN_INTERFACE_MAP(RunningObjectTable)
			INTERFACE_ENTRY(Omega::Activation::IRunningObjectTable)
		END_INTERFACE_MAP()

	private:
		RunningObjectTable(const RunningObjectTable&) : OTL::ObjectBase() {}
		RunningObjectTable& operator = (const RunningObjectTable&) { return *this; }

		OTL::ObjectPtr<Omega::Activation::IRunningObjectTable>   m_ptrROT;
		ACE_RW_Thread_Mutex                                      m_lock;
		std::map<Omega::guid_t,OTL::ObjectPtr<Omega::IObject> >  m_mapServices;
	};
}

#endif // OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_

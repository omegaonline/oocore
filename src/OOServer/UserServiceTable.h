#ifndef OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_
#define OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_

namespace User
{

class ServiceTable :
	public OTL::ObjectBase,
	public Omega::Activation::IServiceTable
{
public:
	void Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOM);

	void Register(const Omega::guid_t& oid, Omega::Activation::IServiceTable::Flags_t flags, Omega::IObject* pObject);
	void Revoke(const Omega::guid_t& oid);
	void GetObject(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject);

	BEGIN_INTERFACE_MAP(ServiceTable)
		INTERFACE_ENTRY(Omega::Activation::IServiceTable)
	END_INTERFACE_MAP()

private:
	OTL::ObjectPtr<Omega::Activation::IServiceTable>         m_ptrSIP;
	ACE_Thread_Mutex                                         m_lock;
	std::map<Omega::guid_t,OTL::ObjectPtr<Omega::IObject> >  m_mapServices;
};

}

#endif // OOSERVER_USER_SERVICE_TABLE_H_INCLUDED_

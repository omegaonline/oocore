#ifndef OOCORE_OBJECT_MANAGER_H_INCLUDED_
#define OOCORE_OBJECT_MANAGER_H_INCLUDED_

namespace OOCore
{
	class StdObjectManager :
		public OTL::ObjectBase,
		public OTL::AutoObjectFactoryNoAggregation<StdObjectManager,&Omega::Remoting::OID_StdObjectManager>,
		public Omega::Remoting::IObjectManager,
		public Omega::System::MetaInfo::IWireManager
	{
	public:
		StdObjectManager();
		virtual ~StdObjectManager();

		BEGIN_INTERFACE_MAP(StdObjectManager)
			INTERFACE_ENTRY(Omega::Remoting::IObjectManager)
			INTERFACE_ENTRY(Omega::System::MetaInfo::IWireManager)
		END_INTERFACE_MAP()

	private:
		StdObjectManager(const StdObjectManager&) : OTL::ObjectBase(),Omega::Remoting::IObjectManager(),Omega::System::MetaInfo::IWireManager() {};
		StdObjectManager& operator = (const StdObjectManager&) { return *this; };

		ACE_RW_Thread_Mutex							m_lock;
		OTL::ObjectPtr<Omega::Remoting::IChannel>	m_ptrChannel;
		Omega::uint32_t								m_uNextStubId;

		std::map<Omega::IObject*,Omega::uint32_t>                                      m_mapStubObjs;
		std::map<Omega::uint32_t,OTL::ObjectPtr<Omega::System::MetaInfo::IWireStub> >  m_mapStubIds;
		std::map<Omega::uint32_t,OTL::ObjectPtr<Omega::System::MetaInfo::IWireProxy> > m_mapProxyIds;

	// IWireManager members
	public:
		void MarshalInterface(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnmarshalInterface(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::IObject*& pObject);
		void ReleaseStub(Omega::uint32_t id);
		Omega::Serialize::IFormattedStream* CreateOutputStream();
		Omega::Serialize::IFormattedStream* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pParams, Omega::uint16_t timeout);

	// IObjectManager members
	public:
		void Connect(Omega::Remoting::IChannel* pChannel);
		void Invoke(Omega::Serialize::IFormattedStream* pParamsIn, Omega::Serialize::IFormattedStream* pParamsOut, Omega::uint32_t timeout);
		void Disconnect();
		void CreateUnboundProxy(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_OBJECT_MANAGER_H_INCLUDED_

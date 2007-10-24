#ifndef OOCORE_OBJECT_MANAGER_H_INCLUDED_
#define OOCORE_OBJECT_MANAGER_H_INCLUDED_

namespace OOCore
{
	class WireStub : 
		public OTL::ObjectBase,
		public Omega::IObject
	{
	public:
		WireStub() : m_stub_id(0)
		{}

		virtual ~WireStub()
		{}

		void Init(Omega::IObject* pObject, Omega::uint32_t stub_id, Omega::System::MetaInfo::IWireManager* pManager);
		void MarshalInterface(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid);
		Omega::System::MetaInfo::IWireStub* UnmarshalStub(Omega::Serialize::IFormattedStream* pStream);
		Omega::IObject* GetStubObject();
		
		BEGIN_INTERFACE_MAP(WireStub)
			INTERFACE_ENTRY(IObject)
		END_INTERFACE_MAP()

	private:
		Omega::uint32_t                                       m_stub_id;
		OTL::ObjectPtr<IObject>                               m_ptrObj;
		ACE_RW_Thread_Mutex                                   m_lock;
		OTL::ObjectPtr<Omega::System::MetaInfo::IWireManager> m_ptrManager;

		std::map<const Omega::guid_t,OTL::ObjectPtr<Omega::System::MetaInfo::IWireStub> > m_iid_map;

		OTL::ObjectPtr<Omega::System::MetaInfo::IWireStub> FindStub(const Omega::guid_t& iid);
	};

	class WireProxy : 
		public OTL::ObjectBase,
		public Omega::System::MetaInfo::IWireProxy
	{
	public:
		WireProxy() : m_proxy_id(0)
		{}

		virtual ~WireProxy()
		{}

		void Init(Omega::uint32_t proxy_id, Omega::System::MetaInfo::IWireManager* pManager);
		IObject* UnmarshalInterface(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid);
		void WriteKey(Omega::Serialize::IFormattedStream* pStream);

		BEGIN_INTERFACE_MAP(WireProxy)
			INTERFACE_ENTRY(Omega::System::MetaInfo::IWireProxy)
			INTERFACE_ENTRY_NOINTERFACE(Omega::System::MetaInfo::SafeProxy)
			INTERFACE_ENTRY_FUNCTION_BLIND(QI)
		END_INTERFACE_MAP()

	private:
		Omega::uint32_t                                       m_proxy_id;
		ACE_RW_Thread_Mutex                                   m_lock;
		OTL::ObjectPtr<Omega::System::MetaInfo::IWireManager> m_ptrManager;

		std::map<const Omega::guid_t,OTL::ObjectPtr<Omega::IObject> > m_iid_map;

		static Omega::IObject* QI(WireProxy* pThis, const Omega::guid_t& iid)
		{
			return pThis->QI2(iid);
		}
		Omega::IObject* QI2(const Omega::guid_t& iid);
	};

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

		ACE_RW_Thread_Mutex                       m_lock;
		OTL::ObjectPtr<Omega::Remoting::IChannel> m_ptrChannel;
		Omega::uint32_t                           m_uNextStubId;

		std::map<OTL::ObjectPtr<Omega::IObject>,OTL::ObjectPtr<WireStub> > m_mapStubObjs;
		std::map<Omega::uint32_t,OTL::ObjectPtr<WireStub> >                m_mapStubIds;
		std::map<Omega::uint32_t,OTL::ObjectPtr<WireProxy> >               m_mapProxyIds;

	// IWireManager members
	public:
		void MarshalInterface(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::IObject* pObject);
		void UnmarshalInterface(Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::IObject*& pObject);
		void ReleaseStub(Omega::uint32_t id);
		Omega::Serialize::IFormattedStream* CreateOutputStream(IObject* pOuter = 0);
		Omega::Serialize::IFormattedStream* SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pParams, Omega::uint16_t timeout);

	// IObjectManager members
	public:
		void Connect(Omega::Remoting::IChannel* pChannel);
		void Invoke(Omega::Serialize::IFormattedStream* pParamsIn, Omega::Serialize::IFormattedStream* pParamsOut);
		void Disconnect();
		void CreateRemoteInstance(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::IObject* pOuter, Omega::IObject*& pObject);
	};
}

#endif // OOCORE_OBJECT_MANAGER_H_INCLUDED_

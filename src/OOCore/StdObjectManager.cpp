#include "OOCore_precomp.h"

#include "./StdObjectManager.h"

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class UnboundProxy :
		public ObjectBase,
		public MetaInfo::IWireProxy
	{
	public:
		void init(MetaInfo::IWireManager* pManager, const guid_t& oid, const guid_t& iid)
		{
			const MetaInfo::qi_rtti* pRtti = MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateWireProxy)
				INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

			this->m_ptrProxy.Attach(pRtti->pfnCreateWireProxy(this,pManager));
			this->m_ptrManager = pManager;
			m_oid = oid;
			m_iid = iid;
		}

	BEGIN_INTERFACE_MAP(UnboundProxy)
		INTERFACE_ENTRY(MetaInfo::IWireProxy)
		INTERFACE_ENTRY_FUNCTION_BLIND(QI)
	END_INTERFACE_MAP()

	private:
		guid_t                              m_oid;
		guid_t                              m_iid;
		ObjectPtr<MetaInfo::IWireManager>   m_ptrManager;
		ObjectPtr<IObject>                  m_ptrProxy;

        static IObject* QI(const guid_t& iid, void* pThis, void*)
        {
            return static_cast<RootClass*>(pThis)->QI2(iid);
        }

		IObject* QI2(const guid_t& iid)
		{
			if (iid != m_iid)
				return 0;

			return m_ptrProxy->QueryInterface(iid);
		}

	// IWireProxy members
	public:
		void WriteKey(Serialize::IFormattedStream* pStream)
		{
			pStream->WriteUInt32(0);
			MetaInfo::wire_write(m_ptrManager,pStream,m_oid);
			MetaInfo::wire_write(m_ptrManager,pStream,m_iid);
		}
	};

	class StdProxy :
		public ObjectBase,
		public MetaInfo::IWireProxy
	{
	public:
		StdProxy() {}

		void init(MetaInfo::IWireManager* pManager, const guid_t& iid, uint32_t uId)
		{
			const MetaInfo::qi_rtti* pRtti = MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateWireProxy)
				INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

			try
			{
				ObjectPtr<IObject> ptrObj;
				ptrObj.Attach(pRtti->pfnCreateWireProxy(this,pManager));
				m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(iid,ptrObj));
				ptrObj.Detach();
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e.what());
			}

			m_ptrManager = pManager;
			m_uId = uId;
		}

	BEGIN_INTERFACE_MAP(StdProxy)
		INTERFACE_ENTRY(MetaInfo::IWireProxy)
		INTERFACE_ENTRY_FUNCTION_BLIND(QI)
	END_INTERFACE_MAP()

	private:
		StdProxy(const StdProxy&) {};
		StdProxy& operator = (const StdProxy&) { return *this; };

		ACE_RW_Thread_Mutex                          m_lock;
		std::map<const guid_t,ObjectPtr<IObject> >   m_iid_map;
		ObjectPtr<MetaInfo::IWireManager>            m_ptrManager;
		uint32_t                                     m_uId;
		
        static IObject* QI(const guid_t& iid, void* pThis, void*)
        {
			return static_cast<RootClass*>(pThis)->QI2(iid);
        }

		inline IObject* QI2(const guid_t& iid);

	// IWireProxy members
	public:
		void WriteKey(Serialize::IFormattedStream* pStream)
		{
			pStream->WriteUInt32(m_uId);
		}
	};
}

IObject* OOCore::StdProxy::QI2(const guid_t& iid)
{
	try
	{
		ObjectPtr<IObject> ptrNew;
		ObjectPtr<IObject> ptrQI;

		// Lookup first...
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<const guid_t,ObjectPtr<IObject> >::iterator i=m_iid_map.find(iid);
			if (i != m_iid_map.end())
			{
				if (i->second)
					return i->second->QueryInterface(iid);
				else
					return 0;
			}
			else
			{
				// QI all entries
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					ptrQI = i->second->QueryInterface(iid);
					if (ptrQI)
					{
						ptrNew = i->second;
						break;
					}
				}
			}
		}

		if (!ptrNew)
		{
			if (iid==Omega::MetaInfo::IID_SafeProxy)
				return 0;

			// New interface required
			const MetaInfo::qi_rtti* pRtti = MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateWireProxy)
				INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

			ptrNew.Attach(pRtti->pfnCreateWireProxy(this,m_ptrManager));
		}

		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<const guid_t,ObjectPtr<IObject> >::iterator,bool> p = m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(iid,ptrNew));
		if (!p.second)
		{
			ptrNew = 0;
			ptrNew.Attach(p.first->second);
		}
		
		if (ptrQI)
			return ptrQI.AddRefReturn();

		if (ptrNew)
			return ptrNew->QueryInterface(iid);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	return 0;
}

OOCore::StdObjectManager::StdObjectManager() :
	m_uNextStubId(1)
{
}

OOCore::StdObjectManager::~StdObjectManager()
{
}

void OOCore::StdObjectManager::Connect(Remoting::IChannel* pChannel)
{
	if (m_ptrChannel)
		OOCORE_THROW_ERRNO(EALREADY);

	m_ptrChannel = pChannel;
}

void OOCore::StdObjectManager::Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout)
{
	if (!pParamsIn)
		OOCORE_THROW_ERRNO(EINVAL);

	ObjectPtr<MetaInfo::IWireStub> ptrStub;
	uint32_t method_id;

	// Read the stub id and method id
	uint32_t stub_id = pParamsIn->ReadUInt32();

	if (stub_id == 0)
	{
		// It's a static interface call...
		// N.B. This will always use standard marshalling

		// Read the oid and iid
		guid_t oid;
		MetaInfo::wire_read(this,pParamsIn,oid);
		guid_t iid;
		MetaInfo::wire_read(this,pParamsIn,iid);

		method_id = pParamsIn->ReadUInt32();

		// IObject interface calls are not allowed on static interfaces!
		if (method_id < 3)
			OOCORE_THROW_ERRNO(EINVAL);

		// Create the required object
		ObjectPtr<IObject> ptrObject;
		ptrObject.Attach(Activation::CreateInstance(oid,Activation::Any,0,iid));

		// Get the handler for the interface
		const MetaInfo::qi_rtti* pRtti = MetaInfo::get_qi_rtti_info(iid);
		if (!pRtti || !pRtti->pfnCreateWireStub)
			INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

		// And create a stub
		ptrStub.Attach(pRtti->pfnCreateWireStub(this,ptrObject,0));
		if (!ptrStub)
			INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);
	}
	else
	{
		// It's a method call on a stub...

		// Look up the stub
		try
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<uint32_t,ObjectPtr<MetaInfo::IWireStub> >::const_iterator i=m_mapStubIds.find(stub_id);
			if (i==m_mapStubIds.end())
				OMEGA_THROW("Bad stub id");
			ptrStub = i->second;
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(e.what());
		}

		// Read the method id
		method_id = pParamsIn->ReadUInt32();
	}

	// Assume we succeed...
	pParamsOut->WriteBoolean(true);

	void* TODO; // TODO decrease the wait time...

	// Ask the stub to make the call
	ptrStub->Invoke(method_id,pParamsIn,pParamsOut,timeout);
}

void OOCore::StdObjectManager::Disconnect()
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	// clear the stub map
	m_mapStubIds.clear();
}

void OOCore::StdObjectManager::MarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject)
{
	// See if object is NULL
	if (!pObject)
	{
		pStream->WriteByte(0);
		return;
	}

	// See if pObject does custom marshalling...
	ObjectPtr<Remoting::IMarshal> ptrMarshal(pObject);
	if (ptrMarshal)
	{
		guid_t oid = ptrMarshal->GetUnmarshalOID(iid,0);
		if (oid != guid_t::NIL)
		{
			// Write the marshalling oid
			pStream->WriteByte(2);
			MetaInfo::wire_write(this,pStream,oid);
			
			// Let the custom handle marshalling...
			ptrMarshal->MarshalInterface(pStream,iid,0);

			// Done
			return;
		}
	}

	// Get the handler for the interface
	const MetaInfo::qi_rtti* pRtti = MetaInfo::get_qi_rtti_info(iid);
	if (!pRtti || !pRtti->pfnCreateWireStub)
		INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

	// Generate a new key and stub pair
	ObjectPtr<MetaInfo::IWireStub> ptrStub;
	uint32_t uId = 0;
	try
	{
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		uId = m_uNextStubId++;
		while (uId<1 || m_mapStubIds.find(uId)!=m_mapStubIds.end())
		{
			uId = m_uNextStubId++;
		}

		// Create a stub
		ptrStub.Attach(pRtti->pfnCreateWireStub(this,pObject,uId));
		if (!ptrStub)
			INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

		// Add to the map...
		m_mapStubIds.insert(std::map<uint32_t,ObjectPtr<MetaInfo::IWireStub> >::value_type(uId,ptrStub));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}

	// Write out the data
	pStream->WriteByte(1);
	MetaInfo::wire_write(this,pStream,iid);
	pStream->WriteUInt32(uId);
}

void OOCore::StdObjectManager::UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject)
{
	// Still wondering if iid is actually required....
	byte_t flag = pStream->ReadByte();
	if (flag == 0)
	{
		pObject = 0;
	}
	else if (flag == 1)
	{
		guid_t wire_iid;
		MetaInfo::wire_read(this,pStream,wire_iid);
		uint32_t uId = pStream->ReadUInt32();

		ObjectPtr<ObjectImpl<OOCore::StdProxy> > ptrProxy = ObjectImpl<OOCore::StdProxy>::CreateInstancePtr();
		ptrProxy->init(this,wire_iid,uId);

		if (iid == guid_t::NIL)
			pObject = ptrProxy->QueryInterface(wire_iid);
		else
			pObject = ptrProxy->QueryInterface(iid);
	}
	else if (flag == 2)
	{
     	guid_t oid;
		MetaInfo::wire_read(this,pStream,oid);
		
		// TODO Create an instance of Oid
		// QI for IMarshal,
		// Call UnmarshalInterface(iid)
		::DebugBreak();
		void* TODO;
	}
	else
		OOCORE_THROW_ERRNO(EINVAL);
}

void OOCore::StdObjectManager::ReleaseStub(uint32_t uId)
{
	try
	{
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,ObjectPtr<MetaInfo::IWireStub> >::iterator i=m_mapStubIds.find(uId);
		if (i==m_mapStubIds.end())
			OOCORE_THROW_ERRNO(EINVAL);

		m_mapStubIds.erase(i);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e.what());
	}
}

Omega::Serialize::IFormattedStream* OOCore::StdObjectManager::CreateOutputStream()
{
	return m_ptrChannel->CreateOutputStream();
}

Omega::Serialize::IFormattedStream* OOCore::StdObjectManager::SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pStream)
{
    ObjectPtr<Serialize::IFormattedStream> ptrResponse;
	ptrResponse.Attach(m_ptrChannel->SendAndReceive(attribs,pStream));

	if (!(attribs & Remoting::asynchronous))
	{
		if (!ptrResponse)
			OMEGA_THROW("No response received");

		// Read exception status
		if (!ptrResponse->ReadBoolean())
		{
			// Unmarshal the exception
			IException* pE;
			MetaInfo::wire_read(this,ptrResponse,pE);
			guid_t iid = pE->ActualIID();
			const MetaInfo::qi_rtti* pRtti = MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnThrow)
				INoInterfaceException::Throw(iid,OMEGA_SOURCE_INFO);

			pRtti->pfnThrow(pE);
		}
	}

	return ptrResponse.Detach();
}

void OOCore::StdObjectManager::CreateUnboundProxy(const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	if (pObject)
		pObject->Release();

	ObjectPtr<ObjectImpl<OOCore::UnboundProxy> > ptrProxy = ObjectImpl<OOCore::UnboundProxy>::CreateInstancePtr();
	ptrProxy->init(this,oid,iid);

	pObject = ptrProxy->QueryInterface(iid);
}

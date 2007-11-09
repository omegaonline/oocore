#include "OOCore_precomp.h"

#include "./StdObjectManager.h"

#if defined(ACE_WIN32) && !defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS) && defined (__GNUC__)
#include <setjmp.h>
#endif

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	guid_t read_guid(Serialize::IFormattedStream* pStream)
	{
		guid_t val;
		val.Data1 = pStream->ReadUInt32();
		val.Data2 = pStream->ReadUInt16();
		val.Data3 = pStream->ReadUInt16();
		uint32_t bytes = 8;
		pStream->ReadBytes(bytes,val.Data4);
		if (bytes != 8)
			OOCORE_THROW_LASTERROR();
		return val;
	}

	void write_guid(Serialize::IFormattedStream* pStream, const guid_t& val)
	{
		pStream->WriteUInt32(val.Data1);
		pStream->WriteUInt16(val.Data2);
		pStream->WriteUInt16(val.Data3);
		pStream->WriteBytes(8,val.Data4);
	}

	struct wire_holder
	{
		static System::MetaInfo::IWireStub_Safe* CreateWireStub(const guid_t& iid, System::MetaInfo::IWireManager_Safe* pManager, System::MetaInfo::IObject_Safe* pObject);
		static System::MetaInfo::IObject_Safe* CreateWireProxy(const guid_t& iid, System::MetaInfo::IWireProxy_Safe* pProxy, System::MetaInfo::IWireManager_Safe* pManager);

		typedef System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnCreateWireStub)(
			System::MetaInfo::marshal_info<System::MetaInfo::IWireStub*&>::safe_type::type retval,
			System::MetaInfo::marshal_info<System::MetaInfo::IWireManager*>::safe_type::type pManager,
			System::MetaInfo::marshal_info<IObject*>::safe_type::type pObject);

		typedef System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnCreateWireProxy)(
			System::MetaInfo::marshal_info<IObject*&>::safe_type::type retval,
			System::MetaInfo::marshal_info<System::MetaInfo::IWireProxy*>::safe_type::type pProxy,
			System::MetaInfo::marshal_info<System::MetaInfo::IWireManager*>::safe_type::type pManager);

		struct pfns
		{
			wire_holder::pfnCreateWireProxy pfnProxy;
			wire_holder::pfnCreateWireStub pfnStub;
		};

		static wire_holder& instance()
		{
			static wire_holder i;
			return i;
		}

		std::map<guid_t,pfns> map;
	};

	class WireStub : public System::MetaInfo::IObject_Safe
	{
	public:
		WireStub(System::MetaInfo::IObject_Safe* pObjS, uint32_t stub_id, StdObjectManager* pManager) : 
			m_refcount(0), m_stub_id(stub_id), m_pObjS(pObjS), m_pManager(pManager)
		{
			m_pManager->AddRef_Safe();
			m_pObjS->AddRef_Safe();
		}

		virtual ~WireStub()
		{
			for (std::map<const guid_t,System::MetaInfo::IWireStub_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				i->second->Release_Safe();
			}
			m_pObjS->Release_Safe();
			m_pManager->AddRef_Safe();
		}

		System::MetaInfo::IException_Safe* MarshalInterface(System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t& iid)
		{
			try
			{
				System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IWireStub_Safe> ptrStub(FindStub(iid));
			}
			catch (IException* pE)
			{
				return System::MetaInfo::return_safe_exception(pE);
			}

			System::MetaInfo::IException_Safe* pSE = pStream->WriteUInt32_Safe(m_stub_id);
			if (pSE)
				return pSE;

			return System::MetaInfo::wire_write(pStream,iid);
		}

		System::MetaInfo::IException_Safe* ReleaseMarshalData(System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t&)
		{
			uint32_t v;
			System::MetaInfo::IException_Safe* pSE = pStream->ReadUInt32_Safe(&v);
			if (pSE)
				return pSE;

			guid_t iid;
			return System::MetaInfo::wire_read(pStream,iid);
		}

		System::MetaInfo::IWireStub_Safe* LookupStub(Serialize::IFormattedStream* pStream)
		{
			return FindStub(read_guid(pStream));
		}

	// IObject_Safe methods
	public:
		void OMEGA_CALL AddRef_Safe()
		{
			++m_refcount;
		}

		void OMEGA_CALL Release_Safe()
		{
			if (--m_refcount==0)
				delete this;
		}

		System::MetaInfo::IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, System::MetaInfo::IObject_Safe** ppS)
		{
			*ppS = 0;
			if (*piid == OMEGA_UUIDOF(IObject))
			{
				*ppS = this;
				(*ppS)->AddRef_Safe();
			}
			return 0;
		}

		void OMEGA_CALL Pin() {}
		void OMEGA_CALL Unpin() {}
				
	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,uint32_t>       m_refcount;
		ACE_RW_Thread_Mutex                            m_lock;
		uint32_t                                       m_stub_id;
		System::MetaInfo::IObject_Safe*                m_pObjS;
		StdObjectManager*                              m_pManager;

		std::map<const guid_t,System::MetaInfo::IWireStub_Safe*> m_iid_map;

		System::MetaInfo::IWireStub_Safe* FindStub(const guid_t& iid);
	};

	class WireProxy : 
		public System::MetaInfo::IWireProxy_Safe,
		public System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class
	{
	public:
		WireProxy(uint32_t proxy_id, StdObjectManager* pManager) : 
			m_refcount(0), m_marshal_count(0), m_proxy_id(proxy_id), m_pManager(pManager)
		{
			m_pManager->AddRef_Safe();
		}

		virtual ~WireProxy()
		{
			for (std::map<const guid_t,System::MetaInfo::IObject_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				i->second->Release_Safe();
			}
			m_pManager->RemoveProxy(m_proxy_id);
			m_pManager->Release_Safe();
		}

		System::MetaInfo::IObject_Safe* UnmarshalInterface(System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t& iid);
		
	// IObject_Safe methods
	public:
		void OMEGA_CALL AddRef_Safe()
		{
			++m_refcount;
		}

		void OMEGA_CALL Release_Safe()
		{
			if (--m_refcount==0)
				delete this;
		}

		System::MetaInfo::IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, System::MetaInfo::IObject_Safe** ppS);

		void OMEGA_CALL Pin() {}
		void OMEGA_CALL Unpin() {}
		
	// IWireProxy_Safe members
	public:
		System::MetaInfo::IException_Safe* OMEGA_CALL WriteKey_Safe(System::MetaInfo::IFormattedStream_Safe* pStream)
		{
			return pStream->WriteUInt32_Safe(m_proxy_id);
		}

	// IMarshal_Safe members
	public:
		System::MetaInfo::IException_Safe* OMEGA_CALL GetUnmarshalFactoryOID_Safe(guid_t* pRet, const guid_t*, Remoting::IMarshal::Flags_t)
		{
			*pRet = OID_WireProxyMarshalFactory;
			return 0;
		}

		System::MetaInfo::IException_Safe* OMEGA_CALL MarshalInterface_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, Remoting::IMarshal::Flags_t flags);
		System::MetaInfo::IException_Safe* OMEGA_CALL ReleaseMarshalData_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, Remoting::IMarshal::Flags_t flags);
	
	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,uint32_t>       m_refcount;
		ACE_Atomic_Op<ACE_Thread_Mutex,uint32_t>       m_marshal_count;
		ACE_RW_Thread_Mutex                            m_lock;
		uint32_t                                       m_proxy_id;
		StdObjectManager*                              m_pManager;

		std::map<const guid_t,System::MetaInfo::IObject_Safe*> m_iid_map;

		bool CallRemoteQI(const guid_t& iid);
		uint32_t CallRemoteStubMarshal(Remoting::IObjectManager* pObjectManager, const guid_t& iid);
	};
}

void OOCore::WireProxyMarshalFactory::UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t, Omega::IObject*& pObject)
{
	IObject* pOM = 0;
	pObjectManager->UnmarshalInterface(pStream,OMEGA_UUIDOF(Remoting::IObjectManager),pOM);

	OTL::ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(static_cast<Remoting::IObjectManager*>(pOM));

	ptrOM->UnmarshalInterface(pStream,iid,pObject);
}

void OOCore::StdObjectManagerMarshalFactory::UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::IMarshal::Flags_t, Omega::IObject*& pObject)
{
	IObject* pObj = 0;
	pObjectManager->UnmarshalInterface(pStream,OMEGA_UUIDOF(IObject),pObj);

	OTL::ObjectPtr<IObject> ptrObj;
	ptrObj.Attach(pObj);

	OTL::ObjectPtr<Remoting::IObjectManager> ptrOM(pObj);
	if (!ptrOM)
	{
		OTL::ObjectPtr<Remoting::IChannel> ptrChannel(pObj);
		if (ptrChannel)
		{
			ptrOM = ObjectPtr<Remoting::IObjectManager>(Remoting::OID_StdObjectManager,Activation::InProcess);
			ptrOM->Connect(ptrChannel);
		}
	}

	if (!ptrOM)
		throw INoInterfaceException::Create(OMEGA_UUIDOF(Remoting::IChannel),OMEGA_SOURCE_INFO);

	pObject = ptrOM->QueryInterface(iid);	
}

System::MetaInfo::IWireStub_Safe* OOCore::wire_holder::CreateWireStub(const guid_t& iid, System::MetaInfo::IWireManager_Safe* pManager, System::MetaInfo::IObject_Safe* pObjS)
{
	wire_holder::pfns p;
	try
	{
		std::map<guid_t,wire_holder::pfns>::const_iterator i=wire_holder::instance().map.find(iid);
		if (i == wire_holder::instance().map.end())
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
		p = i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	System::MetaInfo::IWireStub_Safe* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnStub(&pRet,pManager,pObjS);

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

System::MetaInfo::IObject_Safe* OOCore::wire_holder::CreateWireProxy(const guid_t& iid, System::MetaInfo::IWireProxy_Safe* pProxy, System::MetaInfo::IWireManager_Safe* pManager)
{
	wire_holder::pfns p;
	try
	{
		std::map<guid_t,wire_holder::pfns>::const_iterator i=wire_holder::instance().map.find(iid);
		if (i == wire_holder::instance().map.end())
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
		p = i->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	System::MetaInfo::IObject_Safe* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnProxy(&pRet,pProxy,pManager);

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_RegisterWireFactories,3,((in),const guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub))
{
	OOCore::wire_holder::pfns funcs;
	funcs.pfnProxy = (OOCore::wire_holder::pfnCreateWireProxy)(pfnProxy);
	funcs.pfnStub = (OOCore::wire_holder::pfnCreateWireStub)(pfnStub);

	try
	{
		OOCore::wire_holder::instance().map.insert(std::map<guid_t,OOCore::wire_holder::pfns>::value_type(iid,funcs));
	}
	catch (...)
	{}
}

System::MetaInfo::IWireStub_Safe* OOCore::WireStub::FindStub(const guid_t& iid)
{
	try
	{
		System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IWireStub_Safe> ptrStub;
		bool bAdd = false;
		
		// See if we have a stub for this interface already...
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<const guid_t,System::MetaInfo::IWireStub_Safe*>::iterator i=m_iid_map.find(iid);
			if (i != m_iid_map.end())
				ptrStub = i->second;
			
			if (!ptrStub)
			{
				// See if any known interface supports the new interface
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					bool_t bSupports = false;
					System::MetaInfo::IException_Safe* pSE = i->second->SupportsInterface_Safe(System::MetaInfo::marshal_info<bool_t&>::safe_type::coerce(bSupports),&iid);
					if (pSE)
						throw_correct_exception(pSE);
						
					if (bSupports)
					{
						bAdd = true;
						ptrStub = i->second;
						break;
					}
				}
			}
		}

		if (!ptrStub)
		{
			// Check whether underlying object supports interface
			System::MetaInfo::IObject_Safe* pQI = 0;
			System::MetaInfo::IException_Safe* pSE = m_pObjS->QueryInterface_Safe(&iid,&pQI);
			if (pSE)
				throw_correct_exception(pSE);
			if (!pQI)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			System::MetaInfo::auto_iface_safe_ptr<IObject_Safe> ptrQI(pQI);
						
			// Create a stub for this interface
			ptrStub.attach(wire_holder::CreateWireStub(iid,m_pManager,ptrQI));
			if (!ptrStub)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			bAdd = true;
		}

		if (bAdd)
		{
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
				
			std::pair<std::map<const guid_t,System::MetaInfo::IWireStub_Safe*>::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,System::MetaInfo::IWireStub_Safe*>::value_type(iid,ptrStub));
			if (!p.second)
				ptrStub = p.first->second;
			else
				ptrStub->AddRef_Safe();
		}

		ptrStub->AddRef_Safe();
		return ptrStub;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

System::MetaInfo::IObject_Safe* OOCore::WireProxy::UnmarshalInterface(System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t& iid)
{
	try
	{
		System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IObject_Safe> ptrProxy;

		guid_t wire_iid;
		System::MetaInfo::IException_Safe* pSE = System::MetaInfo::wire_read(pStream,wire_iid);
		if (pSE)
			System::MetaInfo::throw_correct_exception(pSE);

		bool bAdd = false;
		
		// See if we have a proxy for this interface already...
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<const guid_t,System::MetaInfo::IObject_Safe*>::iterator i=m_iid_map.find(wire_iid);
			if (i != m_iid_map.end())
				ptrProxy = i->second;
			
			if (!ptrProxy)
			{
				// See if any known interface supports the new interface
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{		
					System::MetaInfo::IObject_Safe* pQI = 0;
					pSE = i->second->QueryInterface_Safe(&wire_iid,&pQI);
					if (pSE)
						System::MetaInfo::throw_correct_exception(pSE);

					if (pQI)
					{
						bAdd = true;
						ptrProxy = i->second;
						pQI->Release_Safe();
						break;
					}
				}
			}
		}

		if (!ptrProxy)
		{
			// Create a new proxy for this interface
			ptrProxy.attach(wire_holder::CreateWireProxy(wire_iid,this,m_pManager));
			bAdd = true;
		}

		if (bAdd)
		{
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
				
			std::pair<std::map<const guid_t,System::MetaInfo::IObject_Safe*>::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,System::MetaInfo::IObject_Safe*>::value_type(wire_iid,ptrProxy));
			if (!p.second)
				ptrProxy = p.first->second;
			else
				ptrProxy->AddRef_Safe();
		}

		++m_marshal_count;
		
		if (iid == OMEGA_UUIDOF(IObject))
		{
			AddRef_Safe();
			return static_cast<System::MetaInfo::IWireProxy_Safe*>(this);
		}

		System::MetaInfo::IObject_Safe* pQI = 0;
		pSE = ptrProxy->QueryInterface_Safe(&iid,&pQI);
		if (pSE)
			System::MetaInfo::throw_correct_exception(pSE);
		return pQI;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

bool OOCore::WireProxy::CallRemoteQI(const guid_t& iid)
{
	ObjectPtr<Serialize::IFormattedStream> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateOutputStream());

	pParamsOut->WriteUInt32(m_proxy_id);
	write_guid(pParamsOut,OMEGA_UUIDOF(IObject));
	pParamsOut->WriteUInt32(1);
	write_guid(pParamsOut,iid);
    
	Serialize::IFormattedStream* pParamsIn = 0;
	IException* pE = m_pManager->SendAndReceive(0,pParamsOut,pParamsIn,0);
	if (pE)
		throw pE;

	ObjectPtr<Serialize::IFormattedStream> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	return ptrParamsIn->ReadBoolean();
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireProxy::QueryInterface_Safe(const guid_t* piid, System::MetaInfo::IObject_Safe** ppS)
{
	if (*piid == OMEGA_UUIDOF(IObject) ||
		*piid == OMEGA_UUIDOF(System::MetaInfo::IWireProxy))
	{
		*ppS = static_cast<System::MetaInfo::IWireProxy_Safe*>(this);
		(*ppS)->AddRef_Safe();
		return 0;
	}
	else if (*piid == OMEGA_UUIDOF(Remoting::IMarshal))
	{
		*ppS = static_cast<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class*>(this);
		(*ppS)->AddRef_Safe();
		return 0;
	}
	
	*ppS = 0;

	System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IObject_Safe> ptrProxy;
		
	try
	{
		bool bAdd = false;

		// See if we have a proxy for this interface already...
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<const guid_t,System::MetaInfo::IObject_Safe*>::iterator i=m_iid_map.find(*piid);
			if (i != m_iid_map.end())
				ptrProxy = i->second;
			
			if (!ptrProxy)
			{
				// See if any known interface supports the new interface
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{		
					System::MetaInfo::IObject_Safe* pQI = 0;
					System::MetaInfo::IException_Safe* pSE = i->second->QueryInterface_Safe(piid,&pQI);
					if (pSE)
						return pSE;

					if (pQI)
					{
						pQI->Release_Safe();
						bAdd = true;
						ptrProxy = i->second;
						break;
					}
				}
			}
		}

		if (!ptrProxy)
		{
			// Send a packet to the other end to see if the stub supports the interface
			if (!CallRemoteQI(*piid))
				return 0;

			// Create a new proxy for this interface
			ptrProxy.attach(wire_holder::CreateWireProxy(*piid,this,m_pManager));
			bAdd = true;
		}

		if (bAdd)
		{
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
				
			std::pair<std::map<const guid_t,System::MetaInfo::IObject_Safe*>::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,System::MetaInfo::IObject_Safe*>::value_type(*piid,ptrProxy));
			if (!p.second)
				ptrProxy = p.first->second;
			else
				ptrProxy->AddRef_Safe();
		}
	}
	catch (std::exception& e)
	{
		return System::MetaInfo::return_safe_exception(IException::Create(string_t(e.what(),false),OMEGA_SOURCE_INFO));
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}

	return ptrProxy->QueryInterface_Safe(piid,ppS);
}

uint32_t OOCore::WireProxy::CallRemoteStubMarshal(Remoting::IObjectManager* pObjectManager, const guid_t& iid)
{
	ObjectPtr<Serialize::IFormattedStream> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateOutputStream());

	pParamsOut->WriteUInt32(m_proxy_id);
	write_guid(pParamsOut,OMEGA_UUIDOF(IObject));
	pParamsOut->WriteUInt32(2);

	write_guid(pParamsOut,iid);
	m_pManager->MarshalInterface(pParamsOut,OMEGA_UUIDOF(System::MetaInfo::IWireManager),pObjectManager);
    
	Serialize::IFormattedStream* pParamsIn = 0;
	IException* pE = 0;
	try
	{
		pE = m_pManager->SendAndReceive(0,pParamsOut,pParamsIn,0);
	}
	catch (...)
	{
		pParamsOut->ReadUInt32();
		read_guid(pParamsOut);
		pParamsOut->ReadUInt32();
		read_guid(pParamsOut);
		m_pManager->ReleaseMarshalData(pParamsOut,OMEGA_UUIDOF(Remoting::IObjectManager),pObjectManager);

		throw;
	}
	ObjectPtr<Serialize::IFormattedStream> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	if (pE)
		throw pE;

	// Read the new stub's key
	uint32_t ret = ptrParamsIn->ReadUInt32();
	read_guid(ptrParamsIn);
	return ret;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireProxy::MarshalInterface_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, Remoting::IMarshal::Flags_t)
{
	// Tell the stub to expect incoming requests from a different channel...
	uint32_t new_key = 0;
	try
	{
		new_key = CallRemoteStubMarshal(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(pObjectManager),*piid);
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}

	// Marshal our own manager out...
	System::MetaInfo::IException_Safe* pSE = pObjectManager->MarshalInterface_Safe(pStream,&OMEGA_UUIDOF(System::MetaInfo::IWireManager),static_cast<System::MetaInfo::IWireManager_Safe*>(m_pManager));
	if (pSE)
		return pSE;

	// The following format is the same as StdObjectManager::UnmarshalInterface...
	size_t undo_count = 0;
	pSE = pStream->WriteByte_Safe(1);
	if (pSE)
		goto Cleanup;
	++undo_count;

	pSE = pStream->WriteUInt32_Safe(new_key);
	if (pSE)
		goto Cleanup;
	++undo_count;

	pSE = System::MetaInfo::wire_write(pStream,*piid);
	if (pSE)
		goto Cleanup;
	++undo_count;

	return 0;
Cleanup:

	System::MetaInfo::IException_Safe* pSE2 = 0;
	if (undo_count > 0)
	{
		byte_t v;
		pSE2 = pStream->ReadByte_Safe(&v);
		if (pSE2)
			return pSE2;
	}

	if (undo_count > 1)
	{
		uint32_t v;
		pSE2 = pStream->ReadUInt32_Safe(&v);
		if (pSE2)
			return pSE2;
	}

	if (undo_count > 2)
	{
		guid_t v;
		pSE2 = System::MetaInfo::wire_read(pStream,v);
		if (pSE2)
			return pSE2;
	}

	if (undo_count > 3)
	{
		pSE2 = pObjectManager->ReleaseMarshalData_Safe(pStream,&OMEGA_UUIDOF(System::MetaInfo::IWireManager),static_cast<System::MetaInfo::IWireManager_Safe*>(m_pManager));
		if (pSE2)
			return pSE2;
	}

	return pSE;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireProxy::ReleaseMarshalData_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t*, Remoting::IMarshal::Flags_t)
{
	// Marshal our own manager out...
	System::MetaInfo::IException_Safe* pSE = pObjectManager->ReleaseMarshalData_Safe(pStream,&OMEGA_UUIDOF(System::MetaInfo::IWireManager),static_cast<System::MetaInfo::IWireManager_Safe*>(m_pManager));
	if (pSE)
		return pSE;

	byte_t v;
	pSE = pStream->ReadByte_Safe(&v);
	if (pSE)
		return pSE;

	uint32_t key;
	return pStream->ReadUInt32_Safe(&key);
}

namespace OOCore
{
	namespace SEH
	{

#if defined(ACE_WIN32) && !defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS) && defined (__GNUC__)

		struct ExceptInfo
		{
			ExceptInfo* prev;
			int (WINAPI *handler)(PEXCEPTION_RECORD,ExceptInfo*,PCONTEXT,PEXCEPTION_RECORD);
			jmp_buf* pjb;
		};

		int WINAPI ExceptHandler(PEXCEPTION_RECORD record, ExceptInfo* frame, PCONTEXT, PEXCEPTION_RECORD)
		{
			longjmp(*frame->pjb,record->ExceptionCode);
			return 0;
		}
#endif

		void DoInvoke2(System::MetaInfo::IWireStub_Safe* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, IException*& pE);
		int DoInvoke(System::MetaInfo::IWireStub_Safe* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, IException*& pE);
	}
}

void OOCore::SEH::DoInvoke2(System::MetaInfo::IWireStub_Safe* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, IException*& pE)
{
	try
	{
		System::MetaInfo::IException_Safe* pSE = pStub->Invoke_Safe(
			System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pParamsIn),
			System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pParamsOut));
		
		if (pSE)
			throw_correct_exception(pSE);
	}
	catch (IException* pE2)
	{
		pE = pE2;
	}
}

int OOCore::SEH::DoInvoke(System::MetaInfo::IWireStub_Safe* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, IException*& pE)
{
	int err = 0;

#if defined(ACE_WIN32)
	#if defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
		LPEXCEPTION_POINTERS ex = 0;
		ACE_SEH_TRY
		{
			DoInvoke2(pStub,pParamsIn,pParamsOut,pE);
		}
		ACE_SEH_EXCEPT((ex = GetExceptionInformation(),EXCEPTION_EXECUTE_HANDLER))
		{
			err = ex->ExceptionRecord->ExceptionCode;
		}
	#elif defined (__GNUC__) && !defined(ACE_WIN64)

		// This is hideous scary stuff... but it taps into the Win32 SEH stuff
		jmp_buf jmpb;
		ExceptInfo xc;
		xc.handler = ExceptHandler;
		xc.pjb = &jmpb;

		// Install SEH handler
		__asm__ __volatile__ ("movl %%fs:0, %0" : "=r" (xc.prev));
		__asm__ __volatile__ ("movl %0, %%fs:0" : : "r" (&xc));

		err = setjmp(jmpb);
		if (err == 0)
		{
			try
			{
				DoInvoke2(pStub,pParamsIn,pParamsOut,pE);
			}
			catch (...)
			{
				// Remove SEH handler
				__asm__ __volatile__ ( "movl %0, %%fs:0" : : "r" (xc.prev));
				throw;
			}
		}

		// Remove SEH handler
		__asm__ __volatile__ ( "movl %0, %%fs:0" : : "r" (xc.prev));

	#else
		void* TODO; // You have no protection around Invoke for your compiler...
		DoInvoke2(pStub,pParamsIn,pParamsOut,pE);
	#endif
#else
	void* TODO; // Some kind of signal handler here please?
	DoInvoke2(pStub,pParamsIn,pParamsOut,pE);
#endif

	return err;
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

void OOCore::StdObjectManager::Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
{
	if (!pParamsIn)
		OOCORE_THROW_ERRNO(EINVAL);

	// Assume we succeed...
	pParamsOut->WriteBoolean(true);

	// Read the stub id
	uint32_t stub_id = pParamsIn->ReadUInt32();

	if (stub_id == 0)
	{
		// It's a call from CreateRemoteInstance
				
		// Read the oid and iid
		guid_t oid = read_guid(pParamsIn);
		guid_t iid = read_guid(pParamsIn);

		// Read the outer object
		IObject* pOuter = 0;
		UnmarshalInterface(pParamsIn,OMEGA_UUIDOF(IObject),pOuter);
		ObjectPtr<IObject> ptrOuter;
		ptrOuter.Attach(pOuter);

		// Create the required object
		ObjectPtr<IObject> ptrObject;
		ptrObject.Attach(CreateInstance(oid,Activation::Any | Activation::RemoteInvocation,ptrOuter,iid));

		// Write it out and return
		MarshalInterface(pParamsOut,iid,ptrObject);
		return;
	}
	
	// It's a method call on a stub...
	System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IWireStub_Safe> ptrStub;

	// Look up the stub
	try
	{
		OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,WireStub*>::iterator i=m_mapStubIds.find(stub_id);
		if (i==m_mapStubIds.end())
			OMEGA_THROW(L"Bad stub id");

		ptrStub.attach(i->second->LookupStub(pParamsIn));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	// Ask the stub to make the call
	IException* pE = 0;
	int err = SEH::DoInvoke(ptrStub,pParamsIn,pParamsOut,pE);
	if (pE)
		throw pE;
	else if (err != 0)
		OOCORE_THROW_ERRNO(err);
}

void OOCore::StdObjectManager::Disconnect()
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	// clear the stub map
	m_mapStubIds.clear();
	m_mapStubObjs.clear();
	m_mapProxyIds.clear();
}

void OOCore::StdObjectManager::MarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject)
{
	System::MetaInfo::IException_Safe* pSE = MarshalInterface_Safe(
		System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pStream),
		&iid,
		System::MetaInfo::marshal_info<IObject*>::safe_type::coerce(pObject));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);
}

void OOCore::StdObjectManager::UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject)
{
	System::MetaInfo::IException_Safe* pSE = UnmarshalInterface_Safe(
		System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pStream),
		&iid,
		System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(pObject,iid));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);
}

void OOCore::StdObjectManager::ReleaseMarshalData(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject)
{
	System::MetaInfo::IException_Safe* pSE = ReleaseMarshalData_Safe(
		System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pStream),
		&iid,
		System::MetaInfo::marshal_info<IObject*>::safe_type::coerce(pObject));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);
}

Serialize::IFormattedStream* OOCore::StdObjectManager::CreateOutputStream(IObject* pOuter)
{
	if (!m_ptrChannel)
		OOCORE_THROW_ERRNO(EINVAL);

	return m_ptrChannel->CreateOutputStream(pOuter);
}

IException* OOCore::StdObjectManager::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pSend, Serialize::IFormattedStream*& pRecv, uint16_t timeout)
{
	if (timeout == 0)
		timeout = 15000;

    IException* pE = m_ptrChannel->SendAndReceive(attribs,pSend,pRecv,timeout);
	if (pE)
		return pE;

	ObjectPtr<Serialize::IFormattedStream> ptrRecv;
	ptrRecv.Attach(pRecv);
	pRecv = 0;

	if (!(attribs & Remoting::asynchronous))
	{
		try
		{
			if (!ptrRecv)
				OMEGA_THROW(L"No response received");

			// Read exception status
			if (!ptrRecv->ReadBoolean())
			{
				// Unmarshal the exception
				IObject* pE = 0;
				UnmarshalInterface(ptrRecv,OMEGA_UUIDOF(IException),pE);
				return static_cast<IException*>(pE);
			}
		}
		catch (IException* pE)
		{
			return pE;
		}
	}

	pRecv = ptrRecv.Detach();
	return 0;
}

void OOCore::StdObjectManager::RemoveProxy(uint32_t proxy_id)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapProxyIds.erase(proxy_id);
}

void OMEGA_CALL OOCore::StdObjectManager::AddRef_Safe()
{
	this->Internal_AddRef();
}

void OMEGA_CALL OOCore::StdObjectManager::Release_Safe()
{
	this->Internal_Release();
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::QueryInterface_Safe(const guid_t* piid, System::MetaInfo::IObject_Safe** ppS)
{
	*ppS = 0;
	if (*piid == OMEGA_UUIDOF(IObject) ||
		*piid == OMEGA_UUIDOF(System::MetaInfo::IWireManager))
	{
		*ppS = static_cast<System::MetaInfo::IWireManager_Safe*>(this);
		(*ppS)->AddRef_Safe();
	}
	else if (*piid == OMEGA_UUIDOF(Remoting::IMarshal))
	{
		*ppS = static_cast<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class*>(this);
		(*ppS)->AddRef_Safe();
	}

	return 0;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::MarshalInterface_Safe(System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, System::MetaInfo::IObject_Safe* pObject)
{
	try
	{
		// See if object is NULL
		if (!pObject)
			return pStream->WriteByte_Safe(0);
	
		// See if we have a stub already...
		System::MetaInfo::IObject_Safe* pObjS = 0;
		System::MetaInfo::IException_Safe* pSE = pObject->QueryInterface_Safe(&OMEGA_UUIDOF(IObject),&pObjS);
		if (pSE)
			return pSE;
		System::MetaInfo::auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);

		System::MetaInfo::auto_iface_safe_ptr<WireStub> ptrStub;
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<IObject_Safe*,WireStub*>::const_iterator i=m_mapStubObjs.find(ptrObjS);
			if (i != m_mapStubObjs.end())
			{
				ptrStub = i->second;
			}
		}
		
		if (!ptrStub)
		{
			// See if pObject does custom marshalling...
			System::MetaInfo::IObject_Safe* pMarshal = 0;
			pSE = pObject->QueryInterface_Safe(&OMEGA_UUIDOF(Remoting::IMarshal),&pMarshal);
			if (pSE)
				return pSE;
			
			if (pMarshal)
			{
				System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class> ptrMarshal(static_cast<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class*>(pMarshal));
				guid_t oid;
				pSE = ptrMarshal->GetUnmarshalFactoryOID_Safe(&oid,piid,0);
				if (pSE)
					return pSE;

				if (oid != guid_t::Null())
				{
					// Write the marshalling oid
					pSE = pStream->WriteByte_Safe(2);
					if (pSE)
						return pSE;

					pSE = System::MetaInfo::wire_write(pStream,oid);
					if (pSE)
						return pSE;

					// Let the custom handle marshalling...
					return ptrMarshal->MarshalInterface_Safe(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(this),pStream,piid,Remoting::IMarshal::inter_process);
				}
			}

			// Create a new stub and stub id
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			uint32_t stub_id = m_uNextStubId++;
			while (stub_id==0 || m_mapStubIds.find(stub_id)!=m_mapStubIds.end())
			{
				stub_id = m_uNextStubId++;
			}

			OMEGA_NEW(ptrStub,WireStub(ptrObjS,stub_id,this));
			
			// Add to the map...
			std::pair<std::map<System::MetaInfo::IObject_Safe*,WireStub*>::iterator,bool> p=m_mapStubObjs.insert(std::map<System::MetaInfo::IObject_Safe*,WireStub*>::value_type(ptrObjS,ptrStub));
			if (!p.second)
				ptrStub = p.first->second;
			else
			{
				m_mapStubIds.insert(std::map<uint32_t,WireStub*>::value_type(stub_id,ptrStub));
				ptrStub->AddRef_Safe();
			}
		}	

		// Write out the data
		pSE = pStream->WriteByte_Safe(1);
		if (pSE)
			return pSE;

		return ptrStub->MarshalInterface(pStream,*piid);
	}
	catch (std::exception& e)
	{
		return System::MetaInfo::return_safe_exception(IException::Create(string_t(e.what(),false),OMEGA_SOURCE_INFO));
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::UnmarshalInterface_Safe(System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, System::MetaInfo::IObject_Safe** ppObjS)
{
	try
	{
		byte_t flag;
		System::MetaInfo::IException_Safe* pSE = pStream->ReadByte_Safe(&flag);
		if (pSE)
			return pSE;

		if (flag == 0)
		{
			*ppObjS = 0;
			return 0;
		}
		else if (flag == 1)
		{
			uint32_t proxy_id;
			pSE = pStream->ReadUInt32_Safe(&proxy_id);
			if (pSE)
				return pSE;

			// See if we have a proxy already...
			System::MetaInfo::auto_iface_safe_ptr<WireProxy> ptrProxy;
			{
				OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				std::map<uint32_t,WireProxy*>::iterator i=m_mapProxyIds.find(proxy_id);
				if (i != m_mapProxyIds.end())
					ptrProxy = i->second;
			}

			if (!ptrProxy)
			{
				OMEGA_NEW(ptrProxy,WireProxy(proxy_id,this));

				OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				std::pair<std::map<uint32_t,WireProxy*>::iterator,bool> p = m_mapProxyIds.insert(std::map<uint32_t,WireProxy*>::value_type(proxy_id,ptrProxy));
				if (!p.second)
					ptrProxy = p.first->second;
			}

			*ppObjS = ptrProxy->UnmarshalInterface(pStream,*piid);
		}
		else if (flag == 2)
		{
			guid_t oid;
			pSE = System::MetaInfo::wire_read(pStream,oid);
			if (pSE)
				return pSE;

			// Create an instance of Oid
			ObjectPtr<Remoting::IMarshalFactory> ptrMarshalFactory(oid,Activation::InProcess);
			if (!ptrMarshalFactory)
				throw INoInterfaceException::Create(OMEGA_UUIDOF(Remoting::IMarshalFactory),OMEGA_SOURCE_INFO);

			ptrMarshalFactory->UnmarshalInterface(
				this,
				System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pStream),
				*piid,Remoting::IMarshal::inter_process,
				System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(ppObjS));
		}
		else
			OOCORE_THROW_ERRNO(EINVAL);

		return 0;
	}
	catch (std::exception& e)
	{
		return System::MetaInfo::return_safe_exception(IException::Create(string_t(e.what(),false),OMEGA_SOURCE_INFO));
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::ReleaseMarshalData_Safe(System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, System::MetaInfo::IObject_Safe* pObject)
{
	try
	{
		byte_t flag;
		System::MetaInfo::IException_Safe* pSE = pStream->ReadByte_Safe(&flag);
		if (pSE)
			return pSE;

		if (flag == 0)
		{
			return 0;
		}
		else if (flag == 1)
		{
			System::MetaInfo::IObject_Safe* pObjS = 0;
			System::MetaInfo::IException_Safe* pSE = pObject->QueryInterface_Safe(&OMEGA_UUIDOF(IObject),&pObjS);
			if (pSE)
				return pSE;
			System::MetaInfo::auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);

			System::MetaInfo::auto_iface_safe_ptr<WireStub> ptrStub;
			try
			{
				OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				std::map<System::MetaInfo::IObject_Safe*,WireStub*>::const_iterator i=m_mapStubObjs.find(ptrObjS);
				if (i != m_mapStubObjs.end())
				{
					ptrStub = i->second;
				}
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(string_t(e.what(),false));
			}

			// If there is no stub... what are we unmarshalling?
			if (ptrStub)
				OOCORE_THROW_ERRNO(EINVAL);
			
			// Read the data
			return ptrStub->ReleaseMarshalData(pStream,*piid);
		}
		else if (flag == 2)
		{
			// See if pObject does custom marshalling...
			System::MetaInfo::IObject_Safe* pMarshal = 0;
			pSE = pObject->QueryInterface_Safe(&OMEGA_UUIDOF(Remoting::IMarshal),&pMarshal);
			if (pSE)
				return pSE;
			
			if (!pMarshal)
				OOCORE_THROW_ERRNO(EINVAL);

			System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class> ptrMarshal(static_cast<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class*>(pMarshal));

			// Skip the guid...
			guid_t oid;
			pSE = System::MetaInfo::wire_read(pStream,oid);
			if (pSE)
				return pSE;
		
			return ptrMarshal->ReleaseMarshalData_Safe(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(this),pStream,piid,Remoting::IMarshal::inter_process);
		}
		else
		{
			OOCORE_THROW_ERRNO(EINVAL);
		}
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
	catch (std::exception& e)
	{
		return System::MetaInfo::return_safe_exception(IException::Create(string_t(e.what(),false),OMEGA_SOURCE_INFO));
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::CreateOutputStream_Safe(System::MetaInfo::IFormattedStream_Safe** ppRet, System::MetaInfo::IObject_Safe* pOuter)
{
	try
	{
		static_cast<Serialize::IFormattedStream*&>(System::MetaInfo::marshal_info<Serialize::IFormattedStream*&>::safe_type::coerce(ppRet)) = CreateOutputStream(System::MetaInfo::marshal_info<IObject*>::safe_type::coerce(pOuter));
		return 0;
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::SendAndReceive_Safe(System::MetaInfo::IException_Safe** ppRet, Remoting::MethodAttributes_t attribs, System::MetaInfo::IFormattedStream_Safe* pSend, System::MetaInfo::IFormattedStream_Safe** ppRecv, uint16_t timeout)
{
	try
	{
		static_cast<IException*&>(System::MetaInfo::marshal_info<IException*&>::safe_type::coerce(ppRet)) = SendAndReceive(attribs,System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pSend),System::MetaInfo::marshal_info<Serialize::IFormattedStream*&>::safe_type::coerce(ppRecv),timeout);
		return 0;
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

void OOCore::StdObjectManager::CreateRemoteInstance(const guid_t& oid, const guid_t& iid, IObject* pOuter, IObject*& pObject)
{
	if (pObject)
		pObject->Release();

	ObjectPtr<Serialize::IFormattedStream> ptrParamsOut; 
	ptrParamsOut.Attach(CreateOutputStream());

	ptrParamsOut->WriteUInt32(0);
	write_guid(ptrParamsOut,oid);
	write_guid(ptrParamsOut,iid);
	MarshalInterface(ptrParamsOut,OMEGA_UUIDOF(IObject),pOuter);

	Serialize::IFormattedStream* pParamsIn = 0;
	IException* pERet;
		
	try
	{
		pERet = SendAndReceive(0,ptrParamsOut,pParamsIn,0);
	}
	catch (IException* pE)
	{
		ReleaseMarshalData(ptrParamsOut,OMEGA_UUIDOF(IObject),pOuter);
		throw pE;
	}

	if (pERet)
		throw pERet;
		
	ObjectPtr<Serialize::IFormattedStream> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	UnmarshalInterface(ptrParamsIn,iid,pObject);
}

Omega::guid_t OOCore::StdObjectManager::GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	return OID_StdObjectManagerMarshalFactory;
}

void OOCore::StdObjectManager::MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	pObjectManager->MarshalInterface(pStream,OMEGA_UUIDOF(Remoting::IChannel),m_ptrChannel);
}

void OOCore::StdObjectManager::ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::IMarshal::Flags_t)
{
	pObjectManager->ReleaseMarshalData(pStream,OMEGA_UUIDOF(Remoting::IChannel),m_ptrChannel);
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::GetUnmarshalFactoryOID_Safe(guid_t* pRet, const guid_t*, Remoting::IMarshal::Flags_t)
{
	*pRet = OID_StdObjectManagerMarshalFactory;
	return 0;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::MarshalInterface_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, Remoting::IMarshal::Flags_t flags)
{
	try
	{
		MarshalInterface(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(pObjectManager),System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pStream),*piid,flags);
		return 0;
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::ReleaseMarshalData_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, Remoting::IMarshal::Flags_t flags)
{
	try
	{
		ReleaseMarshalData(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(pObjectManager),System::MetaInfo::marshal_info<Serialize::IFormattedStream*>::safe_type::coerce(pStream),*piid,flags);
		return 0;
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

OMEGA_DEFINE_OID(OOCore,OID_WireProxyMarshalFactory,"{69099DD8-A628-458a-861F-009E016DB81B}");
OMEGA_DEFINE_OID(OOCore,OID_StdObjectManagerMarshalFactory,"{3AC2D04F-A8C5-4214-AFE4-A64DB8DC992C}");
OMEGA_DEFINE_OID(Remoting,OID_StdObjectManager,"{63EB243E-6AE3-43bd-B073-764E096775F8}");
OMEGA_DEFINE_OID(Remoting,OID_InterProcess,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");

#include "OOCore_precomp.h"

#include "./StdObjectManager.h"

#if defined(ACE_WIN32) && !defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS) && defined (__GNUC__)
#include <setjmp.h>
#endif

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	struct wire_holder
	{
		static System::MetaInfo::IWireStub* CreateWireStub(const guid_t& iid, System::MetaInfo::IWireManager* pManager, IObject* pObject, uint32_t id);
		static IObject* CreateWireProxy(const guid_t& iid, IObject* pOuter, System::MetaInfo::IWireManager* pManager, uint32_t id);

		typedef System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnCreateWireProxy)(
			System::MetaInfo::interface_info<IObject*>::safe_class pOuter,
			System::MetaInfo::interface_info<System::MetaInfo::IWireManager*>::safe_class pManager,
			System::MetaInfo::interface_info<uint32_t>::safe_class id,
			System::MetaInfo::interface_info<IObject*>::safe_class* pObj);

		typedef System::MetaInfo::IException_Safe* (OMEGA_CALL *pfnCreateWireStub)(
			System::MetaInfo::interface_info<System::MetaInfo::IWireStub*>::safe_class* retval,
			System::MetaInfo::interface_info<System::MetaInfo::IWireManager*>::safe_class pManager,
			System::MetaInfo::interface_info<IObject*>::safe_class pObject,
			System::MetaInfo::interface_info<uint32_t>::safe_class id);

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

	class UnboundProxy :
		public ObjectBase,
		public System::MetaInfo::IWireProxy
	{
	public:
		UnboundProxy() {}

		virtual ~UnboundProxy()
		{
		}

		void init(System::MetaInfo::IWireManager* pManager, const guid_t& oid, const guid_t& iid)
		{
			m_oid = oid;
			m_iid = iid;
			this->m_ptrProxy.Attach(wire_holder::CreateWireProxy(iid,this,pManager,0));
		}

	BEGIN_INTERFACE_MAP(UnboundProxy)
		INTERFACE_ENTRY(System::MetaInfo::IWireProxy)
		INTERFACE_ENTRY_NOINTERFACE(Remoting::IMarshal)
		INTERFACE_ENTRY_FUNCTION_BLIND(QI)
	END_INTERFACE_MAP()

	private:
		UnboundProxy(const UnboundProxy&) : ObjectBase(), System::MetaInfo::IWireProxy() {};
		UnboundProxy& operator = (const UnboundProxy&) { return *this; };

		guid_t                                    m_oid;
		guid_t                                    m_iid;
		ObjectPtr<IObject>                        m_ptrProxy;

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
		virtual void WriteKey(Serialize::IFormattedStream* pStream)
		{
			System::MetaInfo::wire_write(pStream,m_oid);
			System::MetaInfo::wire_write(pStream,m_iid);
		}
	};

	class StdProxy :
		public ObjectBase,
		public System::MetaInfo::IWireProxy
	{
	public:
		StdProxy() {}
		virtual ~StdProxy() {}

		void init(System::MetaInfo::IWireManager* pManager, const guid_t& iid, uint32_t uId);

	BEGIN_INTERFACE_MAP(StdProxy)
		INTERFACE_ENTRY(System::MetaInfo::IWireProxy)
		INTERFACE_ENTRY_NOINTERFACE(Remoting::IMarshal)
		INTERFACE_ENTRY_FUNCTION_BLIND(QI)
	END_INTERFACE_MAP()

	private:
		StdProxy(const StdProxy&) : ObjectBase(), System::MetaInfo::IWireProxy() {};
		StdProxy& operator = (const StdProxy&) { return *this; };

		ACE_RW_Thread_Mutex                          m_lock;
		std::map<const guid_t,ObjectPtr<IObject> >   m_iid_map;

        static IObject* QI(const guid_t& iid, void* pThis, void*)
        {
			return static_cast<RootClass*>(pThis)->QI2(iid);
        }

		IObject* QI2(const guid_t& iid);

	// IWireProxy members
	public:
		virtual void WriteKey(Serialize::IFormattedStream* /*pStream*/)
		{
			OOCORE_THROW_ERRNO(EINVAL);
		}
	};
}

void OOCore::StdProxy::init(System::MetaInfo::IWireManager* pManager, const guid_t& iid, uint32_t uId)
{
	try
	{
		ObjectPtr<IObject> ptrObj;
		ptrObj.Attach(wire_holder::CreateWireProxy(iid,this,pManager,uId));
		m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(iid,ptrObj));
		ptrObj.Detach();
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

IObject* OOCore::StdProxy::QI2(const guid_t& iid)
{
	if (iid==OMEGA_UUIDOF(System::MetaInfo::SafeProxy))
		return 0;

	try
	{
		ObjectPtr<IObject> ptrNew;
		ObjectPtr<IObject> ptrQI;

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

		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::pair<std::map<const guid_t,ObjectPtr<IObject> >::iterator,bool> p = m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(iid,ptrNew));
		if (!p.second)
			ptrNew.Attach(p.first->second);

		if (ptrQI)
			return ptrQI.AddRefReturn();

		if (ptrNew)
			return ptrNew->QueryInterface(iid);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
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

		void DoInvoke2(uint32_t method_id, System::MetaInfo::IWireStub* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, IException*& pE);
		int DoInvoke(uint32_t method_id, System::MetaInfo::IWireStub* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, IException*& pE);
	}
}

void OOCore::SEH::DoInvoke2(uint32_t method_id, System::MetaInfo::IWireStub* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, IException*& pE)
{
	try
	{
		pStub->Invoke(method_id,pParamsIn,pParamsOut);
	}
	catch (IException* pE2)
	{
		pE = pE2;
	}
}

int OOCore::SEH::DoInvoke(uint32_t method_id, System::MetaInfo::IWireStub* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, IException*& pE)
{
	int err = 0;

#if defined(ACE_WIN32)
	#if defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
		LPEXCEPTION_POINTERS ex = 0;
		ACE_SEH_TRY
		{
			DoInvoke2(method_id,pStub,pParamsIn,pParamsOut,pE);
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
				DoInvoke2(method_id,pStub,pParamsIn,pParamsOut,pE);
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
		DoInvoke2(method_id,pStub,pParamsIn,pParamsOut,pE);

	#endif
#else
	void* TODO; // Some kind of signal handler here please?
	DoInvoke2(method_id,pStub,pParamsIn,pParamsOut,pE);
#endif

	return err;
}

void OOCore::StdObjectManager::Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
{
	if (!pParamsIn)
		OOCORE_THROW_ERRNO(EINVAL);

	ObjectPtr<System::MetaInfo::IWireStub> ptrStub;
	uint32_t method_id;

	// Read the stub id and method id
	uint32_t stub_id = pParamsIn->ReadUInt32();

	if (stub_id == 0)
	{
		// It's a static interface call...
		// N.B. This will always use standard marshalling

		// Read the oid and iid
		guid_t oid;
		System::MetaInfo::wire_read(pParamsIn,oid);
		guid_t iid;
		System::MetaInfo::wire_read(pParamsIn,iid);

		method_id = pParamsIn->ReadUInt32();

		// IObject interface calls are not allowed on static interfaces!
		if (method_id < 3)
			OOCORE_THROW_ERRNO(EINVAL);

		// Create the required object
		ObjectPtr<IObject> ptrObject;
		ptrObject.Attach(CreateInstance(oid,Activation::Any,0,iid));

		// And create a stub
		ptrStub.Attach(wire_holder::CreateWireStub(iid,this,ptrObject,0));
		if (!ptrStub)
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
	}
	else
	{
		// It's a method call on a stub...

		// Look up the stub
		try
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<uint32_t,ObjectPtr<System::MetaInfo::IWireStub> >::const_iterator i=m_mapStubIds.find(stub_id);
			if (i==m_mapStubIds.end())
				OMEGA_THROW(L"Bad stub id");
			ptrStub = i->second;
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(string_t(e.what(),false));
		}

		// Read the method id
		method_id = pParamsIn->ReadUInt32();
	}

	// Assume we succeed...
	pParamsOut->WriteBoolean(true);

	// Ask the stub to make the call
	IException* pE = 0;
	int err = SEH::DoInvoke(method_id,ptrStub,pParamsIn,pParamsOut,pE);
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
	// See if object is NULL
	if (!pObject)
	{
		pStream->WriteByte(0);
		return;
	}

	// See if we have a stub already...
	ObjectPtr<IObject> ptrObject(pObject);
	uint32_t stub_id = 0;
	try
	{
		OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<IObject*,uint32_t>::const_iterator i=m_mapStubObjs.find(ptrObject);
		if (i != m_mapStubObjs.end())
		{
			stub_id = i->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	if (!stub_id)
	{
		// See if pObject does custom marshalling...
		ObjectPtr<Remoting::IMarshal> ptrMarshal(pObject);
		if (ptrMarshal)
		{
			guid_t oid = ptrMarshal->GetUnmarshalOID(iid,0);
			if (oid != guid_t::Null())
			{
				// Write the marshalling oid
				pStream->WriteByte(2);
				System::MetaInfo::wire_write(pStream,oid);

				// Let the custom handle marshalling...
				ptrMarshal->Marshal(pStream,iid,0);

				// Done
				return;
			}
		}

		// Generate a new key and stub pair
		try
		{
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			stub_id = m_uNextStubId++;
			while (stub_id==0 || m_mapStubIds.find(stub_id)!=m_mapStubIds.end())
			{
				stub_id = m_uNextStubId++;
			}

			/*char szBuf[128];
			sprintf(szBuf,"Stub %u for %ls\n",stub_id,System::MetaInfo::lookup_iid(iid).c_str());
			OutputDebugString(szBuf);*/

			// Create a stub
			ObjectPtr<System::MetaInfo::IWireStub> ptrStub;
			ptrStub.Attach(wire_holder::CreateWireStub(iid,this,pObject,stub_id));
			if (!ptrStub)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			// Add to the map...
			std::pair<std::map<IObject*,uint32_t>::iterator,bool> p=m_mapStubObjs.insert(std::map<IObject*,uint32_t>::value_type(ptrObject,stub_id));
			if (!p.second)
				stub_id = p.first->second;
			else
				m_mapStubIds.insert(std::map<uint32_t,ObjectPtr<System::MetaInfo::IWireStub> >::value_type(stub_id,ptrStub));
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(string_t(e.what(),false));
		}
	}

	// Write out the data
	pStream->WriteByte(1);
	System::MetaInfo::wire_write(pStream,iid);
	pStream->WriteUInt32(stub_id);
}

void OOCore::StdObjectManager::UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject*& pObject)
{
	byte_t flag = pStream->ReadByte();
	if (flag == 0)
	{
		pObject = 0;
	}
	else if (flag == 1)
	{
		guid_t wire_iid;
		System::MetaInfo::wire_read(pStream,wire_iid);
		uint32_t proxy_id = pStream->ReadUInt32();

		// See if we have a proxy already...
		ObjectPtr<System::MetaInfo::IWireProxy> ptrProxy;
		try
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<uint32_t,ObjectPtr<System::MetaInfo::IWireProxy> >::iterator i=m_mapProxyIds.find(proxy_id);
			if (i != m_mapProxyIds.end())
			{
				ptrProxy = i->second;
			}
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(string_t(e.what(),false));
		}

		if (!ptrProxy)
		{
			/*char szBuf[128];
			sprintf(szBuf,"Proxy %u for %ls\n",proxy_id,System::MetaInfo::lookup_iid(wire_iid).c_str());
			OutputDebugString(szBuf);*/

			ObjectPtr<ObjectImpl<OOCore::StdProxy> > ptrStdProxy = ObjectImpl<OOCore::StdProxy>::CreateInstancePtr();
			ptrStdProxy->init(this,wire_iid,proxy_id);
			ptrProxy.Attach(static_cast<System::MetaInfo::IWireProxy*>(ptrStdProxy->QueryInterface(OMEGA_UUIDOF(System::MetaInfo::IWireProxy))));

			try
			{
				OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				std::pair<std::map<uint32_t,ObjectPtr<System::MetaInfo::IWireProxy> >::iterator,bool> p = m_mapProxyIds.insert(std::map<uint32_t,ObjectPtr<System::MetaInfo::IWireProxy> >::value_type(proxy_id,ptrProxy));
				if (!p.second)
					ptrProxy = p.first->second;
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(string_t(e.what(),false));
			}
		}

		if (iid == guid_t::Null())
			pObject = ptrProxy->QueryInterface(wire_iid);
		else
			pObject = ptrProxy->QueryInterface(iid);
	}
	else if (flag == 2)
	{
     	guid_t oid;
		System::MetaInfo::wire_read(pStream,oid);

		// Create an instance of Oid
		ObjectPtr<Remoting::IMarshal> ptrMarshal(oid,Activation::InProcess);
		if (!ptrMarshal)
			throw INoInterfaceException::Create(OMEGA_UUIDOF(Remoting::IMarshal),OMEGA_SOURCE_INFO);

		pObject = ptrMarshal->Unmarshal(pStream,iid);
	}
	else
		OOCORE_THROW_ERRNO(EINVAL);
}

void OOCore::StdObjectManager::ReleaseStub(uint32_t uId)
{
	try
	{
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,ObjectPtr<System::MetaInfo::IWireStub> >::iterator i=m_mapStubIds.find(uId);
		if (i==m_mapStubIds.end())
			OOCORE_THROW_ERRNO(EINVAL);

		ObjectPtr<IObject> ptrObj;
		ptrObj.Attach(i->second->GetStubObject());
		m_mapStubObjs.erase(ptrObj);

		m_mapStubIds.erase(i);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

void OOCore::StdObjectManager::CreateOutputStream(IObject* pOuter, Omega::Serialize::IFormattedStream*& pStream)
{
	if (!m_ptrChannel)
		OOCORE_THROW_ERRNO(EINVAL);

	m_ptrChannel->CreateOutputStream(pOuter,pStream);
}

Serialize::IFormattedStream* OOCore::StdObjectManager::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pStream, uint16_t timeout)
{
    ObjectPtr<Serialize::IFormattedStream> ptrResponse;
	ptrResponse.Attach(m_ptrChannel->SendAndReceive(attribs,pStream,timeout));

	if (!(attribs & Remoting::asynchronous))
	{
		if (!ptrResponse)
			OMEGA_THROW(L"No response received");

		// Read exception status
		if (!ptrResponse->ReadBoolean())
		{
			// Unmarshal the exception
			IException* pE;
			System::MetaInfo::wire_read(this,ptrResponse,pE);
			throw pE;
		}
	}

	return ptrResponse.AddRefReturn();
}

void OOCore::StdObjectManager::CreateUnboundProxy(const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	if (pObject)
		pObject->Release();

	ObjectPtr<ObjectImpl<OOCore::UnboundProxy> > ptrProxy = ObjectImpl<OOCore::UnboundProxy>::CreateInstancePtr();
	ptrProxy->init(this,oid,iid);

	pObject = ptrProxy->QueryInterface(iid);
}

OMEGA_DEFINE_OID(Omega::Remoting,OID_StdObjectManager,"{63EB243E-6AE3-43bd-B073-764E096775F8}");
OMEGA_DEFINE_OID(Omega::Remoting,OID_InterProcess,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");

System::MetaInfo::IWireStub* OOCore::wire_holder::CreateWireStub(const guid_t& iid, System::MetaInfo::IWireManager* pManager, IObject* pObject, uint32_t id)
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

	System::MetaInfo::IWireStub* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnStub(
		System::MetaInfo::interface_info<System::MetaInfo::IWireStub*&>::proxy_functor(pRet),
		System::MetaInfo::interface_info<System::MetaInfo::IWireManager*>::proxy_functor(pManager),
		System::MetaInfo::interface_info<IObject*>::proxy_functor(pObject),
		System::MetaInfo::interface_info<uint32_t>::proxy_functor(id));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

IObject* OOCore::wire_holder::CreateWireProxy(const guid_t& iid, IObject* pOuter, System::MetaInfo::IWireManager* pManager, uint32_t id)
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

	IObject* pRet = 0;
	System::MetaInfo::IException_Safe* pSE = p.pfnProxy(
		System::MetaInfo::interface_info<IObject*>::proxy_functor(pOuter),
		System::MetaInfo::interface_info<System::MetaInfo::IWireManager*>::proxy_functor(pManager),
		System::MetaInfo::interface_info<uint32_t>::proxy_functor(id),
		System::MetaInfo::interface_info<IObject*&>::proxy_functor(pRet,OMEGA_UUIDOF(IObject),pOuter));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_RegisterWireFactories,3,((in),const guid_t&,iid,(in),void*,pfnProxy,(in),void*,pfnStub))
{
	OOCore::wire_holder::pfns funcs;
	funcs.pfnProxy = (OOCore::wire_holder::pfnCreateWireProxy)(pfnProxy);
	funcs.pfnStub= (OOCore::wire_holder::pfnCreateWireStub)(pfnStub);

	try
	{
		OOCore::wire_holder::instance().map.insert(std::map<guid_t,OOCore::wire_holder::pfns>::value_type(iid,funcs));
	}
	catch (...)
	{}
}

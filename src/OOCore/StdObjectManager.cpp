#include "OOCore_precomp.h"

#include "./StdObjectManager.h"

#if defined(ACE_WIN32) && !defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS) && defined (__GNUC__)
#include <setjmp.h>
#endif

using namespace Omega;
using namespace OTL;

namespace OOCore
{
	class UnboundProxy :
		public ObjectBase,
		public System::MetaInfo::IWireProxy
	{
	public:
		void init(System::MetaInfo::IWireManager* pManager, const guid_t& oid, const guid_t& iid)
		{
			const System::MetaInfo::qi_rtti* pRtti = System::MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateWireProxy)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			this->m_ptrProxy.Attach(pRtti->pfnCreateWireProxy(this,pManager));
			this->m_ptrManager = pManager;
			m_oid = oid;
			m_iid = iid;
		}

	BEGIN_INTERFACE_MAP(UnboundProxy)
		INTERFACE_ENTRY(System::MetaInfo::IWireProxy)
		INTERFACE_ENTRY_NOINTERFACE(Remoting::IMarshal)
		INTERFACE_ENTRY_FUNCTION_BLIND(QI)
	END_INTERFACE_MAP()

	private:
		guid_t                                    m_oid;
		guid_t                                    m_iid;
		ObjectPtr<System::MetaInfo::IWireManager> m_ptrManager;
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
		void WriteKey(Serialize::IFormattedStream* pStream)
		{
			pStream->WriteUInt32(0);
			System::MetaInfo::wire_write(m_ptrManager,pStream,m_oid);
			System::MetaInfo::wire_write(m_ptrManager,pStream,m_iid);
		}
	};

	class StdProxy :
		public ObjectBase,
		public System::MetaInfo::IWireProxy
	{
	public:
		StdProxy() {}

		void init(System::MetaInfo::IWireManager* pManager, const guid_t& iid, uint32_t uId)
		{
			const System::MetaInfo::qi_rtti* pRtti = System::MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateWireProxy)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			try
			{
				ObjectPtr<IObject> ptrObj;
				ptrObj.Attach(pRtti->pfnCreateWireProxy(this,pManager));
				m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(iid,ptrObj));
				ptrObj.Detach();
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(string_t(e.what(),false));
			}

			m_ptrManager = pManager;
			m_uId = uId;
		}

	BEGIN_INTERFACE_MAP(StdProxy)
		INTERFACE_ENTRY(System::MetaInfo::IWireProxy)
		INTERFACE_ENTRY_NOINTERFACE(Remoting::IMarshal)
		INTERFACE_ENTRY_FUNCTION_BLIND(QI)
	END_INTERFACE_MAP()

	private:
		StdProxy(const StdProxy&) :
            ObjectBase(), System::MetaInfo::IWireProxy()
        {};
		StdProxy& operator = (const StdProxy&) { return *this; };

		ACE_RW_Thread_Mutex                          m_lock;
		std::map<const guid_t,ObjectPtr<IObject> >   m_iid_map;
		ObjectPtr<System::MetaInfo::IWireManager>    m_ptrManager;
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
			if (iid==OMEGA_UUIDOF(System::MetaInfo::SafeProxy))
				return 0;

			// New interface required
			const System::MetaInfo::qi_rtti* pRtti = System::MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnCreateWireProxy)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			ptrNew.Attach(pRtti->pfnCreateWireProxy(this,m_ptrManager));
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

		void DoInvoke2(uint32_t method_id, System::MetaInfo::IWireStub* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout, IException*& pE);
		int DoInvoke(uint32_t method_id, System::MetaInfo::IWireStub* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout, IException*& pE);
	}
}

void OOCore::SEH::DoInvoke2(uint32_t method_id, System::MetaInfo::IWireStub* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout, IException*& pE)
{
	try
	{
		pStub->Invoke(method_id,pParamsIn,pParamsOut,timeout);
	}
	catch (IException* pE2)
	{
		pE = pE2;
	}
}

int OOCore::SEH::DoInvoke(uint32_t method_id, System::MetaInfo::IWireStub* pStub, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout, IException*& pE)
{
	int err = 0;

#if defined(ACE_WIN32)
	#if defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS)
		LPEXCEPTION_POINTERS ex = 0;
		ACE_SEH_TRY
		{
			DoInvoke2(method_id,pStub,pParamsIn,pParamsOut,timeout,pE);
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
				DoInvoke2(method_id,pStub,pParamsIn,pParamsOut,timeout,pE);
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
		DoInvoke2(method_id,pStub,pParamsIn,pParamsOut,timeout,pE);

	#endif
#else
	void* TODO; // Some kind of signal handler here please?
	DoInvoke2(method_id,pStub,pParamsIn,pParamsOut,timeout,pE);
#endif

	return err;
}

void OOCore::StdObjectManager::Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout)
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
		System::MetaInfo::wire_read(this,pParamsIn,oid);
		guid_t iid;
		System::MetaInfo::wire_read(this,pParamsIn,iid);

		method_id = pParamsIn->ReadUInt32();

		// IObject interface calls are not allowed on static interfaces!
		if (method_id < 3)
			OOCORE_THROW_ERRNO(EINVAL);

		// Create the required object
		ObjectPtr<IObject> ptrObject;
		ptrObject.Attach(CreateInstance(oid,Activation::Any,0,iid));

		// Get the handler for the interface
		const System::MetaInfo::qi_rtti* pRtti = System::MetaInfo::get_qi_rtti_info(iid);
		if (!pRtti || !pRtti->pfnCreateWireStub)
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

		// And create a stub
		ptrStub.Attach(pRtti->pfnCreateWireStub(this,ptrObject,0));
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
	int err = SEH::DoInvoke(method_id,ptrStub,pParamsIn,pParamsOut,timeout,pE);
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
		if (oid != guid_t::Null())
		{
			// Write the marshalling oid
			pStream->WriteByte(2);
			System::MetaInfo::wire_write(this,pStream,oid);

			// Let the custom handle marshalling...
			ptrMarshal->Marshal(pStream,iid,0);

			// Done
			return;
		}
	}

	// Get the handler for the interface
	const System::MetaInfo::qi_rtti* pRtti = System::MetaInfo::get_qi_rtti_info(iid);
	if (!pRtti || !pRtti->pfnCreateWireStub)
		throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

	// Generate a new key and stub pair
	ObjectPtr<System::MetaInfo::IWireStub> ptrStub;
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
			throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

		// Add to the map...
		m_mapStubIds.insert(std::map<uint32_t,ObjectPtr<System::MetaInfo::IWireStub> >::value_type(uId,ptrStub));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	// Write out the data
	pStream->WriteByte(1);
	System::MetaInfo::wire_write(this,pStream,iid);
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
		System::MetaInfo::wire_read(this,pStream,wire_iid);
		uint32_t uId = pStream->ReadUInt32();

		ObjectPtr<ObjectImpl<OOCore::StdProxy> > ptrProxy = ObjectImpl<OOCore::StdProxy>::CreateInstancePtr();
		ptrProxy->init(this,wire_iid,uId);

		if (iid == guid_t::Null())
			pObject = ptrProxy->QueryInterface(wire_iid);
		else
			pObject = ptrProxy->QueryInterface(iid);
	}
	else if (flag == 2)
	{
     	guid_t oid;
		System::MetaInfo::wire_read(this,pStream,oid);

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

		m_mapStubIds.erase(i);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

Omega::Serialize::IFormattedStream* OOCore::StdObjectManager::CreateOutputStream()
{
	if (!m_ptrChannel)
		OOCORE_THROW_ERRNO(EINVAL);

	return m_ptrChannel->CreateOutputStream();
}

Omega::Serialize::IFormattedStream* OOCore::StdObjectManager::SendAndReceive(Omega::Remoting::MethodAttributes_t attribs, Omega::Serialize::IFormattedStream* pStream, uint16_t timeout)
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
			guid_t iid = pE->ActualIID();
			const System::MetaInfo::qi_rtti* pRtti = System::MetaInfo::get_qi_rtti_info(iid);
			if (!pRtti || !pRtti->pfnThrow)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			pRtti->pfnThrow(pE);
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

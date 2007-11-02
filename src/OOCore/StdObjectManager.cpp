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
		static System::MetaInfo::IWireStub* CreateWireStub(const guid_t& iid, System::MetaInfo::IWireManager* pManager, IObject* pObject);
		static IObject* CreateWireProxy(const guid_t& iid, System::MetaInfo::IWireProxy* pProxy, System::MetaInfo::IWireManager* pManager);

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
}

void OOCore::WireStub::Init(IObject* pObject, uint32_t stub_id, Omega::System::MetaInfo::IWireManager* pManager)
{
	m_ptrObj = pObject;
	m_stub_id = stub_id;
	m_ptrManager = pManager;
}

ObjectPtr<System::MetaInfo::IWireStub> OOCore::WireStub::FindStub(const guid_t& iid)
{
	try
	{
		ObjectPtr<System::MetaInfo::IWireStub> ptrStub;
		bool bAdd = false;
		
		// See if we have a stub for this interface already...
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<const guid_t,ObjectPtr<System::MetaInfo::IWireStub> >::iterator i=m_iid_map.find(iid);
			if (i != m_iid_map.end())
				ptrStub = i->second;
			
			if (!ptrStub)
			{
				// See if any known interface supports the new interface
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					if (i->second->SupportsInterface(iid))
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
			ObjectPtr<IObject> ptrQI;
			ptrQI.Attach(m_ptrObj->QueryInterface(iid));
			if (!ptrQI)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
						
			// Create a stub for this interface
			ptrStub.Attach(wire_holder::CreateWireStub(iid,m_ptrManager,ptrQI));
			if (!ptrStub)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);

			bAdd = true;
		}

		if (bAdd)
		{
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
				
			std::pair<std::map<const guid_t,ObjectPtr<System::MetaInfo::IWireStub> >::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,ObjectPtr<System::MetaInfo::IWireStub> >::value_type(iid,ptrStub));
			if (!p.second)
				ptrStub = p.first->second;
		}

		return ptrStub;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}
}

void OOCore::WireStub::MarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid)
{
	ObjectPtr<System::MetaInfo::IWireStub> ptrStub = FindStub(iid);
		
	pStream->WriteUInt32(m_stub_id);
	write_guid(pStream,iid);
}

void OOCore::WireStub::ReleaseMarshalData(Serialize::IFormattedStream* pStream, const guid_t&)
{
	pStream->ReadUInt32();
	read_guid(pStream);
}

System::MetaInfo::IWireStub* OOCore::WireStub::UnmarshalStub(Serialize::IFormattedStream* pStream)
{
	return FindStub(read_guid(pStream)).AddRefReturn();
}

IObject* OOCore::WireStub::GetStubObject()
{
	return m_ptrObj;
}

void OOCore::WireProxy::Init(uint32_t proxy_id, Omega::System::MetaInfo::IWireManager* pManager)
{
	m_proxy_id = proxy_id;
	m_ptrManager = pManager;
}

IObject* OOCore::WireProxy::UnmarshalInterface(Serialize::IFormattedStream* pStream, const guid_t& iid)
{
	ObjectPtr<IObject> ptrProxy;

	guid_t wire_iid = read_guid(pStream);
		
	try
	{
		bool bAdd = false;
		
		// See if we have a proxy for this interface already...
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<const guid_t,ObjectPtr<IObject> >::iterator i=m_iid_map.find(wire_iid);
			if (i != m_iid_map.end())
				ptrProxy = i->second;
			
			if (!ptrProxy)
			{
				// See if any known interface supports the new interface
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{		
					ObjectPtr<IObject> ptrQI;
					ptrQI.Attach(i->second->QueryInterface(wire_iid));
					if (ptrQI)
					{
						bAdd = true;
						ptrProxy = i->second;
						break;
					}
				}
			}
		}

		if (!ptrProxy)
		{
			// Create a new proxy for this interface
			ptrProxy.Attach(wire_holder::CreateWireProxy(wire_iid,this,m_ptrManager));
			if (!ptrProxy)
				throw INoInterfaceException::Create(wire_iid,OMEGA_SOURCE_INFO);
			
			bAdd = true;
		}

		if (bAdd)
		{
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
				
			std::pair<std::map<const guid_t,ObjectPtr<IObject> >::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(wire_iid,ptrProxy));
			if (!p.second)
				ptrProxy = p.first->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	if (iid == OMEGA_UUIDOF(IObject))
	{
		AddRef();
		return this;
	}

	return ptrProxy->QueryInterface(iid);
}

void OOCore::WireProxy::WriteKey(Omega::Serialize::IFormattedStream* pStream)
{
	pStream->WriteUInt32(m_proxy_id);
}

bool OOCore::WireProxy::CallRemoteQI(const guid_t& iid)
{
	ObjectPtr<Serialize::IFormattedStream> pParamsOut;
	pParamsOut.Attach(m_ptrManager->CreateOutputStream());

	pParamsOut->WriteUInt32(m_proxy_id);
	write_guid(pParamsOut,OMEGA_UUIDOF(IObject));
	pParamsOut->WriteUInt32(2);

	write_guid(pParamsOut,iid);
    
	Serialize::IFormattedStream* pParamsIn = 0;
	IException* pE = m_ptrManager->SendAndReceive(0,pParamsOut,pParamsIn);
	if (pE)
		throw pE;

	ObjectPtr<Serialize::IFormattedStream> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	return ptrParamsIn->ReadBoolean();
}

IObject* OOCore::WireProxy::QI2(const Omega::guid_t& iid)
{
	ObjectPtr<IObject> ptrProxy;
		
	try
	{
		bool bAdd = false;

		// See if we have a proxy for this interface already...
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<const guid_t,ObjectPtr<IObject> >::iterator i=m_iid_map.find(iid);
			if (i != m_iid_map.end())
				ptrProxy = i->second;
			
			if (!ptrProxy)
			{
				// See if any known interface supports the new interface
				for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{		
					ObjectPtr<IObject> ptrQI;
					ptrQI.Attach(i->second->QueryInterface(iid));
					if (ptrQI)
					{
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
			if (!CallRemoteQI(iid))
				return 0;

			// Create a new proxy for this interface
			ptrProxy.Attach(wire_holder::CreateWireProxy(iid,this,m_ptrManager));
			if (!ptrProxy)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
			
			bAdd = true;
		}

		if (bAdd)
		{
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);
				
			std::pair<std::map<const guid_t,ObjectPtr<IObject> >::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,ObjectPtr<IObject> >::value_type(iid,ptrProxy));
			if (!p.second)
				ptrProxy = p.first->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	return ptrProxy->QueryInterface(iid);
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
		IObject* pOuter;
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
	ObjectPtr<System::MetaInfo::IWireStub> ptrStub;

	// Look up the stub
	try
	{
		OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,ObjectPtr<WireStub> >::iterator i=m_mapStubIds.find(stub_id);
		if (i==m_mapStubIds.end())
			OMEGA_THROW(L"Bad stub id");

		ptrStub.Attach(i->second->UnmarshalStub(pParamsIn));
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	// Read the method id
	uint32_t method_id = pParamsIn->ReadUInt32();
	
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
	ObjectPtr<IObject> ptrObject;
	ptrObject.Attach(pObject->QueryInterface(OMEGA_UUIDOF(IObject)));

	ObjectPtr<WireStub> ptrStub;
	try
	{
		OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<OTL::ObjectPtr<Omega::IObject>,OTL::ObjectPtr<WireStub> >::const_iterator i=m_mapStubObjs.find(ptrObject);
		if (i != m_mapStubObjs.end())
		{
			ptrStub = i->second;
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(string_t(e.what(),false));
	}

	if (!ptrStub)
	{
		// See if pObject does custom marshalling...
		//ObjectPtr<Remoting::IMarshal> ptrMarshal(ptrObject);
		//if (ptrMarshal)
		//{
		//	guid_t oid = ptrMarshal->GetUnmarshalOID(iid,0);
		//	if (oid != guid_t::Null())
		//	{
		//		// Write the marshalling oid
		//		pStream->WriteByte(2);
		//		System::MetaInfo::wire_write(pStream,oid);

		//		// Let the custom handle marshalling...
		//		ptrMarshal->Marshal(pStream,iid,0);

		//		// Done
		//		return;
		//	}
		//}

		// Create a new stub and stub id
		try
		{
			ptrStub = ObjectImpl<WireStub>::CreateInstance();

			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			uint32_t stub_id = m_uNextStubId++;
			while (stub_id==0 || m_mapStubIds.find(stub_id)!=m_mapStubIds.end())
			{
				stub_id = m_uNextStubId++;
			}

			ptrStub->Init(ptrObject,stub_id,this);

			// Add to the map...
			std::pair<std::map<OTL::ObjectPtr<Omega::IObject>,OTL::ObjectPtr<WireStub> >::iterator,bool> p=m_mapStubObjs.insert(std::map<OTL::ObjectPtr<Omega::IObject>,OTL::ObjectPtr<WireStub> >::value_type(ptrObject,ptrStub));
			if (!p.second)
				ptrStub = p.first->second;
			else
				m_mapStubIds.insert(std::map<uint32_t,ObjectPtr<WireStub> >::value_type(stub_id,ptrStub));
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(string_t(e.what(),false));
		}
	}

	// Write out the data
	pStream->WriteByte(1);
	ptrStub->MarshalInterface(pStream,iid);
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
		uint32_t proxy_id = pStream->ReadUInt32();

		// See if we have a proxy already...
		ObjectPtr<WireProxy> ptrProxy;
		try
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<uint32_t,ObjectPtr<WireProxy> >::iterator i=m_mapProxyIds.find(proxy_id);
			if (i != m_mapProxyIds.end())
				ptrProxy = i->second;
		}
		catch (std::exception& e)
		{
			OMEGA_THROW(string_t(e.what(),false));
		}

		if (!ptrProxy)
		{
			ptrProxy = ObjectImpl<WireProxy>::CreateInstance();
			ptrProxy->Init(proxy_id,this);

			try
			{
				OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				std::pair<std::map<uint32_t,ObjectPtr<WireProxy> >::iterator,bool> p = m_mapProxyIds.insert(std::map<uint32_t,ObjectPtr<WireProxy> >::value_type(proxy_id,ptrProxy));
				if (!p.second)
					ptrProxy = p.first->second;
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(string_t(e.what(),false));
			}
		}

		pObject = ptrProxy->UnmarshalInterface(pStream,iid);
	}
	//else if (flag == 2)
	//{
	//	guid_t oid;
	//	System::MetaInfo::wire_read(pStream,oid);

	//	// Create an instance of Oid
	//	ObjectPtr<Remoting::IMarshal> ptrMarshal(oid,Activation::InProcess);
	//	if (!ptrMarshal)
	//		throw INoInterfaceException::Create(OMEGA_UUIDOF(Remoting::IMarshal),OMEGA_SOURCE_INFO);

	//	pObject = ptrMarshal->Unmarshal(pStream,iid);
	//}
	else
		OOCORE_THROW_ERRNO(EINVAL);
}

void OOCore::StdObjectManager::ReleaseMarshalData(Serialize::IFormattedStream* pStream, const guid_t& iid, IObject* pObject)
{
	byte_t flag = pStream->ReadByte();

	if (flag == 1)
	{
		ObjectPtr<IObject> ptrObject;
		ptrObject.Attach(pObject->QueryInterface(OMEGA_UUIDOF(IObject)));

		ObjectPtr<WireStub> ptrStub;
		try
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<OTL::ObjectPtr<Omega::IObject>,OTL::ObjectPtr<WireStub> >::const_iterator i=m_mapStubObjs.find(ptrObject);
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
		if (!ptrStub)
			OOCORE_THROW_ERRNO(EINVAL);
		
		// Read the data
		ptrStub->ReleaseMarshalData(pStream,iid);
	}
	//else if (flag == 2)
	//{
	//	// Skip the guid...
	//	read_guid(pStream);

	//	// See if pObject does custom marshalling...
	//	ObjectPtr<Remoting::IMarshal> ptrMarshal(ptrObject);
	//	if (!ptrMarshal)
	//		OOCORE_THROW_ERRNO(EINVAL);
	//	
	//	ptrMarshal->ReleaseMarhslData(pStream,iid,0);
	//}
}

void OOCore::StdObjectManager::ReleaseStub(uint32_t uId)
{
	try
	{
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		std::map<uint32_t,ObjectPtr<WireStub> >::iterator i=m_mapStubIds.find(uId);
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

Omega::Serialize::IFormattedStream* OOCore::StdObjectManager::CreateOutputStream(IObject* pOuter)
{
	if (!m_ptrChannel)
		OOCORE_THROW_ERRNO(EINVAL);

	return m_ptrChannel->CreateOutputStream(pOuter);
}

IException* OOCore::StdObjectManager::SendAndReceive(Remoting::MethodAttributes_t attribs, Serialize::IFormattedStream* pSend, Serialize::IFormattedStream*& pRecv, uint16_t timeout)
{
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
				IObject* pE;
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
	IException* pE = SendAndReceive(0,ptrParamsOut,pParamsIn,0);
	if (pE)
	{
		ReleaseMarshalData(ptrParamsOut,OMEGA_UUIDOF(IObject),pOuter);
		throw pE;
	}

	ObjectPtr<Serialize::IFormattedStream> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	UnmarshalInterface(ptrParamsIn,iid,pObject);
}

OMEGA_DEFINE_OID(Omega::Remoting,OID_StdObjectManager,"{63EB243E-6AE3-43bd-B073-764E096775F8}");
OMEGA_DEFINE_OID(Omega::Remoting,OID_InterProcess,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");

System::MetaInfo::IWireStub* OOCore::wire_holder::CreateWireStub(const guid_t& iid, System::MetaInfo::IWireManager* pManager, IObject* pObject)
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
		System::MetaInfo::marshal_info<System::MetaInfo::IWireStub*&>::safe_type::coerce(pRet),
		System::MetaInfo::marshal_info<System::MetaInfo::IWireManager*>::safe_type::coerce(pManager),
		System::MetaInfo::marshal_info<IObject*>::safe_type::coerce(pObject,iid));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

IObject* OOCore::wire_holder::CreateWireProxy(const guid_t& iid, System::MetaInfo::IWireProxy* pProxy, System::MetaInfo::IWireManager* pManager)
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
		System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(pRet),
		System::MetaInfo::marshal_info<System::MetaInfo::IWireProxy*>::safe_type::coerce(pProxy),
		System::MetaInfo::marshal_info<System::MetaInfo::IWireManager*>::safe_type::coerce(pManager));

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

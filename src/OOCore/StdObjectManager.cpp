///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "StdObjectManager.h"
#include "WireProxy.h"
#include "WireStub.h"
#include "IPS.h"

// This is all the SEH guff - needs to go into the surrogate project
#if 0

#if defined(ACE_WIN32) && !defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS) && defined (__GNUC__)
#include <setjmp.h>
#endif

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

		void DoInvoke2(System::Internal::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE);
		int DoInvoke(System::Internal::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE);
	}

void OOCore::SEH::DoInvoke2(System::Internal::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE)
{
	try
	{
		System::Internal::IException_Safe* pSE = pStub->Invoke_Safe(
			System::Internal::marshal_info<Remoting::IMessage*>::safe_type::coerce(pParamsIn),
			System::Internal::marshal_info<Remoting::IMessage*>::safe_type::coerce(pParamsOut));

		if (pSE)
			throw_correct_exception(pSE);
	}
	catch (IException* pE2)
	{
		pE = pE2;
	}
}

int OOCore::SEH::DoInvoke(System::Internal::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE)
{
	int err = 0;

#if defined(_WIN32)
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
	#elif defined (__GNUC__) && !defined(OMEGA_WIN64)

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

#endif

using namespace Omega;
using namespace OTL;

namespace
{
	struct CallContext
	{
		CallContext() :
			m_deadline(OOBase::timeval_t::MaxTime),
			m_src_id(0),
			m_flags(0)
		{}

		virtual ~CallContext()
		{
		}

		OOBase::timeval_t           m_deadline;
		uint32_t                 m_src_id;
		Remoting::MarshalFlags_t m_flags;
	};

	class StdCallContext :
		public ObjectBase,
		public Remoting::ICallContext
	{
		BEGIN_INTERFACE_MAP(StdCallContext)
			INTERFACE_ENTRY(Remoting::ICallContext)
		END_INTERFACE_MAP()

	public:
		uint32_t Timeout();
		bool_t HasTimedOut();
		uint32_t SourceId();
		Remoting::MarshalFlags_t SourceType();

	public:
		CallContext m_cc;
	};

	ObjectPtr<Remoting::IProxy> GetWireProxy(IObject* pObject)
	{
		ObjectPtr<Remoting::IProxy> ptrProxy;

		ObjectPtr<System::Internal::ISafeProxy> ptrSProxy(pObject);
		if (ptrSProxy)
		{
			System::Internal::auto_safe_shim shim = ptrSProxy->GetShim(OMEGA_GUIDOF(IObject));
			if (shim && static_cast<const System::Internal::IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe)
			{
				// Retrieve the underlying wire proxy
				System::Internal::auto_safe_shim proxy;
				const System::Internal::SafeShim* pE = static_cast<const System::Internal::IObject_Safe_VTable*>(shim->m_vtable)->pfnGetWireProxy_Safe(shim,&proxy);
				if (pE)
					System::Internal::throw_correct_exception(pE);

				ptrProxy.Attach(System::Internal::create_safe_proxy<Remoting::IProxy>(proxy));
			}
		}

		return ptrProxy;
	}
}

uint32_t StdCallContext::Timeout()
{
	OOBase::timeval_t now = OOBase::gettimeofday();
	if (m_cc.m_deadline <= now)
		return 0;

	if (m_cc.m_deadline == OOBase::timeval_t::MaxTime)
		return (uint32_t)-1;

	return (m_cc.m_deadline - now).msec();
}

bool_t StdCallContext::HasTimedOut()
{
	return (m_cc.m_deadline <= OOBase::gettimeofday());
}

uint32_t StdCallContext::SourceId()
{
	return m_cc.m_src_id;
}

Remoting::MarshalFlags_t StdCallContext::SourceType()
{
	return m_cc.m_flags;
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
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	if (m_ptrChannel)
		OMEGA_THROW(L"ObjectManager already connected to a Channel");

	m_ptrChannel = pChannel;
}

void OOCore::StdObjectManager::Shutdown()
{
	std::list<ObjectPtr<ObjectImpl<Stub> > >  listStubs;
	std::list<ObjectImpl<Proxy>* > listProxies;

	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	// Copy the stub map
	for (std::map<IObject*,ObjectPtr<ObjectImpl<Stub> > >::iterator i=m_mapStubObjs.begin();i!=m_mapStubObjs.end();++i)
		listStubs.push_back(i->second);

	m_mapStubIds.clear();
	m_mapStubObjs.clear();

	// Copy the proxys
	for (std::map<uint32_t,ObjectImpl<Proxy>* >::iterator j=m_mapProxyIds.begin();j!=m_mapProxyIds.end();++j)
		listProxies.push_back(j->second);

	m_mapProxyIds.clear();

	guard.release();

	// Now do the final releases on the stub and proxies
	listProxies.clear();
	listStubs.clear();
}

void OOCore::StdObjectManager::InvokeGetRemoteInstance(Remoting::IMessage* pParamsIn, ObjectPtr<Remoting::IMessage>& ptrResponse)
{
	// Read the oid, iid and flags
	string_t strOID = pParamsIn->ReadString(L"oid");
	guid_t iid = pParamsIn->ReadGuid(L"iid");
	Activation::Flags_t act_flags;
	System::Internal::wire_read(L"flags",pParamsIn,act_flags);

	// Check our permissions
	if (m_ptrChannel->GetMarshalFlags() == Remoting::RemoteMachine)
		act_flags |= Activation::RemoteActivation;

	// Work out the oid
	guid_t oid;
	if (!guid_t::FromString(strOID,oid))
		oid = Activation::NameToOid(strOID);

	// Get the required object
	ObjectPtr<IObject> ptrObject;
	ptrObject.Attach(Activation::GetRegisteredObject(oid,act_flags,iid));

	// Write it out and return
	MarshalInterface(L"$retval",ptrResponse,iid,ptrObject);
}

Remoting::IMessage* OOCore::StdObjectManager::Invoke(Remoting::IMessage* pParamsIn, uint32_t timeout)
{
	if (!pParamsIn)
		OMEGA_THROW(L"Invoke called with no message");

	if (!m_ptrChannel)
		throw Remoting::IChannelClosedException::Create();

	// Stash call context
	CallContext* pCC = 0;
	pCC = OOBase::TLSSingleton<CallContext,OOCore::DLL>::instance();
	CallContext old_context;
	if (pCC)
        old_context = *pCC;

	try
	{
		if (timeout)
			pCC->m_deadline = OOBase::timeval_t::deadline(timeout);
		else
			pCC->m_deadline = OOBase::timeval_t::MaxTime;

		pCC->m_src_id = m_ptrChannel->GetSource();
		pCC->m_flags = m_ptrChannel->GetMarshalFlags();

		// Create a response
		ObjectPtr<Remoting::IMessage> ptrResponse;
		ptrResponse.Attach(m_ptrChannel->CreateMessage());

		// Assume we succeed
		ptrResponse->WriteStructStart(L"ipc_response",L"$ipc_response_type");
		ptrResponse->WriteBoolean(L"$throw",false);

		try
		{
			// Read the header start
			pParamsIn->ReadStructStart(L"ipc_request",L"$ipc_request_type");

			// Read the stub id
			uint32_t stub_id = pParamsIn->ReadUInt32(L"$stub_id");
			if (stub_id == 0)
			{
				uint32_t method_id = pParamsIn->ReadUInt32(L"$method_id");
				if (method_id == 0)
				{
					// It's a call from GetRemoteInstance
					InvokeGetRemoteInstance(pParamsIn,ptrResponse);
				}
				else
					OMEGA_THROW(L"Invalid static function call!");
			}
			else
			{
				// It's a method call on a stub...
				ObjectPtr<ObjectImpl<Stub> > ptrStub;

				// Look up the stub
				try
				{
					OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

					std::map<uint32_t,std::map<IObject*,ObjectPtr<ObjectImpl<Stub> > >::iterator>::iterator i=m_mapStubIds.find(stub_id);
					if (i==m_mapStubIds.end())
						OMEGA_THROW(L"Bad stub id");

					ptrStub = i->second->second;
				}
				catch (std::exception& e)
				{
					OMEGA_THROW(e);
				}

				// Ask the stub to make the call
				ptrStub->Invoke(pParamsIn,ptrResponse);
			}
		}
		catch (IException* pE)
		{
			// Make sure the excpetion is released
			ObjectPtr<IException> ptrE;
			ptrE.Attach(pE);

			// Dump the previous output and create a fresh output
			ptrResponse.Attach(m_ptrChannel->CreateMessage());
			ptrResponse->WriteStructStart(L"ipc_response",L"$ipc_response_type");

			ptrResponse->WriteBoolean(L"$throw",true);

			// Write the exception onto the wire
			MarshalInterface(L"exception",ptrResponse,pE->GetThrownIID(),pE);
		}

		// Close the struct block
		ptrResponse->WriteStructEnd(L"ipc_response");

		// Restore context
		*pCC = old_context;

		return ptrResponse.AddRef();
	}
	catch (...)
	{
		// Restore context
		*pCC = old_context;
		throw;
	}
}

void OOCore::StdObjectManager::GetRemoteInstance(const string_t& strOID, Activation::Flags_t flags, const guid_t& iid, IObject*& pObject)
{
	pObject = 0;

	ObjectPtr<Remoting::IMessage> ptrParamsOut;
	ptrParamsOut.Attach(CreateMessage());

	ptrParamsOut->WriteStructStart(L"ipc_request",L"$ipc_request_type");

	ptrParamsOut->WriteUInt32(L"$stub_id",0);
	ptrParamsOut->WriteUInt32(L"$method_id",0);
	ptrParamsOut->WriteString(L"oid",strOID);
	ptrParamsOut->WriteGuid(L"iid",iid);
	System::Internal::wire_write(L"flags",ptrParamsOut,flags);

	ptrParamsOut->WriteStructEnd(L"ipc_request");

	Remoting::IMessage* pParamsIn = 0;
	IException* pERet;

	try
	{
		pERet = SendAndReceive(TypeInfo::Synchronous,ptrParamsOut,pParamsIn);
	}
	catch (...)
	{
		ptrParamsOut->ReadStructStart(L"ipc_request",L"$ipc_request_type");

		ptrParamsOut->ReadUInt32(L"$stub_id");
		ptrParamsOut->ReadUInt32(L"$method_id");
		ptrParamsOut->ReadGuid(L"oid");
		ptrParamsOut->ReadGuid(L"iid");

		Activation::Flags_t f;
		System::Internal::wire_read(L"flags",ptrParamsOut,f);

		ptrParamsOut->ReadStructEnd(L"ipc_request");

		throw;
	}

	if (pERet)
		pERet->Throw();

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	UnmarshalInterface(L"$retval",ptrParamsIn,iid,pObject);
}

bool OOCore::StdObjectManager::IsAlive()
{
	if (!m_ptrChannel)
		return false;

	return m_ptrChannel->IsConnected();
}

Remoting::IMessage* OOCore::StdObjectManager::CreateMessage()
{
	if (!m_ptrChannel)
		throw Remoting::IChannelClosedException::Create();

	return m_ptrChannel->CreateMessage();
}

IException* OOCore::StdObjectManager::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	if (!m_ptrChannel)
		throw Remoting::IChannelClosedException::Create();

	Remoting::IMessage* pInternalRecv = 0;
	IException* pE = m_ptrChannel->SendAndReceive(attribs,pSend,pInternalRecv,timeout);
	if (pE)
		return pE;

	try
	{
		ObjectPtr<Remoting::IMessage> ptrRecv;
		ptrRecv.Attach(pInternalRecv);
		
		if (!(attribs & TypeInfo::Asynchronous))
		{
			assert(pInternalRecv);

			// Read the header
			ptrRecv->ReadStructStart(L"ipc_response",L"$ipc_response_type");

			// Read exception status
			if (ptrRecv->ReadBoolean(L"$throw"))
			{
				// Unmarshal the exception
				ObjectPtr<IException> ptrE = ObjectPtr<Remoting::IMarshaller>(static_cast<Remoting::IMarshaller*>(this)).UnmarshalInterface<IException>(L"exception",ptrRecv);
				if (!ptrE)
					OMEGA_THROW(L"Null exception returned");

				return ptrE.AddRef();
			}
		}

		pRecv = ptrRecv.AddRef();
		return 0;
	}
	catch (IException* pE)
	{
		return pE;
	}
}

TypeInfo::ITypeInfo* OOCore::StdObjectManager::GetTypeInfo(const guid_t& iid)
{
	// Check the auto registered stuff first
	TypeInfo::ITypeInfo* pRet = OOCore::GetTypeInfo(iid);
	if (!pRet)
	{
		// Ask the other end if it has a clue?
		void* TODO;
	}

	return pRet;
}

void OOCore::StdObjectManager::RemoveProxy(uint32_t proxy_id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	m_mapProxyIds.erase(proxy_id);
}

void OOCore::StdObjectManager::RemoveStub(uint32_t stub_id)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	std::map<uint32_t,std::map<IObject*,ObjectPtr<ObjectImpl<Stub> > >::iterator>::iterator i=m_mapStubIds.find(stub_id);
	if (i != m_mapStubIds.end())
	{
		m_mapStubObjs.erase(i->second);
		m_mapStubIds.erase(i);
	}
}

bool OOCore::StdObjectManager::CustomMarshalInterface(const wchar_t* pszName, ObjectPtr<Remoting::IMarshal>& ptrMarshal, const guid_t& iid, Remoting::IMessage* pMessage)
{
	Remoting::MarshalFlags_t marshal_flags = m_ptrChannel->GetMarshalFlags();

	guid_t oid = ptrMarshal->GetUnmarshalFactoryOID(iid,marshal_flags);
	if (oid == guid_t::Null())
		return false;

	size_t undo_count = 0;
	try
	{
		// Write the marshalling oid
		pMessage->WriteByte(L"$marshal_type",2);
		++undo_count;

		pMessage->WriteGuid(L"$oid",oid);
		++undo_count;

		// Let the custom handle marshalling...
		ptrMarshal->MarshalInterface(this,pMessage,iid,marshal_flags);
		++undo_count;

		// Write the struct end
		pMessage->WriteStructEnd(pszName);
	}
	catch (...)
	{
		if (undo_count > 0)
			pMessage->ReadByte(L"$marshal_type");

		if (undo_count > 1)
			pMessage->ReadGuid(L"$oid");

		if (undo_count > 2)
			ptrMarshal->ReleaseMarshalData(this,pMessage,iid,marshal_flags);

		throw;
	}
	
	return true;
}

void OOCore::StdObjectManager::MarshalInterface(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject)
{
	try
	{
		if (!m_ptrChannel)
			throw Remoting::IChannelClosedException::Create();

		// Write a header
		pMessage->WriteStructStart(pszName,L"$iface_marshal");

		// See if object is NULL
		if (!pObject)
		{
			pMessage->WriteByte(L"$marshal_type",0);
			pMessage->WriteStructEnd(pszName);
			return;
		}

		// See if we have a stub already...
		ObjectPtr<IObject> ptrObj;
		ptrObj.Attach(pObject->QueryInterface(OMEGA_GUIDOF(IObject)));

		ObjectPtr<ObjectImpl<Stub> > ptrStub;
		{
			OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

			std::map<IObject*,ObjectPtr<ObjectImpl<Stub> > >::const_iterator i=m_mapStubObjs.find(ptrObj);
			if (i != m_mapStubObjs.end())
				ptrStub = i->second;
		}

		if (!ptrStub)
		{
			ObjectPtr<Remoting::IMarshal> ptrMarshal;

			// See if pObject is a SafeProxy wrapping a WireProxy...
			ObjectPtr<Remoting::IProxy> ptrProxy = GetWireProxy(pObject);
			if (ptrProxy)
				ptrMarshal.Attach(ptrProxy.QueryInterface<Remoting::IMarshal>());

			// See if pObject does custom marshalling...
			if (!ptrMarshal)
				ptrMarshal.Attach(static_cast<Remoting::IMarshal*>(pObject->QueryInterface(OMEGA_GUIDOF(Remoting::IMarshal))));
			
			// See if custom marshalling is possible...
			if (ptrMarshal && CustomMarshalInterface(pszName,ptrMarshal,iid,pMessage))
				return;
							
			// Create a new stub and stub id
			OOBase::Guard<OOBase::RWMutex> guard(m_lock);

			uint32_t stub_id = m_uNextStubId++;
			while (stub_id==0 || m_mapStubIds.find(stub_id)!=m_mapStubIds.end())
			{
				stub_id = m_uNextStubId++;
			}

			ptrStub = ObjectImpl<Stub>::CreateInstancePtr();
			ptrStub->init(ptrObj,stub_id,this);

			// Add to the map...
			std::pair<std::map<IObject*,ObjectPtr<ObjectImpl<Stub> > >::iterator,bool> p=m_mapStubObjs.insert(std::map<IObject*,ObjectPtr<ObjectImpl<Stub> > >::value_type(ptrObj,ptrStub));
			if (!p.second)
				ptrStub = p.first->second;
			else
				m_mapStubIds.insert(std::map<uint32_t,std::map<IObject*,ObjectPtr<ObjectImpl<Stub> > >::iterator>::value_type(stub_id,p.first));
		}

		// Write out the data
		pMessage->WriteByte(L"$marshal_type",1);

		ptrStub->MarshalInterface(pMessage,iid);

		pMessage->WriteStructEnd(pszName);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void OOCore::StdObjectManager::UnmarshalInterface(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject*& pObject)
{
	try
	{
		if (!m_ptrChannel)
			throw Remoting::IChannelClosedException::Create();

		// Read the header
		pMessage->ReadStructStart(pszName,L"$iface_marshal");

		byte_t flag = pMessage->ReadByte(L"$marshal_type");
		if (flag == 0)
		{
			// NOP
			pObject = 0;
		}
		else if (flag == 1)
		{
			uint32_t proxy_id = pMessage->ReadUInt32(L"id");

			// See if we have a proxy already...
			ObjectPtr<ObjectImpl<Proxy> > ptrProxy;
			{
				OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

				std::map<uint32_t,ObjectImpl<Proxy>*>::iterator i=m_mapProxyIds.find(proxy_id);
				if (i != m_mapProxyIds.end())
					ptrProxy = i->second;
			}

			if (!ptrProxy)
			{
				ptrProxy = ObjectImpl<Proxy>::CreateInstancePtr();
				ptrProxy->init(proxy_id,this);

				OOBase::Guard<OOBase::RWMutex> guard(m_lock);

				std::pair<std::map<uint32_t,ObjectImpl<Proxy>*>::iterator,bool> p = m_mapProxyIds.insert(std::map<uint32_t,ObjectImpl<Proxy>*>::value_type(proxy_id,ptrProxy));
				if (!p.second)
					ptrProxy = p.first->second;
			}

			// Unmarshal the object
			ObjectPtr<IObject> ptrObj;
			ptrObj.Attach(ptrProxy->UnmarshalInterface(pMessage));

			// QI for the desired interface
			pObject = ptrObj->QueryInterface(iid);
			if (!pObject)
				throw INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
		}
		else if (flag == 2)
		{
			guid_t oid = pMessage->ReadGuid(L"$oid");

			// Create an instance of Oid
			ObjectPtr<Remoting::IMarshalFactory> ptrMarshalFactory(oid,Activation::InProcess);
			if (!ptrMarshalFactory)
				throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshalFactory),OMEGA_SOURCE_INFO);

			ptrMarshalFactory->UnmarshalInterface(this,pMessage,iid,m_ptrChannel->GetMarshalFlags(),pObject);
		}
		else
			OMEGA_THROW(L"Invalid marshal flag");

		pMessage->ReadStructEnd(pszName);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void OOCore::StdObjectManager::ReleaseMarshalData(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject)
{
	try
	{
		if (!m_ptrChannel)
			throw Remoting::IChannelClosedException::Create();

		// Read the header
		pMessage->ReadStructStart(pszName,L"$iface_marshal");

		byte_t flag = pMessage->ReadByte(L"$marshal_type");
		if (flag == 0)
		{
			/* NOP */
		}
		else if (flag == 1)
		{
			// Skip the stub id
			pMessage->ReadUInt32(L"id");

			IObject* pObj = pObject->QueryInterface(OMEGA_GUIDOF(IObject));
			ObjectPtr<IObject> ptrObj;
			ptrObj.Attach(pObj);

			ObjectPtr<ObjectImpl<Stub> > ptrStub;
			try
			{
				OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

				std::map<IObject*,ObjectPtr<ObjectImpl<Stub> > >::const_iterator i=m_mapStubObjs.find(ptrObj);
				if (i != m_mapStubObjs.end())
					ptrStub = i->second;
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}

			// If there is no stub... what are we unmarshalling?
			if (!ptrStub)
				OMEGA_THROW(L"No stub to unmarshal");

			// Read the data
			ptrStub->ReleaseMarshalData(pMessage,iid);
		}
		else if (flag == 2)
		{
			// Skip the guid...
			guid_t oid = pMessage->ReadGuid(L"oid");

			// See if pObject does custom marshalling...
			ObjectPtr<Remoting::IMarshal> ptrMarshal(pObject);

			if (!ptrMarshal)
				throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshal),OMEGA_SOURCE_INFO);

			ptrMarshal->ReleaseMarshalData(this,pMessage,iid,m_ptrChannel->GetMarshalFlags());
		}
		else
		{
			OMEGA_THROW(L"Invalid marshal flag");
		}

		pMessage->ReadStructEnd(pszName);
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

void OOCore::StdObjectManager::DoMarshalChannel(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pParamsOut)
{
	// QI pObjectManager for a private interface - it will have it because pObjectManager is
	// an instance of StdObjectManager 2 calls up the stack..
	// Call a private method that marshals the channel...
	ObjectPtr<IStdObjectManager> ptrOM(pMarshaller);
	assert(ptrOM);

	ptrOM->MarshalChannel(this,pParamsOut,m_ptrChannel->GetMarshalFlags());
}

void OOCore::StdObjectManager::MarshalChannel(Remoting::IMarshaller* pMarshaller, Remoting::IMessage* pMessage, Remoting::MarshalFlags_t flags)
{
	ObjectPtr<Remoting::IMarshal> ptrMarshal(m_ptrChannel);
	if (!ptrMarshal)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshal),OMEGA_SOURCE_INFO);

	// The following format is the same as IObjectManager::UnmarshalInterface...
	pMessage->WriteStructStart(L"m_ptrChannel",L"$iface_marshal");
	pMessage->WriteByte(L"$marshal_type",2);

	guid_t oid = ptrMarshal->GetUnmarshalFactoryOID(OMEGA_GUIDOF(Remoting::IChannel),flags);
	if (oid == guid_t::Null())
		OMEGA_THROW(L"Channels must support custom marshalling if they support reflection");

	pMessage->WriteGuid(L"$oid",oid);

	ptrMarshal->MarshalInterface(pMarshaller,pMessage,OMEGA_GUIDOF(Remoting::IChannel),flags);

	pMessage->WriteStructEnd(L"m_ptrChannel");
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::ICallContext*,OOCore_Remoting_GetCallContext,0,())
{
	ObjectPtr<ObjectImpl<StdCallContext> > ptrCC = ObjectImpl<StdCallContext>::CreateInstancePtr();

	ptrCC->m_cc = *OOBase::TLSSingleton<CallContext,OOCore::DLL>::instance();

	return ptrCC.AddRef();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(bool_t,OOCore_Remoting_IsAlive,1,((in),IObject*,pObject))
{
	bool_t ret = false;
	if (pObject)
	{
		ret = true;

		ObjectPtr<Remoting::IProxy> ptrProxy = GetWireProxy(pObject);
		if (ptrProxy)
			ret = ptrProxy->IsAlive();
	}

	return ret;
}

OMEGA_DEFINE_OID(OOCore,OID_ProxyMarshalFactory,"{69099DD8-A628-458a-861F-009E016DB81B}");
OMEGA_DEFINE_OID(OOCore,OID_InterProcessService,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");
OMEGA_DEFINE_OID(Remoting,OID_StdObjectManager,"{63EB243E-6AE3-43bd-B073-764E096775F8}");

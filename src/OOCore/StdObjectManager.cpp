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

		void DoInvoke2(System::MetaInfo::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE);
		int DoInvoke(System::MetaInfo::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE);
	}

void OOCore::SEH::DoInvoke2(System::MetaInfo::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE)
{
	try
	{
		System::MetaInfo::IException_Safe* pSE = pStub->Invoke_Safe(
			System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pParamsIn),
			System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pParamsOut));

		if (pSE)
			throw_correct_exception(pSE);
	}
	catch (IException* pE2)
	{
		pE = pE2;
	}
}

int OOCore::SEH::DoInvoke(System::MetaInfo::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE)
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
			m_deadline(OOBase::timeval_t::max_time),
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
}

uint32_t StdCallContext::Timeout()
{
	OOBase::timeval_t now = OOBase::gettimeofday();
	if (m_cc.m_deadline <= now)
		return 0;

	if (m_cc.m_deadline == OOBase::timeval_t::max_time)
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

void OOCore::StdObjectManager::Connect(Remoting::IChannelBase* pChannel)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);

	if (m_ptrChannel)
		OMEGA_THROW(L"ObjectManager already connected to a Channel");

	m_ptrChannel = pChannel;
	if (!m_ptrChannel)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IChannel),OMEGA_SOURCE_INFO);
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
	string_t strOID;
	pParamsIn->ReadStrings(L"oid",1,&strOID);
	guid_t iid;
	System::MetaInfo::wire_read(L"iid",pParamsIn,iid);
	Activation::Flags_t act_flags;
	System::MetaInfo::wire_read(L"flags",pParamsIn,act_flags);

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
		OMEGA_THROW(L"ObjectManager not connected to a channel");

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
			pCC->m_deadline = OOBase::timeval_t::max_time;

		pCC->m_src_id = m_ptrChannel->GetSource();
		pCC->m_flags = m_ptrChannel->GetMarshalFlags();

		// Create a response
		ObjectPtr<Remoting::IMessage> ptrResponse;
		ptrResponse.Attach(m_ptrChannel->CreateMessage());

		// Assume we succeed
		ptrResponse->WriteStructStart(L"ipc_response",L"$ipc_response_type");

		bool_t v = false;
		ptrResponse->WriteBooleans(L"$throw",1,&v);

		try
		{
			// Read the header start
			pParamsIn->ReadStructStart(L"ipc_request",L"$ipc_request_type");

			// Read the stub id
			uint32_t stub_id;
			System::MetaInfo::wire_read(L"$stub_id",pParamsIn,stub_id);
			if (stub_id == 0)
			{
				uint32_t method_id;
				System::MetaInfo::wire_read(L"$method_id",pParamsIn,method_id);
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
				ObjectPtr<System::IStub> ptrStub;

				// Look up the stub
				try
				{
					OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

					std::map<Omega::uint32_t,std::map<Omega::IObject*,ObjectPtr<ObjectImpl<Stub> > >::iterator>::iterator i=m_mapStubIds.find(stub_id);
					if (i==m_mapStubIds.end())
						OMEGA_THROW(L"Bad stub id");

					ptrStub = i->second->second->LookupStub(pParamsIn);
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

			bool_t v = true;
			ptrResponse->WriteBooleans(L"$throw",1,&v);

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

	System::MetaInfo::wire_write(L"$stub_id",ptrParamsOut,(uint32_t)0);
	System::MetaInfo::wire_write(L"$method_id",ptrParamsOut,(uint32_t)0);
	System::MetaInfo::wire_write(L"oid",ptrParamsOut,strOID);
	System::MetaInfo::wire_write(L"iid",ptrParamsOut,iid);
	System::MetaInfo::wire_write(L"flags",ptrParamsOut,flags);

	ptrParamsOut->WriteStructEnd(L"ipc_request");

	Remoting::IMessage* pParamsIn = 0;
	IException* pERet;

	try
	{
		pERet = SendAndReceive(TypeInfo::Synchronous,ptrParamsOut,pParamsIn);
	}
	catch (IException*)
	{
		ptrParamsOut->ReadStructStart(L"ipc_request",L"$ipc_request_type");

		uint32_t u32;
		System::MetaInfo::wire_read(L"$stub_id",ptrParamsOut,u32);
		System::MetaInfo::wire_read(L"$method_id",ptrParamsOut,u32);
		guid_t g;
		System::MetaInfo::wire_read(L"oid",ptrParamsOut,g);
		System::MetaInfo::wire_read(L"iid",ptrParamsOut,g);

		Activation::Flags_t f;
		System::MetaInfo::wire_read(L"flags",ptrParamsOut,f);

		ptrParamsOut->ReadStructEnd(L"ipc_request");

		throw;
	}

	if (pERet)
		throw pERet;

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
		OMEGA_THROW(L"ObjectManager is not connected");

	return m_ptrChannel->CreateMessage();
}

IException* OOCore::StdObjectManager::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	if (!m_ptrChannel)
		OMEGA_THROW(L"ObjectManager is not connected");

	IException* pE = m_ptrChannel->SendAndReceive(attribs,pSend,pRecv,timeout);
	if (pE)
		return pE;
	
	try
	{
		ObjectPtr<Remoting::IMessage> ptrRecv;
		ptrRecv.Attach(pRecv);
		pRecv = 0;

		if (!(attribs & TypeInfo::Asynchronous))
		{
			assert(ptrRecv);

			// Read the header
			ptrRecv->ReadStructStart(L"ipc_response",L"$ipc_response_type");

			// Read exception status
			bool_t bthrow;
			System::MetaInfo::wire_read(L"$throw",ptrRecv,bthrow);
			if (bthrow)
			{
				// Unmarshal the exception
				IObject* pE = 0;
				UnmarshalInterface(L"exception",ptrRecv,OMEGA_GUIDOF(IException),pE);
				if (!pE)
					OMEGA_THROW(L"Null exception returned");

				return static_cast<IException*>(pE);
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

	std::map<Omega::uint32_t,std::map<Omega::IObject*,ObjectPtr<ObjectImpl<Stub> > >::iterator>::iterator i=m_mapStubIds.find(stub_id);
	if (i==m_mapStubIds.end())
		OMEGA_THROW(L"Bad stub id");

	m_mapStubObjs.erase(i->second);
	m_mapStubIds.erase(i);
}

void OOCore::StdObjectManager::MarshalInterface(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject)
{
	try
	{
		if (!m_ptrChannel)
			OMEGA_THROW(L"ObjectManager is not connected");

		// Write a header
		pMessage->WriteStructStart(pszName,L"$iface_marshal");
		
		// See if object is NULL
		if (!pObject)
		{
			System::MetaInfo::wire_write(L"$marshal_type",pMessage,(byte_t)0);
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
			{
				ptrStub = i->second;
			}
		}

		if (!ptrStub)
		{
			// See if pObject does custom marshalling...
			ObjectPtr<Remoting::IMarshal> ptrMarshal(pObject);
			if (ptrMarshal)
			{
				Remoting::MarshalFlags_t marshal_flags = m_ptrChannel->GetMarshalFlags();

				guid_t oid = ptrMarshal->GetUnmarshalFactoryOID(iid,marshal_flags);
				if (oid != guid_t::Null())
				{
					size_t undo_count = 0;
					try
					{
						// Write the marshalling oid
						System::MetaInfo::wire_write(L"$marshal_type",pMessage,(byte_t)2);
						++undo_count;

						System::MetaInfo::wire_write(L"$oid",pMessage,oid);
						++undo_count;

						// Let the custom handle marshalling...
						ptrMarshal->MarshalInterface(this,pMessage,iid,marshal_flags);
						++undo_count;

						// Write the struct end
						pMessage->WriteStructEnd(pszName);
						return;
					}
					catch (...)
					{
						void* TODO; // This won't work without correct unwinding semantics on a message

						if (undo_count > 0)
						{
							byte_t v;
							System::MetaInfo::wire_read(L"$marshal_type",pMessage,v);
						}

						if (undo_count > 1)
						{
							guid_t v;
							System::MetaInfo::wire_read(L"$oid",pMessage,v);
						}

						if (undo_count > 2)
							ptrMarshal->ReleaseMarshalData(this,pMessage,iid,marshal_flags);
						
						throw;
					}
				}
			}

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
		System::MetaInfo::wire_write(L"$marshal_type",pMessage,(byte_t)1);
		
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
			OMEGA_THROW(L"ObjectManager is not connected");

		// Read the header
		pMessage->ReadStructStart(pszName,L"$iface_marshal");
		
		byte_t flag;
		System::MetaInfo::wire_read(L"$marshal_type",pMessage,flag);
		
		if (flag == 0)
		{
			// NOP
			pObject = 0;
		}
		else if (flag == 1)
		{
			uint32_t proxy_id;
			System::MetaInfo::wire_read(L"id",pMessage,proxy_id);
			
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

			pObject = ptrProxy->UnmarshalInterface(pMessage,iid);
		}
		else if (flag == 2)
		{
			guid_t oid;
			System::MetaInfo::wire_read(L"$oid",pMessage,oid);
			
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
			OMEGA_THROW(L"ObjectManager is not connected");

		// Read the header
		pMessage->ReadStructStart(pszName,L"$iface_marshal");
		
		byte_t flag;
		System::MetaInfo::wire_read(L"$marshal_type",pMessage,flag);
		
		if (flag == 0)
		{
			/* NOP */
		}
		else if (flag == 1)
		{
			// Skip the stub id
			uint32_t stub_id;
			System::MetaInfo::wire_read(L"id",pMessage,stub_id);
			
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
			guid_t oid;
			System::MetaInfo::wire_read(L"oid",pMessage,oid);
			
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

void OOCore::StdObjectManager::DoMarshalChannel(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pParamsOut)
{
	if (!m_ptrChannel)
		OMEGA_THROW(L"ObjectManager is not connected");

	// QI pObjectManager for a private interface - it will have it because pObjectManager is 
	// an instance of StdObjectManager 2 calls up the stack..
	// Call a private method that marshals the channel...
	ObjectPtr<IStdObjectManager> ptrOM(pObjectManager);
	assert(ptrOM);
	
	ptrOM->MarshalChannel(this,pParamsOut,m_ptrChannel->GetMarshalFlags());	
}

void OOCore::StdObjectManager::MarshalChannel(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pMessage, Remoting::MarshalFlags_t flags)
{
	ObjectPtr<Remoting::IMarshal> ptrMarshal(m_ptrChannel);
	if (!ptrMarshal)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshal),OMEGA_SOURCE_INFO);

	// The following format is the same as IObjectManager::UnmarshalInterface...
	pMessage->WriteStructStart(L"m_ptrChannel",L"$iface_marshal");
	System::MetaInfo::wire_write(L"$marshal_type",pMessage,(byte_t)2);

	guid_t oid = ptrMarshal->GetUnmarshalFactoryOID(OMEGA_GUIDOF(Remoting::IChannel),flags);
	if (oid == guid_t::Null())
		OMEGA_THROW(L"Channels must support custom marshalling if they support reflection");

	System::MetaInfo::wire_write(L"$oid",pMessage,oid);

	ptrMarshal->MarshalInterface(pObjectManager,pMessage,OMEGA_GUIDOF(Remoting::IChannel),flags);
	
	pMessage->WriteStructEnd(L"m_ptrChannel");		
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::ICallContext*,OOCore_Remoting_GetCallContext,0,())
{
	ObjectPtr<ObjectImpl<StdCallContext> > ptrCC = ObjectImpl<StdCallContext>::CreateInstancePtr();

	ptrCC->m_cc = *OOBase::TLSSingleton<CallContext,OOCore::DLL>::instance();

	return ptrCC.AddRef();
}

OMEGA_DEFINE_OID(OOCore,OID_ProxyMarshalFactory,"{69099DD8-A628-458a-861F-009E016DB81B}");
OMEGA_DEFINE_OID(Remoting,OID_StdObjectManager,"{63EB243E-6AE3-43bd-B073-764E096775F8}");
OMEGA_DEFINE_OID(System,OID_InterProcessService,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");

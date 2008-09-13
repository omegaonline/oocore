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

#include "./StdObjectManager.h"
#include "./WireProxy.h"
#include "./WireStub.h"
#include "./WireImpl.h"

#if defined(ACE_WIN32) && !defined(ACE_HAS_WIN32_STRUCTURAL_EXCEPTIONS) && defined (__GNUC__)
#include <setjmp.h>
#endif

using namespace Omega;
using namespace OTL;

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

		void DoInvoke2(System::MetaInfo::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE);
		int DoInvoke(System::MetaInfo::IStub_Safe* pStub, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut, IException*& pE);
	}

	struct CallContext
	{
		CallContext() :
			m_deadline(ACE_Time_Value::max_time),
			m_src_id(0),
			m_flags(0)
		{}

		virtual ~CallContext()
		{
		}

		ACE_Time_Value           m_deadline;
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

uint32_t OOCore::StdCallContext::Timeout()
{
	ACE_Time_Value now = ACE_OS::gettimeofday();
	if (m_cc.m_deadline <= now)
		return 0;

	if (m_cc.m_deadline == ACE_Time_Value::max_time)
		return (uint32_t)-1;

	return (m_cc.m_deadline - now).msec();
}

bool_t OOCore::StdCallContext::HasTimedOut()
{
	return (m_cc.m_deadline <= ACE_OS::gettimeofday());
}

uint32_t OOCore::StdCallContext::SourceId()
{
	return m_cc.m_src_id;
}

Remoting::MarshalFlags_t OOCore::StdCallContext::SourceType()
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
	if (m_ptrChannel)
		OMEGA_THROW(EALREADY);

	m_ptrChannel = pChannel;
	if (!m_ptrChannel)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IChannel),OMEGA_SOURCE_INFO);
}

void OOCore::StdObjectManager::Disconnect()
{
	std::list<Stub*> listStubs;
	std::list<Proxy*> listProxies;

	{
		OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

		// Copy the stub map
		for (std::map<uint32_t,std::map<System::MetaInfo::IObject_Safe*,Stub*>::iterator>::iterator i=m_mapStubIds.begin();i!=m_mapStubIds.end();++i)
			listStubs.push_back(i->second->second);
		
		m_mapStubIds.clear();
		m_mapStubObjs.clear();

		// Copy the proxys
		for (std::map<uint32_t,Proxy*>::iterator j=m_mapProxyIds.begin();j!=m_mapProxyIds.end();++j)
			listProxies.push_back(j->second);
		
		m_mapProxyIds.clear();
	}

	// Disconnect the proxies
	for (std::list<Proxy*>::iterator j=listProxies.begin();j!=listProxies.end();++j)
		(*j)->Disconnect();

	// Release the stubs
	for (std::list<Stub*>::iterator i=listStubs.begin();i!=listStubs.end();++i)
		(*i)->Release_Safe();
}

void OOCore::StdObjectManager::InvokeGetRemoteInstance(Remoting::IMessage* pParamsIn, ObjectPtr<Remoting::IMessage>& ptrResponse)
{
	// Read the oid, iid and flags
	string_t strOID;
	pParamsIn->ReadStrings(L"oid",1,&strOID);
	guid_t iid = ReadGuid(L"iid",pParamsIn);
	Activation::Flags_t act_flags = static_cast<Activation::Flags_t>(ReadUInt16(L"flags",pParamsIn));

	// Check our permissions
	if (m_ptrChannel->GetMarshalFlags() == Remoting::RemoteMachine)
		act_flags |= Activation::RemoteServer;

	// Work out the oid
	guid_t oid = guid_t::FromString(strOID);
	if (oid == guid_t::Null())
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
		OMEGA_THROW(EINVAL);

	// Stash call context
	CallContext* pCC = 0;
	pCC = ACE_TSS_Singleton<CallContext,ACE_Recursive_Thread_Mutex>::instance();
	CallContext old_context;
	if (pCC)
        old_context = *pCC;

	try
	{
		if (timeout)
			pCC->m_deadline = ACE_OS::gettimeofday() + ACE_Time_Value(timeout / 1000,(timeout % 1000) * 1000);
		else
			pCC->m_deadline = ACE_Time_Value::max_time;

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
			uint32_t stub_id = ReadUInt32(L"$stub_id",pParamsIn);
			if (stub_id == 0)
			{
				uint16_t method_id = ReadUInt16(L"$method_id",pParamsIn);
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
				System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IStub_Safe> ptrStub;

				// Look up the stub
				try
				{
					OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

					std::map<uint32_t,std::map<System::MetaInfo::IObject_Safe*,Stub*>::iterator>::iterator i=m_mapStubIds.find(stub_id);
					if (i==m_mapStubIds.end())
						OMEGA_THROW(L"Bad stub id");

					ptrStub.attach(i->second->second->LookupStub(pParamsIn));
				}
				catch (std::exception& e)
				{
					OMEGA_THROW(e);
				}

				// Ask the stub to make the call
				IException* pE = 0;
				int err = SEH::DoInvoke(ptrStub,pParamsIn,ptrResponse,pE);

				if (pE)
					throw pE;
				else if (err != 0)
					OMEGA_THROW(err);
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

	WriteUInt32(L"$stub_id",ptrParamsOut,0);
	WriteUInt16(L"$method_id",ptrParamsOut,0);
	ptrParamsOut->WriteStrings(L"oid",1,&strOID);
	WriteGuid(L"iid",ptrParamsOut,iid);
	WriteUInt16(L"flags",ptrParamsOut,flags);

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

		ReadUInt32(L"$stub_id",ptrParamsOut);
		ReadUInt16(L"$method_id",ptrParamsOut);
		ReadGuid(L"oid",ptrParamsOut);
		ReadGuid(L"iid",ptrParamsOut);
		ReadUInt16(L"flags",ptrParamsOut);

		ptrParamsOut->ReadStructEnd(L"ipc_request");

		throw;
	}

	if (pERet)
		throw pERet;

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	UnmarshalInterface(L"$retval",ptrParamsIn,iid,pObject);
}

void OOCore::StdObjectManager::MarshalInterface(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject)
{
	System::MetaInfo::IException_Safe* pSE = MarshalInterface_Safe(
		pszName,
		System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage),
		&iid,
		System::MetaInfo::marshal_info<IObject*>::safe_type::coerce(pObject,iid));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);
}

void OOCore::StdObjectManager::UnmarshalInterface(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject*& pObject)
{
	System::MetaInfo::IException_Safe* pSE = UnmarshalInterface_Safe(
		pszName,
		System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage),
		&iid,
		System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(pObject,iid));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);
}

void OOCore::StdObjectManager::ReleaseMarshalData(const wchar_t* pszName, Remoting::IMessage* pMessage, const guid_t& iid, IObject* pObject)
{
	System::MetaInfo::IException_Safe* pSE = ReleaseMarshalData_Safe(
		pszName,
		System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage),
		&iid,
		System::MetaInfo::marshal_info<IObject*>::safe_type::coerce(pObject,iid));

	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);
}

bool OOCore::StdObjectManager::IsAlive()
{
	return (m_ptrChannel->GetSource() != 0);
}

Remoting::IMessage* OOCore::StdObjectManager::CreateMessage()
{
	return m_ptrChannel->CreateMessage();
}

IException* OOCore::StdObjectManager::SendAndReceive(TypeInfo::MethodAttributes_t attribs, Remoting::IMessage* pSend, Remoting::IMessage*& pRecv, uint32_t timeout)
{
	IException* pE = m_ptrChannel->SendAndReceive(attribs,pSend,pRecv,timeout);
	if (pE)
		return pE;

	ObjectPtr<Remoting::IMessage> ptrRecv;
	ptrRecv.Attach(pRecv);
	pRecv = 0;

	if (!(attribs & TypeInfo::Asynchronous))
	{
		try
		{
			if (!ptrRecv)
				OMEGA_THROW(L"No response received");

			// Read the header
			ptrRecv->ReadStructStart(L"ipc_response",L"$ipc_response_type");

			// Read exception status
			if (ReadBoolean(L"$throw",ptrRecv))
			{
				// Unmarshal the exception
				IObject* pE = 0;
				UnmarshalInterface(L"exception",ptrRecv,OMEGA_GUIDOF(IException),pE);

				if (!pE)
					OMEGA_THROW(L"Null exception returned");

				return static_cast<IException*>(pE);
			}
		}
		catch (IException* pE)
		{
			return pE;
		}
	}

	pRecv = ptrRecv.AddRef();
	return 0;
}

TypeInfo::ITypeInfo* OOCore::StdObjectManager::GetTypeInfo(const guid_t& iid)
{
	TypeInfo::ITypeInfo* pRet = 0;

	System::MetaInfo::IException_Safe* pSE = GetTypeInfo_Safe(System::MetaInfo::marshal_info<TypeInfo::ITypeInfo*&>::safe_type::coerce(pRet),&iid);
	if (pSE)
		System::MetaInfo::throw_correct_exception(pSE);

	return pRet;
}

void OOCore::StdObjectManager::RemoveProxy(uint32_t proxy_id)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapProxyIds.erase(proxy_id);
}

void OOCore::StdObjectManager::RemoveStub(uint32_t stub_id)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	std::map<uint32_t,std::map<System::MetaInfo::IObject_Safe*,Stub*>::iterator>::iterator i=m_mapStubIds.find(stub_id);
	if (i==m_mapStubIds.end())
		OMEGA_THROW(L"Bad stub id");

	i->second->second->Release_Safe();

	m_mapStubObjs.erase(i->second);
	m_mapStubIds.erase(i);
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
	if (*piid == OMEGA_GUIDOF(IObject) || *piid == OMEGA_GUIDOF(System::IMarshaller))
	{
		*ppS = static_cast<System::MetaInfo::IMarshaller_Safe*>(this);
		(*ppS)->AddRef_Safe();
	}
	
	return 0;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::MarshalInterface_Safe(const wchar_t* pszName, System::MetaInfo::IMessage_Safe* pMessage, const guid_t* piid, System::MetaInfo::IObject_Safe* pObject)
{
	try
	{
		// Write a header
		System::MetaInfo::IException_Safe* pSE = pMessage->WriteStructStart_Safe(pszName,L"$iface_marshal");
		if (pSE)
			return pSE;

		// See if object is NULL
		if (!pObject)
		{
			pSE = System::MetaInfo::wire_write(L"$marshal_type",pMessage,(byte_t)0);
			if (!pSE)
				pSE = pMessage->WriteStructEnd_Safe(pszName);
			return pSE;
		}

		// See if we have a stub already...
		System::MetaInfo::IObject_Safe* pObjS = 0;
		pSE = pObject->QueryInterface_Safe(&OMEGA_GUIDOF(IObject),&pObjS);
		if (pSE)
			return pSE;
		System::MetaInfo::auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);

		System::MetaInfo::auto_iface_safe_ptr<Stub> ptrStub;
		{
			OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			std::map<IObject_Safe*,Stub*>::const_iterator i=m_mapStubObjs.find(ptrObjS);
			if (i != m_mapStubObjs.end())
			{
				ptrStub = i->second;
			}
		}

		if (!ptrStub)
		{
			// See if pObject does custom marshalling...
			System::MetaInfo::IObject_Safe* pMarshal = 0;
			pSE = pObject->QueryInterface_Safe(&OMEGA_GUIDOF(Remoting::IMarshal),&pMarshal);
			if (pSE)
				return pSE;
			System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class> ptrMarshal(static_cast<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class*>(pMarshal));
				
			if (ptrMarshal)
			{
				Remoting::MarshalFlags_t marshal_flags = m_ptrChannel->GetMarshalFlags();

				guid_t oid;
				pSE = ptrMarshal->GetUnmarshalFactoryOID_Safe(&oid,piid,marshal_flags);
				if (pSE)
					return pSE;

				if (oid != guid_t::Null())
				{
					void* REFACTOR; // Split this out to a seperate fn!

					size_t undo_count = 0;

					// Write the marshalling oid
					pSE = System::MetaInfo::wire_write(L"$marshal_type",pMessage,(byte_t)2);
					if (pSE)
						goto Cleanup;
					++undo_count;

					pSE = System::MetaInfo::wire_write(L"$oid",pMessage,oid);
					if (pSE)
						goto Cleanup;
					++undo_count;

					// Let the custom handle marshalling...
					pSE = ptrMarshal->MarshalInterface_Safe(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(this),pMessage,piid,marshal_flags);
					if (pSE)
						goto Cleanup;
					++undo_count;

					// Write the struct end
					pSE = pMessage->WriteStructEnd_Safe(pszName);
					if (!pSE)
						return 0;

				Cleanup:
					System::MetaInfo::IException_Safe* pSE2 = 0;
					if (undo_count > 0)
					{
						byte_t v;
						pSE2 = System::MetaInfo::wire_read(L"$marshal_type",pMessage,v);
						if (pSE2)
						{
							pSE->Release_Safe();
							return pSE2;
						}
					}

					if (undo_count > 1)
					{
						guid_t v;
						pSE2 = System::MetaInfo::wire_read(L"$oid",pMessage,v);
						if (pSE2)
						{
							pSE->Release_Safe();
							return pSE2;
						}
					}

					if (undo_count > 2)
					{
						pSE2 = ptrMarshal->ReleaseMarshalData_Safe(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(this),pMessage,piid,marshal_flags);
						if (pSE2)
						{
							pSE->Release_Safe();
							return pSE2;
						}
					}

					return pSE;
				}
			}

			// Create a new stub and stub id
			OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

			uint32_t stub_id = m_uNextStubId++;
			while (stub_id==0 || m_mapStubIds.find(stub_id)!=m_mapStubIds.end())
			{
				stub_id = m_uNextStubId++;
			}

			OMEGA_NEW(ptrStub,Stub(ptrObjS,stub_id,this));

			// Add to the map...
			std::pair<std::map<System::MetaInfo::IObject_Safe*,Stub*>::iterator,bool> p=m_mapStubObjs.insert(std::map<System::MetaInfo::IObject_Safe*,Stub*>::value_type(ptrObjS,ptrStub));
			if (!p.second)
				ptrStub = p.first->second;
			else
			{
				m_mapStubIds.insert(std::map<uint32_t,std::map<System::MetaInfo::IObject_Safe*,Stub*>::iterator>::value_type(stub_id,p.first));
				ptrStub->AddRef_Safe();
			}
		}

		// Write out the data
		pSE = System::MetaInfo::wire_write(L"$marshal_type",pMessage,(byte_t)1);
		if (pSE)
			return pSE;

		pSE = ptrStub->MarshalInterface(pMessage,*piid);
		if (pSE)
		{
			byte_t v;
			System::MetaInfo::IException_Safe* pSE2 = System::MetaInfo::wire_read(L"$marshal_type",pMessage,v);
			if (pSE2)
			{
				pSE->Release_Safe();
				return pSE2;
			}
			return pSE;
		}

		pSE = pMessage->WriteStructEnd_Safe(pszName);
		if (pSE)
		{
			byte_t v;
			System::MetaInfo::IException_Safe* pSE2 = System::MetaInfo::wire_read(L"$marshal_type",pMessage,v);
			if (pSE2)
			{
				pSE->Release_Safe();
				return pSE2;
			}

			pSE2 = ptrStub->ReleaseMarshalData(pMessage,*piid);
			if (pSE2)
			{
				pSE->Release_Safe();
				return pSE2;
			}
		}

		return pSE;
	}
	catch (std::exception& e)
	{
		return System::MetaInfo::return_safe_exception(ISystemException::Create(e,OMEGA_SOURCE_INFO));
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::UnmarshalInterface_Safe(const wchar_t* pszName, System::MetaInfo::IMessage_Safe* pMessage, const guid_t* piid, System::MetaInfo::IObject_Safe** ppObjS)
{
	try
	{
		// Read the header
		System::MetaInfo::IException_Safe* pSE = pMessage->ReadStructStart_Safe(pszName,L"$iface_marshal");
		if (pSE)
			return pSE;

		byte_t flag;
		pSE = System::MetaInfo::wire_read(L"$marshal_type",pMessage,flag);
		if (pSE)
			return pSE;

		if (flag == 0)
		{
			*ppObjS = 0;
		}
		else if (flag == 1)
		{
			uint32_t proxy_id;
			pSE = System::MetaInfo::wire_read(L"id",pMessage,proxy_id);
			if (pSE)
				return pSE;

			// See if we have a proxy already...
			System::MetaInfo::auto_iface_safe_ptr<Proxy> ptrProxy;
			{
				OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				std::map<uint32_t,Proxy*>::iterator i=m_mapProxyIds.find(proxy_id);
				if (i != m_mapProxyIds.end())
					ptrProxy = i->second;
			}

			if (!ptrProxy)
			{
				OMEGA_NEW(ptrProxy,Proxy(proxy_id,this));

				OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				std::pair<std::map<uint32_t,Proxy*>::iterator,bool> p = m_mapProxyIds.insert(std::map<uint32_t,Proxy*>::value_type(proxy_id,ptrProxy));
				if (!p.second)
					ptrProxy = p.first->second;
			}

			*ppObjS = ptrProxy->UnmarshalInterface(pMessage,*piid);
		}
		else if (flag == 2)
		{
			guid_t oid;
			pSE = System::MetaInfo::wire_read(L"$oid",pMessage,oid);
			if (pSE)
				return pSE;

			// Create an instance of Oid
			ObjectPtr<Remoting::IMarshalFactory> ptrMarshalFactory(oid,Activation::InProcess);
			if (!ptrMarshalFactory)
				throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshalFactory),OMEGA_SOURCE_INFO);

			ptrMarshalFactory->UnmarshalInterface(
				this,
				System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage),
				*piid,m_ptrChannel->GetMarshalFlags(),
				System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(ppObjS,piid));
		}
		else
			OMEGA_THROW(EINVAL);

		return pMessage->ReadStructEnd_Safe(pszName);
	}
	catch (std::exception& e)
	{
		return System::MetaInfo::return_safe_exception(ISystemException::Create(e,OMEGA_SOURCE_INFO));
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::ReleaseMarshalData_Safe(const wchar_t* pszName, System::MetaInfo::IMessage_Safe* pMessage, const guid_t* piid, System::MetaInfo::IObject_Safe* pObject)
{
	try
	{
		// Read the header
		System::MetaInfo::IException_Safe* pSE = pMessage->ReadStructStart_Safe(pszName,L"$iface_marshal");
		if (pSE)
			return pSE;

		byte_t flag;
		pSE = System::MetaInfo::wire_read(L"$marshal_type",pMessage,flag);
		if (pSE)
			return pSE;

		if (flag == 0)
		{
			/* NOP */
		}
		else if (flag == 1)
		{
			// Skip the stub id
			uint32_t stub_id;
			pSE = System::MetaInfo::wire_read(L"id",pMessage,stub_id);
			if (pSE)
				return pSE;

			System::MetaInfo::IObject_Safe* pObjS = 0;
			pSE = pObject->QueryInterface_Safe(&OMEGA_GUIDOF(IObject),&pObjS);
			if (pSE)
				return pSE;
			System::MetaInfo::auto_iface_safe_ptr<IObject_Safe> ptrObjS(pObjS);

			System::MetaInfo::auto_iface_safe_ptr<Stub> ptrStub;
			try
			{
				OOCORE_READ_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

				std::map<System::MetaInfo::IObject_Safe*,Stub*>::const_iterator i=m_mapStubObjs.find(ptrObjS);
				if (i != m_mapStubObjs.end())
				{
					ptrStub = i->second;
				}
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}

			// If there is no stub... what are we unmarshalling?
			if (ptrStub)
				OMEGA_THROW(EINVAL);

			// Read the data
			pSE = ptrStub->ReleaseMarshalData(pMessage,*piid);
			if (pSE)
				return pSE;
		}
		else if (flag == 2)
		{
			// Skip the guid...
			guid_t oid;
			pSE = System::MetaInfo::wire_read(L"oid",pMessage,oid);
			if (pSE)
				return pSE;

			// See if pObject does custom marshalling...
			System::MetaInfo::IObject_Safe* pMarshal = 0;
			pSE = pObject->QueryInterface_Safe(&OMEGA_GUIDOF(Remoting::IMarshal),&pMarshal);
			if (pSE)
				return pSE;

			if (!pMarshal)
				OMEGA_THROW(EINVAL);

			System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class> ptrMarshal(static_cast<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class*>(pMarshal));

			pSE = ptrMarshal->ReleaseMarshalData_Safe(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(this),pMessage,piid,m_ptrChannel->GetMarshalFlags());
			if (pSE)
				return pSE;
		}
		else
		{
			OMEGA_THROW(EINVAL);
		}

		return pMessage->ReadStructEnd_Safe(pszName);
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
	catch (std::exception& e)
	{
		return System::MetaInfo::return_safe_exception(ISystemException::Create(e,OMEGA_SOURCE_INFO));
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::CreateMessage_Safe(System::MetaInfo::IMessage_Safe** ppRet)
{
	try
	{
		static_cast<Remoting::IMessage*&>(System::MetaInfo::marshal_info<Remoting::IMessage*&>::safe_type::coerce(ppRet)) = CreateMessage();
		return 0;
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::SendAndReceive_Safe(System::MetaInfo::IException_Safe** ppRet, TypeInfo::MethodAttributes_t attribs, System::MetaInfo::IMessage_Safe* pSend, System::MetaInfo::IMessage_Safe** ppRecv, uint32_t timeout)
{
	try
	{
		static_cast<IException*&>(System::MetaInfo::marshal_info<IException*&>::safe_type::coerce(ppRet)) = SendAndReceive(attribs,System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pSend),System::MetaInfo::marshal_info<Remoting::IMessage*&>::safe_type::coerce(ppRecv),timeout);
		return 0;
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::GetTypeInfo_Safe(System::MetaInfo::ITypeInfo_Safe** ppTypeInfo, const guid_t* piid)
{
	// Check the auto registered stuff first
	*ppTypeInfo = OOCore::GetTypeInfo(*piid);

	if (!(*ppTypeInfo))
	{
		// Ask the other end if it has a clue?
		void* TODO;
	}

	return 0;
}

void OOCore::StdObjectManager::DoMarshalChannel(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pParamsOut)
{
	// QI pObjectManager for a private interface - it will have it because pObjectManager is 
	// an instance of StdObjectManager 2 calls up the stack..
	// Call a private method that marshals the channel...
	
	ObjectPtr<IStdObjectManager> ptrOM(pObjectManager);
	if (!ptrOM)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(IStdObjectManager),OMEGA_SOURCE_INFO);

	ptrOM->MarshalChannel(this,pParamsOut,m_ptrChannel->GetMarshalFlags());	
}

void OOCore::StdObjectManager::MarshalChannel(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pMessage, Remoting::MarshalFlags_t flags)
{
	ObjectPtr<Remoting::IMarshal> ptrMarshal(m_ptrChannel);
	if (!ptrMarshal)
		throw INoInterfaceException::Create(OMEGA_GUIDOF(Remoting::IMarshal),OMEGA_SOURCE_INFO);

	// The following format is the same as IObjectManager::UnmarshalInterface...
	pMessage->WriteStructStart(L"m_ptrChannel",L"$iface_marshal");
	byte_t two = 2;
	pMessage->WriteBytes(L"$marshal_type",1,&two);

	guid_t oid = ptrMarshal->GetUnmarshalFactoryOID(OMEGA_GUIDOF(Remoting::IChannel),flags);
	if (oid == guid_t::Null())
		OMEGA_THROW(L"Channels must support custom marshalling if they support reflection");

	WriteGuid(L"$oid",pMessage,oid);

	ptrMarshal->MarshalInterface(pObjectManager,pMessage,OMEGA_GUIDOF(Remoting::IChannel),flags);
	
	pMessage->WriteStructEnd(L"m_ptrChannel");		
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::ICallContext*,Remoting_GetCallContext,0,())
{
	ObjectPtr<ObjectImpl<OOCore::StdCallContext> > ptrCC = ObjectImpl<OOCore::StdCallContext>::CreateInstancePtr();

	ptrCC->m_cc = *ACE_TSS_Singleton<OOCore::CallContext,ACE_Recursive_Thread_Mutex>::instance();

	return ptrCC.AddRef();
}

OMEGA_DEFINE_OID(OOCore,OID_ProxyMarshalFactory,"{69099DD8-A628-458a-861F-009E016DB81B}");
OMEGA_DEFINE_OID(Remoting,OID_StdObjectManager,"{63EB243E-6AE3-43bd-B073-764E096775F8}");
OMEGA_DEFINE_OID(System,OID_InterProcessService,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");

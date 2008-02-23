///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
//
// This file is part of OOCore, the OmegaOnline Core library.
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

void OOCore::StdObjectManagerMarshalFactory::UnmarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t& iid, Omega::Remoting::MarshalFlags_t flags, Omega::IObject*& pObject)
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
			ptrOM->Connect(ptrChannel,flags);
		}
	}

	if (!ptrOM)
		throw INoInterfaceException::Create(OMEGA_UUIDOF(Remoting::IChannel),OMEGA_SOURCE_INFO);

	pObject = ptrOM->QueryInterface(iid);	
}

OOCore::StdObjectManager::StdObjectManager() :
	m_uNextStubId(1)
{
}

OOCore::StdObjectManager::~StdObjectManager()
{
}

void OOCore::StdObjectManager::Connect(Remoting::IChannel* pChannel, Remoting::MarshalFlags_t marshal_flags)
{
	if (m_ptrChannel)
		OMEGA_THROW(EALREADY);

	m_ptrChannel = pChannel;
	m_marshal_flags = marshal_flags;
}

void OOCore::StdObjectManager::Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut)
{
	if (!pParamsIn)
		OMEGA_THROW(EINVAL);

	// Assume we succeed...
	if (pParamsOut)
		pParamsOut->WriteBoolean(true);

	// Read the stub id
	uint32_t stub_id = pParamsIn->ReadUInt32();
	if (stub_id == 0)
	{
		// It's a call from CreateRemoteInstance

		// Check we have a response!
		if (!pParamsOut)
			OMEGA_THROW(L"Async CreateRemoteInstance!");
				
		// Read the oid and iid
		guid_t oid = pParamsIn->ReadGuid();
		guid_t iid = pParamsIn->ReadGuid();

		// Read the outer object
		IObject* pOuter = 0;
		UnmarshalInterface(pParamsIn,OMEGA_UUIDOF(IObject),pOuter);
		ObjectPtr<IObject> ptrOuter;
		ptrOuter.Attach(pOuter);

		// Get the required object
		ObjectPtr<IObject> ptrObject;
		ptrObject.Attach(Activation::GetRegisteredObject(oid,Activation::InProcess | Activation::DontLaunch,iid));
			
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

		std::map<uint32_t,std::map<System::MetaInfo::IObject_Safe*,WireStub*>::iterator>::iterator i=m_mapStubIds.find(stub_id);
		if (i==m_mapStubIds.end())
			OMEGA_THROW(L"Bad stub id");

		ptrStub.attach(i->second->second->LookupStub(pParamsIn));
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
		OMEGA_THROW(err);
}

void OOCore::StdObjectManager::Disconnect()
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	// clear the stub map
	for (std::map<uint32_t,std::map<System::MetaInfo::IObject_Safe*,WireStub*>::iterator>::iterator i=m_mapStubIds.begin();i!=m_mapStubIds.end();++i)
	{
		i->second->second->Release_Safe();
	}
	m_mapStubIds.clear();
	m_mapStubObjs.clear();

	// shutdown the proxys
	for (std::map<uint32_t,WireProxy*>::iterator j=m_mapProxyIds.begin();j!=m_mapProxyIds.end();++j)
	{
		j->second->Disconnect();
	}

	// Disconnect
	m_ptrChannel.Release();
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

Serialize::IFormattedStream* OOCore::StdObjectManager::CreateOutputStream()
{
	if (!m_ptrChannel)
		OMEGA_THROW(EINVAL);

	return m_ptrChannel->CreateOutputStream();
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
				IObject* pE = 0;
				UnmarshalInterface(ptrRecv,OMEGA_UUIDOF(IException),pE);

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

	pRecv = ptrRecv.Detach();
	return 0;
}

void OOCore::StdObjectManager::RemoveProxy(uint32_t proxy_id)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	m_mapProxyIds.erase(proxy_id);
}

void OOCore::StdObjectManager::RemoveStub(uint32_t stub_id)
{
	OOCORE_WRITE_GUARD(ACE_RW_Thread_Mutex,guard,m_lock);

	std::map<uint32_t,std::map<System::MetaInfo::IObject_Safe*,WireStub*>::iterator>::iterator i=m_mapStubIds.find(stub_id);
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
					size_t undo_count = 0;

					// Write the marshalling oid
					pSE = pStream->WriteByte_Safe(2);
					if (pSE)
						goto Cleanup;
					++undo_count;

					pSE = System::MetaInfo::wire_write(pStream,oid);
					if (pSE)
						goto Cleanup;
					++undo_count;

					// Let the custom handle marshalling...
					pSE = ptrMarshal->MarshalInterface_Safe(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(this),pStream,piid,m_marshal_flags);
					if (!pSE)
						return 0;

				Cleanup:
					System::MetaInfo::IException_Safe* pSE2 = 0;
					if (undo_count > 0)
					{
						byte_t v;
						pSE2 = pStream->ReadByte_Safe(&v);
						if (pSE2)
						{
							pSE->Release_Safe();
							return pSE2;
						}
					}

					if (undo_count > 1)
					{
						guid_t v;
						pSE2 = System::MetaInfo::wire_read(pStream,v);
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

			OMEGA_NEW(ptrStub,WireStub(ptrObjS,stub_id,this));
			
			// Add to the map...
			std::pair<std::map<System::MetaInfo::IObject_Safe*,WireStub*>::iterator,bool> p=m_mapStubObjs.insert(std::map<System::MetaInfo::IObject_Safe*,WireStub*>::value_type(ptrObjS,ptrStub));
			if (!p.second)
				ptrStub = p.first->second;
			else
			{
				m_mapStubIds.insert(std::map<uint32_t,std::map<System::MetaInfo::IObject_Safe*,WireStub*>::iterator>::value_type(stub_id,p.first));
				ptrStub->AddRef_Safe();
			}
		}	

		// Write out the data
		pSE = pStream->WriteByte_Safe(1);
		if (pSE)
			return pSE;

		pSE = ptrStub->MarshalInterface(pStream,*piid);
		if (pSE)
		{
			byte_t v;
			System::MetaInfo::IException_Safe* pSE2 = pStream->ReadByte_Safe(&v);
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
				*piid,m_marshal_flags,
				System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(ppObjS));
		}
		else
			OMEGA_THROW(EINVAL);

		return 0;
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
				OMEGA_THROW(EINVAL);
			
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
				OMEGA_THROW(EINVAL);

			System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class> ptrMarshal(static_cast<System::MetaInfo::interface_info<Remoting::IMarshal>::safe_class*>(pMarshal));

			// Skip the guid...
			guid_t oid;
			pSE = System::MetaInfo::wire_read(pStream,oid);
			if (pSE)
				return pSE;
		
			return ptrMarshal->ReleaseMarshalData_Safe(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(this),pStream,piid,m_marshal_flags);
		}
		else
		{
			OMEGA_THROW(EINVAL);
		}
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

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::CreateOutputStream_Safe(System::MetaInfo::IFormattedStream_Safe** ppRet)
{
	try
	{
		static_cast<Serialize::IFormattedStream*&>(System::MetaInfo::marshal_info<Serialize::IFormattedStream*&>::safe_type::coerce(ppRet)) = CreateOutputStream();
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
	ptrParamsOut->WriteGuid(oid);
	ptrParamsOut->WriteGuid(iid);
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

Omega::guid_t OOCore::StdObjectManager::GetUnmarshalFactoryOID(const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
{
	return OID_StdObjectManagerMarshalFactory;
}

void OOCore::StdObjectManager::MarshalInterface(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
{
	pObjectManager->MarshalInterface(pStream,OMEGA_UUIDOF(Remoting::IChannel),m_ptrChannel);
}

void OOCore::StdObjectManager::ReleaseMarshalData(Omega::Remoting::IObjectManager* pObjectManager, Omega::Serialize::IFormattedStream* pStream, const Omega::guid_t&, Omega::Remoting::MarshalFlags_t)
{
	pObjectManager->ReleaseMarshalData(pStream,OMEGA_UUIDOF(Remoting::IChannel),m_ptrChannel);
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::GetUnmarshalFactoryOID_Safe(guid_t* pRet, const guid_t*, Remoting::MarshalFlags_t)
{
	*pRet = OID_StdObjectManagerMarshalFactory;
	return 0;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::MarshalInterface_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, Remoting::MarshalFlags_t flags)
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

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::StdObjectManager::ReleaseMarshalData_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, Remoting::MarshalFlags_t flags)
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
OMEGA_DEFINE_OID(Remoting,OID_InterProcessService,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");

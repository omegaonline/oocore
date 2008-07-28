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

#include "./WireProxy.h"
#include "./WireImpl.h"
#include "./StdObjectManager.h"

using namespace Omega;
using namespace OTL;

OOCore::WireProxy::WireProxy(uint32_t proxy_id, StdObjectManager* pManager) : 
	m_refcount(0), m_marshal_count(0), m_proxy_id(proxy_id), m_pManager(pManager)
{
	m_pManager->AddRef_Safe();
}

OOCore::WireProxy::~WireProxy()
{
	CallRemoteRelease();

	for (std::map<const guid_t,System::MetaInfo::IObject_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
	{
		i->second->Release_Safe();
	}
	m_pManager->RemoveProxy(m_proxy_id);
	m_pManager->Release_Safe();
}

void OOCore::WireProxy::Disconnect()
{
	// Force our marshal count to 0, cos the other end has gone
	m_marshal_count = 0;
}

System::MetaInfo::IObject_Safe* OOCore::WireProxy::UnmarshalInterface(System::MetaInfo::IMessage_Safe* pMessage, const guid_t& iid)
{
	try
	{
		// Up our marshal count early, because we are definitely attached to something!
		++m_marshal_count;

		System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IObject_Safe> ptrProxy;

		guid_t wire_iid;
		System::MetaInfo::IException_Safe* pSE = System::MetaInfo::wire_read(L"iid",pMessage,wire_iid);
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
			ptrProxy.attach(CreateWireProxy(wire_iid,this,m_pManager));
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

		System::MetaInfo::IObject_Safe* pQI = 0;
		if (iid == OMEGA_UUIDOF(IObject))
		{
			pQI = static_cast<System::MetaInfo::IWireProxy_Safe*>(this);
			pQI->AddRef_Safe();
		}
		else
		{
			pSE = ptrProxy->QueryInterface_Safe(&iid,&pQI);
			if (pSE)
				System::MetaInfo::throw_correct_exception(pSE);
			if (!pQI)
				throw INoInterfaceException::Create(iid);
		}
		return pQI;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}
}

bool OOCore::WireProxy::CallRemoteQI(const guid_t& iid)
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	pParamsOut->WriteStructStart(L"ipc_request",L"$ipc_request_type");

	WriteUInt32(L"$stub_id",pParamsOut,m_proxy_id);
	WriteGuid(L"$iid",pParamsOut,OMEGA_UUIDOF(IObject));
	WriteUInt32(L"$method_id",pParamsOut,1);
	WriteGuid(L"iid",pParamsOut,iid);

	pParamsOut->WriteStructEnd(L"ipc_request");

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = m_pManager->SendAndReceive(Remoting::Synchronous,pParamsOut,pParamsIn);
	if (pE)
		throw pE;

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	return ReadBoolean(L"$retval",ptrParamsIn);
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
			ptrProxy.attach(CreateWireProxy(*piid,this,m_pManager));
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
		return System::MetaInfo::return_safe_exception(ISystemException::Create(e,OMEGA_SOURCE_INFO));
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}

	return ptrProxy->QueryInterface_Safe(piid,ppS);
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireProxy::IsAlive_Safe(bool_t* pRet)
{
	*pRet = false;
	try
	{
		*pRet = m_pManager->IsAlive();
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
	return 0;
}

Remoting::IMessage* OOCore::WireProxy::CallRemoteStubMarshal(Remoting::IObjectManager* pObjectManager, const guid_t& iid)
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	pParamsOut->WriteStructStart(L"ipc_request",L"$ipc_request_type");

	WriteUInt32(L"$stub_id",pParamsOut,m_proxy_id);
	WriteGuid(L"$iid",pParamsOut,OMEGA_UUIDOF(IObject));
	WriteUInt32(L"$method_id",pParamsOut,2);
	WriteGuid(L"iid",pParamsOut,iid);

	m_pManager->DoMarshalChannel(pObjectManager,pParamsOut);

	pParamsOut->WriteStructEnd(L"ipc_request");

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = 0;
	try
	{
		pE = m_pManager->SendAndReceive(Remoting::Synchronous,pParamsOut,pParamsIn);
	}
	catch (IException* pE2)
	{
		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE2);

		ReadUInt32(L"$stub_id",pParamsOut);
		ReadGuid(L"$iid",pParamsOut);
		ReadUInt32(L"$method_id",pParamsOut);
		ReadGuid(L"iid",pParamsOut);
		m_pManager->ReleaseMarshalData(L"pObjectManager",pParamsOut,OMEGA_UUIDOF(System::MetaInfo::IWireManager),pObjectManager);

		throw ptrE.AddRef();
	}
	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	if (pE)
		throw pE;

	IObject* pReflect = 0;
	m_pManager->UnmarshalInterface(L"pReflect",ptrParamsIn,OMEGA_UUIDOF(Remoting::IMessage),pReflect);
	return static_cast<Remoting::IMessage*>(pReflect);
}

void OOCore::WireProxy::CallRemoteRelease()
{
	if (m_marshal_count == 0)
		return;

	try
	{
		ObjectPtr<Remoting::IMessage> pParamsOut;
		pParamsOut.Attach(m_pManager->CreateMessage());

		pParamsOut->WriteStructStart(L"ipc_request",L"$ipc_request_type");

		WriteUInt32(L"$stub_id",pParamsOut,m_proxy_id);
		WriteGuid(L"$iid",pParamsOut,OMEGA_UUIDOF(IObject));
		WriteUInt32(L"$method_id",pParamsOut,0);
		WriteUInt32(L"release_count",pParamsOut,m_marshal_count.value());

		pParamsOut->WriteStructEnd(L"ipc_request");
		
		Remoting::IMessage* pParamsIn = 0;
		IException* pE = m_pManager->SendAndReceive(Remoting::Synchronous,pParamsOut,pParamsIn);
		if (pE)
			pE->Release();

		if (pParamsIn)
			pParamsIn->Release();
	}
	catch (IException* pE)
	{
		pE->Release();
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireProxy::MarshalInterface_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IMessage_Safe* pMessage, const guid_t* piid, Remoting::MarshalFlags_t)
{
	// Tell the stub to expect incoming requests from a different channel...
	System::MetaInfo::IMessage_Safe* pReflect = 0;
	try
	{
		static_cast<Remoting::IMessage*&>(System::MetaInfo::marshal_info<Remoting::IMessage*&>::safe_type::coerce(&pReflect)) = CallRemoteStubMarshal(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(pObjectManager),*piid);
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}
	System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IMessage_Safe> ptrReflect(pReflect);

	return pObjectManager->MarshalInterface_Safe(L"pReflect",pMessage,&OMEGA_UUIDOF(Remoting::IMessage),ptrReflect);
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireProxy::ReleaseMarshalData_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class*, System::MetaInfo::IMessage_Safe*, const guid_t*, Remoting::MarshalFlags_t)
{
	return System::MetaInfo::return_safe_exception(Omega::ISystemException::Create(L"Cannot undo from here!"));
}

void OOCore::WireProxyMarshalFactory::UnmarshalInterface(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	pObject = 0;

	// Unmarshal the reflect package
	IObject* pReflect = 0;
	pObjectManager->UnmarshalInterface(L"pReflect",pMessage,OMEGA_UUIDOF(Remoting::IMessage),pReflect);
	ObjectPtr<Remoting::IMessage> ptrReflect;
	ptrReflect.Attach(static_cast<Remoting::IMessage*>(pReflect));

	// Unmarshal the manager
	IObject* pChannel = 0;
	pObjectManager->UnmarshalInterface(L"m_ptrChannel",ptrReflect,OMEGA_UUIDOF(Remoting::IChannelEx),pChannel);
	ObjectPtr<Remoting::IChannelEx> ptrChannel;
	ptrChannel.Attach(static_cast<Remoting::IChannelEx*>(pChannel));

	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());

	// Unmarshal the new proxy on the new manager
	ptrOM->UnmarshalInterface(L"stub",ptrReflect,iid,pObject);
}

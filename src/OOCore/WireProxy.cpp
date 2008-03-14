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

System::MetaInfo::IObject_Safe* OOCore::WireProxy::UnmarshalInterface(System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t& iid)
{
	try
	{
		// Up our marshal count early, because we are definitely attached to something!
		++m_marshal_count;

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
	ObjectPtr<Serialize::IFormattedStream> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateOutputStream());

	pParamsOut->WriteUInt32(m_proxy_id);
	pParamsOut->WriteGuid(OMEGA_UUIDOF(IObject));
	pParamsOut->WriteUInt32(1);
	pParamsOut->WriteGuid(iid);

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

uint32_t OOCore::WireProxy::CallRemoteStubMarshal(Remoting::IObjectManager* pObjectManager, const guid_t& iid)
{
	ObjectPtr<Serialize::IFormattedStream> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateOutputStream());

	pParamsOut->WriteUInt32(m_proxy_id);
	pParamsOut->WriteGuid(OMEGA_UUIDOF(IObject));
	pParamsOut->WriteUInt32(2);

	pParamsOut->WriteGuid(iid);
	m_pManager->MarshalInterface(pParamsOut,OMEGA_UUIDOF(System::MetaInfo::IWireManager),pObjectManager);

	Serialize::IFormattedStream* pParamsIn = 0;
	IException* pE = 0;
	try
	{
		pE = m_pManager->SendAndReceive(0,pParamsOut,pParamsIn,0);
	}
	catch (IException* pE2)
	{
		ObjectPtr<IException> ptrE;
		ptrE.Attach(pE2);

		pParamsOut->ReadUInt32();
		pParamsOut->ReadGuid();
		pParamsOut->ReadUInt32();
		pParamsOut->ReadGuid();
		m_pManager->ReleaseMarshalData(pParamsOut,OMEGA_UUIDOF(Remoting::IObjectManager),pObjectManager);

		throw ptrE.AddRef();
	}
	ObjectPtr<Serialize::IFormattedStream> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	if (pE)
		throw pE;

	// Read the new stub's key
	uint32_t ret = ptrParamsIn->ReadUInt32();
	ptrParamsIn->ReadGuid();
	return ret;
}

void OOCore::WireProxy::CallRemoteRelease()
{
	if (m_marshal_count == 0)
		return;

	try
	{
		ObjectPtr<Serialize::IFormattedStream> pParamsOut;
		pParamsOut.Attach(m_pManager->CreateOutputStream());

		pParamsOut->WriteUInt32(m_proxy_id);
		pParamsOut->WriteGuid(OMEGA_UUIDOF(IObject));
		pParamsOut->WriteUInt32(0);

		pParamsOut->WriteUInt32(m_marshal_count.value());
		
		Serialize::IFormattedStream* pParamsIn = 0;
		IException* pE = m_pManager->SendAndReceive(0,pParamsOut,pParamsIn,0);
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

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireProxy::MarshalInterface_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t* piid, Remoting::MarshalFlags_t)
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

	size_t undo_count = 0;

	// Marshal our own manager out...
	System::MetaInfo::IException_Safe* pSE = pObjectManager->MarshalInterface_Safe(pStream,&OMEGA_UUIDOF(System::MetaInfo::IWireManager),static_cast<System::MetaInfo::IWireManager_Safe*>(m_pManager));
	if (pSE)
		return pSE;
	++undo_count;

	// The following format is the same as StdObjectManager::UnmarshalInterface...
	pSE = pStream->WriteByte_Safe(1);
	if (pSE)
		goto Cleanup;
	++undo_count;

	pSE = pStream->WriteUInt32_Safe(new_key);
	if (pSE)
		goto Cleanup;
	++undo_count;

	pSE = System::MetaInfo::wire_write(pStream,*piid);
	if (!pSE)
		return 0;

Cleanup:
	System::MetaInfo::IException_Safe* pSE2 = 0;
	if (undo_count > 0)
	{
		pSE2 = pObjectManager->ReleaseMarshalData_Safe(pStream,&OMEGA_UUIDOF(System::MetaInfo::IWireManager),static_cast<System::MetaInfo::IWireManager_Safe*>(m_pManager));
		if (pSE2)
		{
			pSE->Release_Safe();
			return pSE2;
		}
	}

	if (undo_count > 1)
	{
		byte_t v;
		pSE2 = pStream->ReadByte_Safe(&v);
		if (pSE2)
		{
			pSE->Release_Safe();
			return pSE2;
		}
	}

	if (undo_count > 2)
	{
		uint32_t v;
		pSE2 = pStream->ReadUInt32_Safe(&v);
		if (pSE2)
		{
			pSE->Release_Safe();
			return pSE2;
		}
	}

	return pSE;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireProxy::ReleaseMarshalData_Safe(System::MetaInfo::interface_info<Remoting::IObjectManager>::safe_class* pObjectManager, System::MetaInfo::IFormattedStream_Safe* pStream, const guid_t*, Remoting::MarshalFlags_t)
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

void OOCore::WireProxyMarshalFactory::UnmarshalInterface(Remoting::IObjectManager* pObjectManager, Serialize::IFormattedStream* pStream, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	IObject* pOM = 0;
	pObjectManager->UnmarshalInterface(pStream,OMEGA_UUIDOF(Remoting::IObjectManager),pOM);

	OTL::ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(static_cast<Remoting::IObjectManager*>(pOM));

	ptrOM->UnmarshalInterface(pStream,iid,pObject);
}

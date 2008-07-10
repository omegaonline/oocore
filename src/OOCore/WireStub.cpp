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

#include "./WireStub.h"
#include "./WireImpl.h"
#include "./StdObjectManager.h"

using namespace Omega;
using namespace OTL;

OOCore::WireStub::WireStub(System::MetaInfo::IObject_Safe* pObjS, uint32_t stub_id, StdObjectManager* pManager) : 
	m_refcount(0), m_marshal_count(0), m_stub_id(stub_id), m_pObjS(pObjS), m_pManager(pManager)
{
	m_pObjS->AddRef_Safe();
}

OOCore::WireStub::~WireStub()
{
	for (std::map<const guid_t,System::MetaInfo::IWireStub_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
	{
		i->second->Release_Safe();
	}
	m_pObjS->Release_Safe();
}

System::MetaInfo::IException_Safe* OOCore::WireStub::MarshalInterface(System::MetaInfo::IMessage_Safe* pMessage, const guid_t& iid)
{
	try
	{
		System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IWireStub_Safe> ptrStub(FindStub(iid));
	}
	catch (IException* pE)
	{
		return System::MetaInfo::return_safe_exception(pE);
	}

	System::MetaInfo::IException_Safe* pSE = System::MetaInfo::wire_write(L"id",pMessage,m_stub_id);
	if (pSE)
		return pSE;

	pSE = System::MetaInfo::wire_write(L"iid",pMessage,iid);
	if (pSE)
	{
		uint32_t v;
		System::MetaInfo::IException_Safe* pSE2 = System::MetaInfo::wire_read(L"id",pMessage,v);
		if (pSE2)
		{
			pSE->Release_Safe();
			return pSE2;
		}
	}

	if (!pSE)
		++m_marshal_count;

	return pSE;
}

System::MetaInfo::IException_Safe* OOCore::WireStub::ReleaseMarshalData(System::MetaInfo::IMessage_Safe* pMessage, const guid_t&)
{
	uint32_t v;
	System::MetaInfo::IException_Safe* pSE = System::MetaInfo::wire_read(L"id",pMessage,v);
	if (!pSE)
	{
		guid_t iid;
		pSE = System::MetaInfo::wire_read(L"iid",pMessage,iid);
	}

	// Deref safely
	RemoteRelease_Safe(1);

	return pSE;
}

System::MetaInfo::IWireStub_Safe* OOCore::WireStub::LookupStub(Remoting::IMessage* pMessage)
{
	return FindStub(ReadGuid(L"$iid",pMessage));
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
			ptrStub.attach(CreateWireStub(iid,this,m_pManager,ptrQI));
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
		OMEGA_THROW(e);
	}
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireStub::RemoteRelease_Safe(uint32_t release_count)
{
	m_marshal_count -= release_count;
	if (m_marshal_count == 0)
	{
		try
		{
			m_pManager->RemoveStub(m_stub_id);
		}
		catch (IException* pE)
		{
			return System::MetaInfo::return_safe_exception(pE);
		}
	}

	return 0;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireStub::SupportsInterface_Safe(bool_t* pbSupports, const guid_t* piid)
{
	System::MetaInfo::IObject_Safe* p;
	System::MetaInfo::IException_Safe* pSE = m_pObjS->QueryInterface_Safe(piid,&p);
	if (pSE)
		return pSE;

	System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IObject_Safe> ptr(p);
	*pbSupports = (p != 0);
	return 0;
}

System::MetaInfo::IException_Safe* OMEGA_CALL OOCore::WireStub::MarshalStub_Safe(System::MetaInfo::IMessage_Safe* pParamsIn, System::MetaInfo::IMessage_Safe* pParamsOut)
{
	System::MetaInfo::marshal_info<guid_t>::wire_type::type iid;
	System::MetaInfo::IException_Safe* pSE = System::MetaInfo::marshal_info<guid_t>::wire_type::read(L"iid",m_pManager,pParamsIn,iid);
	if (pSE)
		return pSE;

	System::MetaInfo::IObject_Safe* pObj = 0;
	pSE = m_pManager->UnmarshalInterface_Safe(L"pObjectManager",pParamsIn,&OMEGA_UUIDOF(IObject),&pObj);
	if (pSE)
		return pSE;

	System::MetaInfo::IObject_Safe* pMO = 0;
	pSE = pObj->QueryInterface_Safe(&OMEGA_UUIDOF(System::MetaInfo::IWireManager),&pMO);
	pObj->Release_Safe();
	if (pSE)
		return pSE;
	System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IWireManager_Safe> ptrManager(static_cast<System::MetaInfo::IWireManager_Safe*>(pMO));

	System::MetaInfo::IMessage_Safe* pMessage = 0;
	pSE = ptrManager->ReflectChannel_Safe(&pMessage);
	if (pSE)
		return pSE;
	System::MetaInfo::auto_iface_safe_ptr<System::MetaInfo::IMessage_Safe> ptrMessage(pMessage);

	pSE = ptrManager->MarshalInterface_Safe(L"stub",pMessage,&iid,m_pObjS);
	if (pSE)
		return pSE;
		
	pSE = m_pManager->MarshalInterface_Safe(L"pReflect",pParamsOut,&OMEGA_UUIDOF(Remoting::IMessage),pMessage);
	if (pSE)
	{
		System::MetaInfo::IException_Safe* pSE2 = ptrManager->ReleaseMarshalData_Safe(L"stub",pMessage,&iid,m_pObjS);
		if (pSE2)
		{
			pSE->Release_Safe();
			return pSE2;
		}
	}
	
	return pSE;
}

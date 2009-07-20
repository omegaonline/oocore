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

#include "WireProxy.h"
#include "StdObjectManager.h"
#include "WireImpl.h"

using namespace Omega;
using namespace OTL;

OOCore::Proxy::Proxy() :
	m_proxy_id(0)
{
	static const System::MetaInfo::vtable_info<System::IProxy>::type proxy_vt =
	{
		{
			&AddRef_Safe,
			&Release_Safe,
			&QueryInterface_Safe,
			&GetBaseShim_Safe,
			&Pin_Safe,
			&Unpin_Safe
		},
		&WriteKey_Safe,
		&UnpackKey_Safe,
		&GetMarshaller_Safe,
		&IsAlive_Safe
	};

	m_proxy_shim.m_vtable = &proxy_vt;
	m_proxy_shim.m_stub = this;
	m_proxy_shim.m_iid = &OMEGA_GUIDOF(System::IProxy);

	static const System::MetaInfo::vtable_info<Remoting::IMarshal>::type marshal_vt =
	{
		{
			&AddRef_Safe,
			&Release_Safe,
			&QueryInterface_Safe,
			&GetBaseShim_Safe,
			&Pin_Safe,
			&Unpin_Safe
		},
		&GetUnmarshalFactoryOID_Safe,
		&MarshalInterface_Safe,
		&ReleaseMarshalData_Safe
	};

	m_marshal_shim.m_vtable = &marshal_vt;
	m_marshal_shim.m_stub = this;
	m_marshal_shim.m_iid = &OMEGA_GUIDOF(Remoting::IMarshal);
}

OOCore::Proxy::~Proxy()
{
	CallRemoteRelease();

	m_pManager->RemoveProxy(m_proxy_id);
	static_cast<IStdObjectManager*>(m_pManager)->Release();
}

void OOCore::Proxy::init(uint32_t proxy_id, StdObjectManager* pManager)
{
	m_proxy_id = proxy_id;
	m_pManager = pManager;
	static_cast<IStdObjectManager*>(m_pManager)->AddRef();
}

void OOCore::Proxy::Disconnect()
{
	// Force our marshal count to 0, cos the other end has gone
	m_marshal_count = 0;
}

System::IMarshaller* OOCore::Proxy::GetMarshaller()
{
	System::IMarshaller* pRet = static_cast<System::IMarshaller*>(m_pManager);
	pRet->AddRef();
	return pRet;
}

bool_t OOCore::Proxy::IsAlive()
{
	return m_pManager->IsAlive();
}

OOCore::Proxy::WireProxyShim OOCore::Proxy::FindShim(const guid_t& iid, bool bCheckRemote)
{
	OOBase::Guard<OOBase::SpinLock> guard(m_lock);

	// See if we have a proxy for this interface already...
	WireProxyShim ptrProxy;

	try
	{
		std::map<const guid_t,WireProxyShim>::iterator i=m_iid_map.find(iid);
		if (i != m_iid_map.end())
			ptrProxy = i->second;
		else
		{
			// See if any known interface supports the new interface
			for (i=m_iid_map.begin();i!=m_iid_map.end();++i)
			{
				if (i->second.IsDerived(iid))
				{
					ptrProxy = i->second;
					break;
				}
			}
		}
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	if (!ptrProxy)
	{
		if (bCheckRemote)
		{
			// Send a packet to the other end to see if the stub supports the interface
			if (!CallRemoteQI(iid))
				return ptrProxy;
		}

		// Create a new proxy for this interface
		ptrProxy.Attach(OOCore::CreateProxy(iid,this));
	}

	try
	{
		std::pair<std::map<const guid_t,WireProxyShim>::iterator,bool> p=m_iid_map.insert(std::map<const guid_t,WireProxyShim>::value_type(iid,ptrProxy));
		if (!p.second)
			ptrProxy = p.first->second;
	}
	catch (std::exception& e)
	{
		OMEGA_THROW(e);
	}

	return ptrProxy;
}

IObject* OOCore::Proxy::UnmarshalInterface(Remoting::IMessage* pMessage, const guid_t& iid)
{
	// Up our marshal count early, because we are definitely attached to something!
	++m_marshal_count;

	guid_t wire_iid;
	System::MetaInfo::wire_read(L"iid",pMessage,wire_iid);

	if (wire_iid == OMEGA_GUIDOF(IObject))
	{
		Internal_AddRef();
		return static_cast<System::IProxy*>(this);
	}

	WireProxyShim ptrProxy = FindShim(wire_iid,false);
	if (!ptrProxy)
		ptrProxy = FindShim(iid,false);

	if (!ptrProxy)
		OMEGA_THROW(L"Failed to find correct shim for wire_iid");

	return System::MetaInfo::create_proxy(ptrProxy.GetShim());
}

void OOCore::Proxy::WriteStubInfo(Remoting::IMessage* pMessage, uint32_t method_id)
{
	pMessage->WriteStructStart(L"ipc_request",L"$ipc_request_type");

	System::MetaInfo::wire_write(L"$stub_id",pMessage,m_proxy_id);
	System::MetaInfo::wire_write(L"$iid",pMessage,OMEGA_GUIDOF(IObject));
	System::MetaInfo::wire_write(L"$method_id",pMessage,method_id);
}

void OOCore::Proxy::ReadStubInfo(Remoting::IMessage* pMessage)
{
	pMessage->ReadStructStart(L"ipc_request",L"$ipc_request_type");

	uint32_t l;
	guid_t m;
	System::MetaInfo::wire_read(L"$stub_id",pMessage,l);
	System::MetaInfo::wire_read(L"$iid",pMessage,m);
	System::MetaInfo::wire_read(L"$method_id",pMessage,l);
}

bool OOCore::Proxy::CallRemoteQI(const guid_t& iid)
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	WriteStubInfo(pParamsOut,1);

	System::MetaInfo::wire_write(L"iid",pParamsOut,iid);

	pParamsOut->WriteStructEnd(L"ipc_request");

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	if (pE)
		throw pE;

	bool_t retval;
	System::MetaInfo::wire_read(L"$retval",ptrParamsIn,retval);
	return retval;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::GetStub(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(IObject) ||
		iid == OMEGA_GUIDOF(System::IProxy))
	{
		Internal_AddRef();
		return &m_proxy_shim;
	}
	
	if (iid == OMEGA_GUIDOF(Remoting::IMarshal))
	{
		Internal_AddRef();
		return &m_marshal_shim;
	}
	
	const System::MetaInfo::SafeShim* shim = 0;
	WireProxyShim ptrProxy = FindShim(iid,true);
	if (ptrProxy)
		shim = ptrProxy.GetShim();
	
	if (shim)
	{
		const System::MetaInfo::SafeShim* except = static_cast<const System::MetaInfo::IObject_Safe_VTable*>(shim->m_vtable)->pfnAddRef_Safe(shim);
		if (except)
			throw_correct_exception(except);
	}

	return shim;
}

IObject* OOCore::Proxy::QI(const guid_t& iid)
{
	WireProxyShim ptrProxy = FindShim(iid,true);
	if (!ptrProxy)
		return 0;

	Internal_AddRef();

	return System::MetaInfo::create_proxy(ptrProxy.GetShim());
}

Remoting::IMessage* OOCore::Proxy::CallRemoteStubMarshal(Remoting::IObjectManager* pObjectManager, const guid_t& iid)
{
	ObjectPtr<Remoting::IMessage> pParamsOut;
	pParamsOut.Attach(m_pManager->CreateMessage());

	WriteStubInfo(pParamsOut,2);

	System::MetaInfo::wire_write(L"iid",pParamsOut,iid);

	Remoting::IMessage* pParamsIn = 0;
	IException* pE = 0;

	try
	{
		m_pManager->DoMarshalChannel(pObjectManager,pParamsOut);

		pParamsOut->WriteStructEnd(L"ipc_request");

		pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);
	}
	catch (...)
	{
		ReadStubInfo(pParamsOut);

		guid_t m;
		System::MetaInfo::wire_read(L"iid",pParamsOut,m);

		void* TODO; // Release marshal data for channel
		//m_pManager->ReleaseMarshalData(L"pObjectManager",pParamsOut,OMEGA_GUIDOF(System::IMarshaller),pObjectManager);

		throw;
	}

	ObjectPtr<Remoting::IMessage> ptrParamsIn;
	ptrParamsIn.Attach(pParamsIn);

	if (pE)
		throw pE;

	IObject* pUI = 0;
	m_pManager->UnmarshalInterface(L"pReflect",ptrParamsIn,OMEGA_GUIDOF(Remoting::IMessage),pUI);
	return static_cast<Remoting::IMessage*>(pUI);
}

void OOCore::Proxy::CallRemoteRelease()
{
	if (m_marshal_count == 0)
		return;

	try
	{
		ObjectPtr<Remoting::IMessage> pParamsOut;
		pParamsOut.Attach(m_pManager->CreateMessage());

		WriteStubInfo(pParamsOut,0);

		System::MetaInfo::wire_write(L"release_count",pParamsOut,static_cast<uint32_t>(m_marshal_count.value()));

		pParamsOut->WriteStructEnd(L"ipc_request");

		Remoting::IMessage* pParamsIn = 0;
		IException* pE = m_pManager->SendAndReceive(TypeInfo::Synchronous,pParamsOut,pParamsIn);

		if (pParamsIn)
			pParamsIn->Release();

		if (pE)
			pE->Release();
	}
	catch (IException* pE)
	{
		pE->Release();
	}
}

void OOCore::Proxy::MarshalInterface(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t)
{
	// Tell the stub to expect incoming requests from a different channel...
	ObjectPtr<Remoting::IMessage> ptrReflect;
	ptrReflect.Attach(CallRemoteStubMarshal(pObjectManager,iid));

	return pObjectManager->MarshalInterface(L"pReflect",pMessage,OMEGA_GUIDOF(Remoting::IMessage),ptrReflect);
}

void OOCore::Proxy::ReleaseMarshalData(Remoting::IObjectManager*, Remoting::IMessage*, const guid_t&, Remoting::MarshalFlags_t)
{
	// How do we undo this?
	void* TODO;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::AddRef_Safe(const System::MetaInfo::SafeShim* shim)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		//printf("Safe ");
		static_cast<Proxy*>(shim->m_stub)->Internal_AddRef();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::Release_Safe(const System::MetaInfo::SafeShim* shim)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		//printf("Safe ");
		static_cast<Proxy*>(shim->m_stub)->Internal_Release();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::QueryInterface_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim** retval, const guid_t* iid)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		//printf("Safe %p QI for %ls\n",static_cast<Proxy*>(shim->m_stub),iid->ToString().c_str());
		static_cast<IObject*&>(System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(retval,iid)) = static_cast<Proxy*>(shim->m_stub)->Internal_QueryInterface(*iid,getQIEntries());
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::GetBaseShim_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim** retval)
{
	*retval = &static_cast<Proxy*>(shim->m_stub)->m_proxy_shim;
	return 0;	
}

const System::MetaInfo::SafeShim* OOCore::Proxy::Pin_Safe(const System::MetaInfo::SafeShim* shim)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->Pin();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::Unpin_Safe(const System::MetaInfo::SafeShim* shim)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->Unpin();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::WriteKey_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim* pMessage)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->WriteKey(System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage));
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::UnpackKey_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim* pMessage)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->UnpackKey(System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage));
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::GetMarshaller_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim** retval)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<System::IMarshaller*&>(System::MetaInfo::marshal_info<System::IMarshaller*&>::safe_type::coerce(retval)) = static_cast<Proxy*>(shim->m_stub)->GetMarshaller();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::IsAlive_Safe(const System::MetaInfo::SafeShim* shim, int* retval)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<bool_t&>(System::MetaInfo::marshal_info<bool_t&>::safe_type::coerce(retval)) = static_cast<Proxy*>(shim->m_stub)->IsAlive();
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::GetUnmarshalFactoryOID_Safe(const System::MetaInfo::SafeShim* shim, guid_t* retval, const guid_t* piid, Remoting::MarshalFlags_t flags)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<guid_t&>(System::MetaInfo::marshal_info<guid_t&>::safe_type::coerce(retval)) = static_cast<Proxy*>(shim->m_stub)->GetUnmarshalFactoryOID(*piid,flags);
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::MarshalInterface_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim* pObjectManager, const System::MetaInfo::SafeShim* pMessage, const guid_t* iid, Remoting::MarshalFlags_t flags)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->MarshalInterface(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(pObjectManager),System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage),*iid,flags);
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

const System::MetaInfo::SafeShim* OOCore::Proxy::ReleaseMarshalData_Safe(const System::MetaInfo::SafeShim* shim, const System::MetaInfo::SafeShim* pObjectManager, const System::MetaInfo::SafeShim* pMessage, const guid_t* iid, Remoting::MarshalFlags_t flags)
{
	const System::MetaInfo::SafeShim* except = 0;
	try
	{
		static_cast<Proxy*>(shim->m_stub)->ReleaseMarshalData(System::MetaInfo::marshal_info<Remoting::IObjectManager*>::safe_type::coerce(pObjectManager),System::MetaInfo::marshal_info<Remoting::IMessage*>::safe_type::coerce(pMessage),*iid,flags);
	}
	catch (IException* pE)
	{
		except = System::MetaInfo::return_safe_exception(pE);
	}
	return except;
}

void OOCore::ProxyMarshalFactory::UnmarshalInterface(Remoting::IObjectManager* pObjectManager, Remoting::IMessage* pMessage, const guid_t& iid, Remoting::MarshalFlags_t, IObject*& pObject)
{
	// Unmarshal the reflect package
	IObject* pUI = 0;
	pObjectManager->UnmarshalInterface(L"pReflect",pMessage,OMEGA_GUIDOF(Remoting::IMessage),pUI);
	ObjectPtr<Remoting::IMessage> ptrReflect;
	ptrReflect.Attach(static_cast<Remoting::IMessage*>(pUI));

	// Unmarshal the manager
	pUI = 0;
	pObjectManager->UnmarshalInterface(L"m_ptrChannel",ptrReflect,OMEGA_GUIDOF(Remoting::IChannel),pUI);
	ObjectPtr<Remoting::IChannel> ptrChannel;
	ptrChannel.Attach(static_cast<Remoting::IChannel*>(pUI));

	ObjectPtr<Remoting::IObjectManager> ptrOM;
	ptrOM.Attach(ptrChannel->GetObjectManager());

	// Unmarshal the new proxy on the new manager
	ptrOM->UnmarshalInterface(L"stub",ptrReflect,iid,pObject);
}

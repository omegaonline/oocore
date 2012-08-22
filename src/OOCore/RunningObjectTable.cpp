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

#include "Activation.h"
#include "WireProxy.h"
#include "Channel.h"
#include "Exception.h"
#include "Activation.h"
#include "Compartment.h"

using namespace Omega;
using namespace OTL;

namespace
{
	// The instance wide LocalROT instance
	class LocalROT :
			public Activation::IRunningObjectTable,
			public Activation::IRunningObjectTableNotify,
			public Notify::INotifier
	{
	public:
		ObjectPtr<OOCore::IInterProcessService> GetIPS(bool bThrow);
		ObjectPtr<Registry::IKey> GetRootKey();
		void RegisterIPS(OOCore::IInterProcessService* pIPS);
		bool IsHosted();
		void RevokeIPS();

	private:
		friend class OOBase::Singleton<LocalROT,OOCore::DLL>;

		LocalROT();
		LocalROT(const LocalROT&);
		LocalROT& operator = (const LocalROT&);

		OOBase::RWMutex m_lock;
		uint32_t        m_notify_cookie;
		bool           m_hosted_by_ooserver;

		ObjectPtr<OOCore::IInterProcessService>    m_ptrIPS;
		ObjectPtr<Registry::IKey>                  m_ptrReg;
		ObjectPtr<Activation::IRunningObjectTable> m_ptrROT;

		struct Info
		{
			string_t                    m_oid;
			ObjectPtr<IObject>          m_ptrObject;
			Activation::RegisterFlags_t m_flags;
			uint32_t                    m_rot_cookie;
		};
		OOBase::HandleTable<uint32_t,Info> m_mapServicesByCookie;
		OOBase::Table<string_t,uint32_t>   m_mapServicesByOid;

		OOBase::HandleTable<uint32_t,ObjectPtr<Activation::IRunningObjectTableNotify> > m_mapNotify;

		ObjectPtr<Activation::IRunningObjectTable> GetROT(bool bThrow);

	// IObject members
	public:
		void AddRef()
		{
			GetModule()->IncLockCount();
		}

		void Release()
		{
			GetModule()->DecLockCount();
		}

		IObject* QueryInterface(const guid_t& iid);

	// IRunningObjectTable members
	public:
		uint32_t RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags);
		void GetObject(const any_t& oid, const guid_t& iid, IObject*& pObject);
		void RevokeObject(uint32_t cookie);

	// IRunningObjectTableNotify members
	public:
		void OnRegisterObject(const any_t& oid, Activation::RegisterFlags_t flags);
		void OnRevokeObject(const any_t& oid, Activation::RegisterFlags_t flags);

	// INotifier members
	public:
		uint32_t RegisterNotify(const guid_t& iid, IObject* pObject);
		void UnregisterNotify(const guid_t& iid, uint32_t cookie);
		iid_list_t ListNotifyInterfaces();
	};

	typedef OOBase::Singleton<LocalROT,OOCore::DLL> LOCAL_ROT;
}
template class OOBase::Singleton<LocalROT,OOCore::DLL>;

namespace OOCore
{
	class RunningObjectTableFactory :
			public ObjectFactoryBase<&Activation::OID_RunningObjectTable,Activation::ProcessScope>
	{
	// IObjectFactory members
	public:
		void CreateInstance(const guid_t& iid, IObject*& pObject);
	};

	class RegistryFactory :
			public ObjectFactoryBase<&Registry::OID_Registry,Activation::ProcessScope>
	{
	// IObjectFactory members
	public:
		void CreateInstance(const guid_t& iid, IObject*& pObject);
	};
}

namespace OTL
{
	// The following is an expansion of BEGIN_LIBRARY_OBJECT_MAP
	// We don't use the macro as we override some behaviours
	namespace Module
	{
		class OOCore_ModuleImpl : public ModuleBase
		{
		public:
			void RegisterObjectFactories(Activation::IRunningObjectTable* pROT, bool bPrivate);
			void UnregisterObjectFactories(Activation::IRunningObjectTable* pROT, bool bPrivate);

			template <typename T>
			struct Creator
			{
				static IObject* Create(const guid_t& iid)
				{
					ObjectPtr<NoLockObjectImpl<T> > ptr = NoLockObjectImpl<T>::CreateInstance();
					IObject* pObject = ptr->QueryInterface(iid);
					if (!pObject)
						throw OOCore_INotFoundException_MissingIID(iid);
					return pObject;
				}
			};

		private:
			ModuleBase::CreatorEntry* getCreatorEntries()
			{
				static ModuleBase::CreatorEntry CreatorEntries[] =
				{
					OBJECT_MAP_ENTRY(OOCore::CDRMessageMarshalFactory)
					OBJECT_MAP_ENTRY(OOCore::ProxyMarshalFactory)
					OBJECT_MAP_FACTORY_ENTRY(OOCore::RunningObjectTableFactory)
					OBJECT_MAP_FACTORY_ENTRY(OOCore::RegistryFactory)
					OBJECT_MAP_ENTRY(OOCore::SystemExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::InternalExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::NotFoundExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::AccessDeniedExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::AlreadyExistsExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::TimeoutExceptionMarshalFactoryImpl)
					OBJECT_MAP_ENTRY(OOCore::ChannelClosedExceptionMarshalFactoryImpl)
					{ 0,0,0,0 }
				};
				return CreatorEntries;
			}

			ModuleBase::CreatorEntry* getCreatorEntries_private()
			{
				static ModuleBase::CreatorEntry CreatorEntries[] =
				{
					OBJECT_MAP_ENTRY(OOCore::ChannelMarshalFactory)
					OBJECT_MAP_FACTORY_ENTRY(OOCore::CompartmentFactory)
					{ 0,0,0,0 }
				};
				return CreatorEntries;
			}
		};

		OMEGA_PRIVATE_FN_DECL(Module::OOCore_ModuleImpl*,GetModule())
		{
			return OOBase::Singleton<Module::OOCore_ModuleImpl,OOCore::DLL>::instance_ptr();
		}

		OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)()
		{
			return OMEGA_PRIVATE_FN_CALL(GetModule)();
		}
	}
}

LocalROT::LocalROT() :
		m_notify_cookie(0),
		m_hosted_by_ooserver(false),
		m_mapServicesByCookie(1),
		m_mapNotify(1)
{
	Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->RegisterObjectFactories(this,false);
}

IObject* LocalROT::QueryInterface(const guid_t& iid)
{
	if (iid == OMEGA_GUIDOF(IObject) ||
			iid == OMEGA_GUIDOF(Activation::IRunningObjectTable))
	{
		AddRef();
		return static_cast<Activation::IRunningObjectTable*>(this);
	}
	else if (iid == OMEGA_GUIDOF(Activation::IRunningObjectTableNotify))
	{
		AddRef();
		return static_cast<Activation::IRunningObjectTableNotify*>(this);
	}
	else if (iid == OMEGA_GUIDOF(Notify::INotifier))
	{
		AddRef();
		return static_cast<Notify::INotifier*>(this);
	}

	return NULL;
}

ObjectPtr<OOCore::IInterProcessService> LocalROT::GetIPS(bool bThrow)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
	
	if (bThrow && !m_ptrIPS)
		throw IInternalException::Create(OOCore::get_text("Omega::Initialize not called"),"OOCore");

	return m_ptrIPS;
}

ObjectPtr<Activation::IRunningObjectTable> LocalROT::GetROT(bool bThrow)
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	if (bThrow && !m_ptrROT)
		throw IInternalException::Create(OOCore::get_text("Omega::Initialize not called"),"OOCore");

	return m_ptrROT;
}

ObjectPtr<Registry::IKey> LocalROT::GetRootKey()
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	if (!m_ptrReg)
		throw IInternalException::Create(OOCore::get_text("Omega::Initialize not called"),"OOCore");

	return m_ptrReg;
}

bool LocalROT::IsHosted()
{
	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);

	return m_hosted_by_ooserver;
}

void LocalROT::RegisterIPS(OOCore::IInterProcessService* pIPS)
{
	if (!pIPS)
		OMEGA_THROW("Null IPS in RegisterIPS");

	// Stash passed in values
	m_ptrIPS = pIPS;
	m_ptrIPS.AddRef();

	m_ptrROT = m_ptrIPS->GetRunningObjectTable();
	m_ptrReg = m_ptrIPS->GetRegistry();

	ObjectPtr<Notify::INotifier> ptrNotify = m_ptrROT.QueryInterface<Notify::INotifier>();
	m_notify_cookie = ptrNotify->RegisterNotify(OMEGA_GUIDOF(Activation::IRunningObjectTableNotify),static_cast<Activation::IRunningObjectTableNotify*>(this));

	ObjectPtr<Remoting::IProxy> ptrProxy = Remoting::GetProxy(pIPS);
	if (!ptrProxy)
		m_hosted_by_ooserver = true;
}

void LocalROT::RevokeIPS()
{
	if (m_notify_cookie)
	{
		ObjectPtr<Notify::INotifier> ptrNotify = m_ptrROT.QueryInterface<Notify::INotifier>();
		if (ptrNotify)
			ptrNotify->UnregisterNotify(OMEGA_GUIDOF(Activation::IRunningObjectTableNotify),m_notify_cookie);
	}

	m_ptrReg.Release();
	m_ptrROT.Release();
	m_ptrIPS.Release();
}

uint32_t LocalROT::RegisterObject(const any_t& oid, IObject* pObject, Activation::RegisterFlags_t flags)
{
	if (!pObject)
		throw IInternalException::Create(OOCore::get_text("NULL object"),"IRunningObjectTable::RegisterObject");

	// Check for user registration
	Activation::RegisterFlags_t scope = (flags & 0xF);
	if (!scope)
		throw IInternalException::Create(OOCore::get_text("Invalid flags"),"IRunningObjectTable::RegisterObject");

	// Create a new info struct
	Info info;
	info.m_oid = oid.cast<string_t>();
	info.m_flags = flags;
	info.m_ptrObject = pObject;
	info.m_ptrObject.AddRef();
	info.m_rot_cookie = 0;

	if (scope & ~Activation::ProcessScope)
	{
		// Register in ROT
		info.m_rot_cookie = GetROT(true)->RegisterObject(oid,pObject,flags);
	}

	OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
	uint32_t nCookie = 0;

	try
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		// Check if we have someone registered already
		for (size_t i=m_mapServicesByOid.find_first(info.m_oid); i<m_mapServicesByOid.size() && *m_mapServicesByOid.key_at(i)==info.m_oid; ++i)
		{
			Info* pInfo = m_mapServicesByCookie.find(*m_mapServicesByOid.at(i));
			if (pInfo)
			{
				// Check its still alive...
				if (!Remoting::IsAlive(pInfo->m_ptrObject))
					revoke_list.push(*m_mapServicesByOid.at(i));
				else
				{
					if (!(pInfo->m_flags & Activation::MultipleRegistration) || pInfo->m_flags == flags)
						throw IAlreadyExistsException::Create(OOCore::get_text("The OID {0} has already been registered") % oid);
				}
			}
		}

		int err = m_mapServicesByCookie.insert(info,nCookie,1,0xFFFFFFFF);
		if (err == 0)
		{
			err = m_mapServicesByOid.insert(info.m_oid,nCookie);
			if (err != 0)
				m_mapServicesByCookie.remove(nCookie);
		}
		if (err)
			OMEGA_THROW(err);
	}
	catch (...)
	{
		if (info.m_rot_cookie)
			GetROT(true)->RevokeObject(info.m_rot_cookie);

		throw;
	}
		
	// Revoke the revoke_list BEFORE we notify of the new entry
	for (uint32_t i = 0;revoke_list.pop(&i);)
		RevokeObject(i);

	if (!info.m_rot_cookie)
		OnRegisterObject(info.m_oid,info.m_flags);
		
	return nCookie;
}

void LocalROT::GetObject(const any_t& oid, const guid_t& iid, IObject*& pObject)
{
	OOBase::Stack<uint32_t,OOBase::LocalAllocator> revoke_list;
	string_t strOid = oid.cast<string_t>();

	OOBase::ReadGuard<OOBase::RWMutex> guard(m_lock);
	
	if (oid == OOCore::OID_InterProcessService)
	{
		pObject = m_ptrIPS->QueryInterface(iid);
		if (!pObject)
			throw OOCore_INotFoundException_MissingIID(iid);

		return;
	}

	ObjectPtr<IObject> ptrObject;
	for (size_t i=m_mapServicesByOid.find_first(strOid); i<m_mapServicesByOid.size() && *m_mapServicesByOid.key_at(i)==strOid; ++i)
	{
		Info* pInfo = m_mapServicesByCookie.find(*m_mapServicesByOid.at(i));
		if (pInfo)
		{
			// Check its still alive...
			if (!Remoting::IsAlive(pInfo->m_ptrObject))
			{
				int err = revoke_list.push(*m_mapServicesByOid.at(i));
				if (err != 0)
					OMEGA_THROW(err);
			}
			else
			{
				if (iid == OMEGA_GUIDOF(IObject))
					ptrObject = pInfo->m_ptrObject;
				else
				{
					ptrObject = pInfo->m_ptrObject->QueryInterface(iid);
					if (!ptrObject)
						throw OOCore_INotFoundException_MissingIID(iid);
				}

				// Remove the entry if Activation::SingleUse
				if (pInfo->m_flags & Activation::SingleUse)
				{
					int err = revoke_list.push(*m_mapServicesByOid.at(i));
					if (err != 0)
						OMEGA_THROW(err);
				}
				break;
			}
		}
	}

	guard.release();

	// Revoke the revoke_list
	for (uint32_t i = 0;revoke_list.pop(&i);)
		RevokeObject(i);
	
	// If we have an object, get out now
	if (ptrObject)
		pObject = ptrObject.Detach();
	else
	{
		// Route to global rot
		GetROT(true)->GetObject(oid,iid,pObject);
	}
}

void LocalROT::RevokeObject(uint32_t cookie)
{
	OOBase::Guard<OOBase::RWMutex> guard(m_lock);
	
	Info info;
	if (m_mapServicesByCookie.remove(cookie,&info))
	{
		for (size_t i=m_mapServicesByOid.find_first(info.m_oid); i<m_mapServicesByOid.size() && *m_mapServicesByOid.key_at(i)==info.m_oid; ++i)
		{
			if (*m_mapServicesByOid.at(i) == cookie)
				m_mapServicesByOid.remove_at(i);
		}

		guard.release();

		if (info.m_rot_cookie)
		{
			// Revoke from ROT
			ObjectPtr<Activation::IRunningObjectTable> ptrROT = GetROT(false);
			if (ptrROT)
				ptrROT->RevokeObject(info.m_rot_cookie);
		}
		else
			OnRevokeObject(info.m_oid,info.m_flags);
	}
}

void LocalROT::OnRegisterObject(const any_t& oid, Activation::RegisterFlags_t flags)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::Stack<uint32_t> stackRemove;

	for (size_t pos = m_mapNotify.begin(); pos != m_mapNotify.npos; pos = m_mapNotify.next(pos))
	{
		try
		{
			ObjectPtr<Activation::IRunningObjectTableNotify> ptrNotify = *m_mapNotify.at(pos);
			if (!ptrNotify || !Remoting::IsAlive(ptrNotify))
				stackRemove.push(*m_mapNotify.key_at(pos));
			else
				ptrNotify->OnRegisterObject(oid,flags);
		}
		catch (IException* pE)
		{
			pE->Release();
		}
	}

	read_guard.release();

	if (!stackRemove.empty())
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (uint32_t i = 0;stackRemove.pop(&i);)
			m_mapNotify.remove(i,NULL);
	}
}

void LocalROT::OnRevokeObject(const any_t& oid, Activation::RegisterFlags_t flags)
{
	OOBase::ReadGuard<OOBase::RWMutex> read_guard(m_lock);

	OOBase::Stack<uint32_t> stackRemove;

	for (size_t pos = m_mapNotify.begin(); pos != m_mapNotify.npos; pos = m_mapNotify.next(pos))
	{
		try
		{
			ObjectPtr<Activation::IRunningObjectTableNotify> ptrNotify = *m_mapNotify.at(pos);
			if (!ptrNotify || !Remoting::IsAlive(ptrNotify))
				stackRemove.push(*m_mapNotify.key_at(pos));
			else
				ptrNotify->OnRevokeObject(oid,flags);
		}
		catch (IException* pE)
		{
			pE->Release();
		}
	}

	read_guard.release();

	if (!stackRemove.empty())
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		for (uint32_t i = 0;stackRemove.pop(&i);)
			m_mapNotify.remove(i,NULL);
	}
}

uint32_t LocalROT::RegisterNotify(const guid_t& iid, IObject* pObject)
{
	uint32_t nCookie = 0;

	if (iid == OMEGA_GUIDOF(Activation::IRunningObjectTableNotify) && pObject)
	{
		ObjectPtr<Activation::IRunningObjectTableNotify> ptrNotify(static_cast<Activation::IRunningObjectTableNotify*>(pObject));
		ptrNotify.AddRef();

		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		int err = m_mapNotify.insert(ptrNotify,nCookie,1,0xFFFFFFFF);
		if (err)
			OMEGA_THROW(err);
	}

	return nCookie;
}

void LocalROT::UnregisterNotify(const guid_t& iid, uint32_t cookie)
{
	if (iid == OMEGA_GUIDOF(Activation::IRunningObjectTableNotify) && cookie)
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		m_mapNotify.remove(cookie,NULL);
	}
}

Notify::INotifier::iid_list_t LocalROT::ListNotifyInterfaces()
{
	Notify::INotifier::iid_list_t list;
	list.push_back(OMEGA_GUIDOF(Activation::IRunningObjectTableNotify));
	return list;
}

void OTL::Module::OOCore_ModuleImpl::RegisterObjectFactories(Activation::IRunningObjectTable* pROT, bool bPrivate)
{
	CreatorEntry* g = bPrivate ? getCreatorEntries_private() : getCreatorEntries();
	for (size_t i=0; g[i].pfnOid!=0; ++i)
	{
		ObjectPtr<Activation::IObjectFactory> ptrOF = static_cast<Activation::IObjectFactory*>(g[i].pfnCreate(OMEGA_GUIDOF(Activation::IObjectFactory)));

		g[i].cookie = pROT->RegisterObject(*(g[i].pfnOid)(),ptrOF,(*g[i].pfnRegistrationFlags)());
	}
}

void OTL::Module::OOCore_ModuleImpl::UnregisterObjectFactories(Activation::IRunningObjectTable* pROT, bool bPrivate)
{
	CreatorEntry* g = bPrivate ? getCreatorEntries_private() : getCreatorEntries();
	for (size_t i=0; g[i].pfnOid!=0; ++i)
	{
		pROT->RevokeObject(g[i].cookie);
		g[i].cookie = 0;
	}
}

OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(OOCore_RegisterIPS,1,((in),OOCore::IInterProcessService*,pIPS))
{
	LOCAL_ROT::instance().RegisterIPS(pIPS);
}

void OOCore::RegisterObjects()
{
	// Register all our local class factories
	Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->RegisterObjectFactories(LOCAL_ROT::instance_ptr(),true);
}

void OOCore::UnregisterObjects(bool bPrivate)
{
	LocalROT* pROT = LOCAL_ROT::instance_ptr();

	pROT->RevokeIPS();

	// Unregister all our local class factories
	Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->UnregisterObjectFactories(pROT,false);
	if (bPrivate)
		Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->UnregisterObjectFactories(pROT,true);
}

ObjectPtr<OOCore::IInterProcessService> OOCore::GetInterProcessService(bool bThrow)
{
	return LOCAL_ROT::instance().GetIPS(bThrow);
}

bool OOCore::HostedByOOServer()
{
	return LOCAL_ROT::instance().IsHosted();
}

IObject* OOCore::GetRegisteredObject(const guid_t& oid, const guid_t& iid)
{
	IObject* pObject = NULL;
	LOCAL_ROT::instance().GetObject(oid,iid,pObject);
	return pObject;
}

OMEGA_DEFINE_EXPORTED_FUNCTION(Remoting::IChannelSink*,OOCore_Remoting_OpenServerSink,2,((in),const guid_t&,message_oid,(in),Remoting::IChannelSink*,pSink))
{
	return OOCore::GetInterProcessService(true)->OpenServerSink(message_oid,pSink);
}

// {F67F5A41-BA32-48C9-BFD2-7B3701984DC8}
OMEGA_DEFINE_OID(Activation,OID_RunningObjectTable,"{F67F5A41-BA32-48C9-BFD2-7B3701984DC8}");

void OOCore::RunningObjectTableFactory::CreateInstance(const guid_t& iid, IObject*& pObject)
{
	pObject = LOCAL_ROT::instance().QueryInterface(iid);
	if (!pObject)
		throw OOCore_INotFoundException_MissingIID(iid);
}

// {EAAC4365-9B65-4C3C-94C2-CC8CC3E64D74}
OMEGA_DEFINE_OID(Registry,OID_Registry,"{EAAC4365-9B65-4C3C-94C2-CC8CC3E64D74}");

// {7A351233-8363-BA15-B443-31DD1C8FC587}
OMEGA_DEFINE_OID(Registry,OID_OverlayKeyFactory,"{7A351233-8363-BA15-B443-31DD1C8FC587}");

void OOCore::RegistryFactory::CreateInstance(const guid_t& iid, IObject*& pObject)
{
	ObjectPtr<Registry::IKey> ptrKey = LOCAL_ROT::instance().GetRootKey();

	pObject = ptrKey->QueryInterface(iid);
	if (!pObject)
		throw OOCore_INotFoundException_MissingIID(iid);
}

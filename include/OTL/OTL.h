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

#ifndef OTL_H_INCLUDED_
#define OTL_H_INCLUDED_

#if !defined(OMEGA_H_INCLUDED_)
#include "../Omega/Omega.h"
#endif

///////////////////////////////////////////////////////////////////
// Interface map macros
//
// These are designed to be used as follows:
//
// BEGIN_INTERFACE_MAP(myClass derived from ObjectBase)
//    INTERFACE_ENTRY(something derived from IObject)
//    INTERFACE_ENTRY_CHAIN(a base class of this class with a map)
// END_INTERFACE_MAP()
//

#define BEGIN_INTERFACE_MAP(cls) \
	private: typedef cls RootClass; \
	public: static const OTL::ObjectBase::QIEntry* getQIEntries() {static const OTL::ObjectBase::QIEntry QIEntries[] = {

#define INTERFACE_ENTRY(iface) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIDelegate<iface,RootClass>, 0, 0, 0 },

#define INTERFACE_ENTRY_IID(iid,iface) \
	{ &iid, &OTL::ObjectBase::QIDelegate<iface,RootClass>, 0, 0, 0 },

#define INTERFACE_ENTRY2(iface,iface2) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIDelegate2<iface,iface2,RootClass>, 0, 0, 0 },

#define INTERFACE_ENTRY2_IID(iid,iface,iface2) \
	{ &iid, &OTL::ObjectBase::QIDelegate2<iface,iface2,RootClass>, 0, 0, 0 },

#define INTERFACE_ENTRY_CHAIN(baseClass) \
	{ &Omega::guid_t::Null(), &OTL::ObjectBase::QIChain<baseClass,RootClass>, 0, 0, baseClass::getQIEntries() },

#define INTERFACE_ENTRY_AGGREGATE(iface,member_object) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIAggregate, offsetof(RootClass,member_object)+1, 0, 0 },

#define INTERFACE_ENTRY_AGGREGATE_BLIND(member_object) \
	{ &Omega::guid_t::Null(), &OTL::ObjectBase::QIAggregate, offsetof(RootClass,member_object)+1, 0, 0 },

#define INTERFACE_ENTRY_FUNCTION(iface,pfn) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIFunction<RootClass>, 0, static_cast<OTL::ObjectBase::PFNMEMQI>(pfn), 0 },

#define INTERFACE_ENTRY_FUNCTION_BLIND(pfn) \
	{ &Omega::guid_t::Null(), &OTL::ObjectBase::QIFunction<RootClass>, 0, static_cast<OTL::ObjectBase::PFNMEMQI>(pfn), 0 },

#define INTERFACE_ENTRY_NOINTERFACE(iface) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIFail, 0, 0, 0 },

#define END_INTERFACE_MAP() \
	{ 0,0,0,0,0 } }; return QIEntries; }

///////////////////////////////////////////////////////////////////
// Object map macros
//
// These are designed to be used as follows:
//
// For dynamic link libraries:
//
// BEGIN_LIBRARY_OBJECT_MAP()
//    OBJECT_MAP_ENTRY(class derived from AutoObjectFactory)
// END_LIBRARY_OBJECT_MAP()
//
// or, for Exe's
//
// BEGIN_PROCESS_OBJECT_MAP()
//    OBJECT_MAP_ENTRY(class derived from AutoObjectFactory)
// END_PROCESS_OBJECT_MAP()
//

#define BEGIN_LIBRARY_OBJECT_MAP() \
	namespace OTL { \
	namespace Module { \
	class OMEGA_PRIVATE_TYPE(LibraryModuleImpl) : public LibraryModule \
	{ \
		ModuleBase::CreatorEntry* getCreatorEntries() { static ModuleBase::CreatorEntry CreatorEntries[] = {

#define OBJECT_MAP_ENTRY(obj) \
		{ &obj::GetOid, &obj::GetRegistrationFlags, &Creator<obj::ObjectFactoryClass>::Create, 0 },

#define OBJECT_MAP_FACTORY_ENTRY(obj) \
		{ &obj::GetOid, &obj::GetRegistrationFlags, &Creator<obj>::Create, 0 },

#define END_LIBRARY_OBJECT_MAP() \
		{ 0,0,0,0 } }; return CreatorEntries; } \
	}; \
	OMEGA_PRIVATE_FN_DECL(Module::OMEGA_PRIVATE_TYPE(LibraryModuleImpl)*,GetModule)() { return Omega::Threading::Singleton<Module::OMEGA_PRIVATE_TYPE(LibraryModuleImpl),Omega::Threading::ModuleDestructor<Omega::System::Internal::OMEGA_PRIVATE_TYPE(safe_module)> >::instance(); } \
	OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)() { return OMEGA_PRIVATE_FN_CALL(GetModule)(); } \
	} } \
	OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_GetLibraryObject,3,((in),const Omega::guid_t&,oid,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject)) \
	{ pObject = OTL::Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->GetLibraryObject(oid,iid); } \
	OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::bool_t,Omega_CanUnloadLibrary,0,()) \
	{ return !(OTL::Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->HaveLocks()); }

#define BEGIN_PROCESS_OBJECT_MAP() \
	namespace OTL { \
	namespace Module { \
	class OMEGA_PRIVATE_TYPE(ProcessModuleImpl) : public ProcessModule \
	{ \
	private: \
		ModuleBase::CreatorEntry* getCreatorEntries() { static ModuleBase::CreatorEntry CreatorEntries[] = {

#define END_PROCESS_OBJECT_MAP() \
		{ 0,0,0,0 } }; return CreatorEntries; } \
	}; \
	OMEGA_PRIVATE_FN_DECL(Module::OMEGA_PRIVATE_TYPE(ProcessModuleImpl)*,GetModule)() { return Omega::Threading::Singleton<Module::OMEGA_PRIVATE_TYPE(ProcessModuleImpl),Omega::Threading::ModuleDestructor<Omega::System::Internal::OMEGA_PRIVATE_TYPE(safe_module)> >::instance(); } \
	OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)() { return OMEGA_PRIVATE_FN_CALL(GetModule)(); } \
	} \
	}

namespace OTL
{
	template <typename OBJECT>
	class ObjectPtr;

	template <typename OBJECT>
	class ObjectPtrBase
	{
	public:
		ObjectPtrBase(OBJECT* obj, bool bAddRef) :
				m_ptr(obj)
		{
			if (m_ptr && bAddRef)
				m_ptr->AddRef();
		}

		ObjectPtrBase(const Omega::any_t& oid, Omega::Activation::Flags_t flags)
		{
			m_ptr = static_cast<OBJECT*>(Omega::CreateInstance(oid,flags,OMEGA_GUIDOF(OBJECT)));
		}

		virtual ~ObjectPtrBase()
		{
			Release();
		}

		void GetInstance(const Omega::any_t& oid, Omega::Activation::Flags_t flags)
		{
			replace(static_cast<OBJECT*>(Omega::GetInstance(oid,flags,OMEGA_GUIDOF(OBJECT))),false);
		}

		OBJECT* Detach()
		{
			OBJECT* ptr = m_ptr;
			m_ptr = NULL;
			return ptr;
		}

		OBJECT* AddRef()
		{
			if (m_ptr)
				m_ptr->AddRef();

			return m_ptr;
		}

		void Release()
		{
			replace(NULL,false);
		}

		template <typename Q>
		ObjectPtr<Q> QueryInterface() const
		{
			return ObjectPtr<Q>(m_ptr ? static_cast<Q*>(m_ptr->QueryInterface(OMEGA_GUIDOF(Q))) : NULL);
		}

		OBJECT* operator ->() const
		{
			return m_ptr;
		}

		operator OBJECT*() const
		{
			return m_ptr;
		}

		operator OBJECT*&()
		{
			return m_ptr;
		}

	protected:
		OBJECT* m_ptr;

		void replace(OBJECT* ptr, bool bAddRef)
		{
			if (ptr != m_ptr)
			{
				if (m_ptr)
					m_ptr->Release();

				m_ptr = ptr;

				if (m_ptr && bAddRef)
					m_ptr->AddRef();
			}
		}

	private:
		ObjectPtrBase(const ObjectPtrBase& rhs);
		ObjectPtrBase& operator = (const ObjectPtrBase& rhs);
	};

	template <typename OBJECT>
	class ObjectPtr : public ObjectPtrBase<OBJECT>
	{
	public:
		ObjectPtr(OBJECT* obj = NULL) :
				ObjectPtrBase<OBJECT>(obj,false)
		{ }

		ObjectPtr(const ObjectPtr<OBJECT>& rhs) :
				ObjectPtrBase<OBJECT>(rhs.m_ptr,true)
		{ }

		ObjectPtr(const Omega::any_t& oid, Omega::Activation::Flags_t flags = Omega::Activation::Default) :
				ObjectPtrBase<OBJECT>(oid,flags)
		{ }

		ObjectPtr(const char* name, Omega::Activation::Flags_t flags = Omega::Activation::Default) :
				ObjectPtrBase<OBJECT>(Omega::string_t(name),flags)
		{ }

		ObjectPtr& operator = (const ObjectPtr<OBJECT>& rhs)
		{
			if (this != &rhs)
				this->replace(rhs.m_ptr,true);

			return *this;
		}

		ObjectPtr& operator = (OBJECT* obj)
		{
			this->replace(obj,false);
			return *this;
		}

		void Unmarshal(Omega::Remoting::IMarshaller* pMarshaller, const Omega::string_t& strName, Omega::Remoting::IMessage* pMessage)
		{
			Omega::IObject* pObj = NULL;
			pMarshaller->UnmarshalInterface(strName,pMessage,OMEGA_GUIDOF(OBJECT),pObj);
			this->replace(static_cast<OBJECT*>(pObj),false);
		}
	};

	template <>
	class ObjectPtr<Omega::IObject> : public ObjectPtrBase<Omega::IObject>
	{
	public:
		ObjectPtr(Omega::IObject* obj = NULL) :
				ObjectPtrBase<Omega::IObject>(obj,false)
		{ }

		ObjectPtr(const ObjectPtr<Omega::IObject>& rhs) :
				ObjectPtrBase<Omega::IObject>(rhs.m_ptr,true)
		{ }

		ObjectPtr(const Omega::any_t& oid, Omega::Activation::Flags_t flags) :
				ObjectPtrBase<Omega::IObject>(oid,flags)
		{ }

		ObjectPtr(const char* name, Omega::Activation::Flags_t flags = Omega::Activation::Default) :
				ObjectPtrBase<Omega::IObject>(Omega::string_t(name),flags)
		{ }

		ObjectPtr& operator = (const ObjectPtr<Omega::IObject>& rhs)
		{
			if (this != &rhs)
				replace(rhs.m_ptr,true);

			return *this;
		}

		ObjectPtr& operator = (Omega::IObject* obj)
		{
			replace(obj,false);

			return *this;
		}
	};

	template <class I>
	inline ObjectPtr<I> QueryInterface(Omega::IObject* pObj)
	{
		I* p = NULL;
		if (pObj)
			p = static_cast<I*>(pObj->QueryInterface(OMEGA_GUIDOF(I)));

		return ObjectPtr<I>(p);
	}

#if defined(OMEGA_REGISTRY_H_INCLUDED_)

	// If the compiler moans here, then you need to #include <OTL/Registry.h>
	template <>
	class ObjectPtr<Omega::Registry::IKey>;

#endif // OMEGA_REGISTRY_H_INCLUDED_

	class ObjectBase : public Omega::System::Internal::ThrowingNew
	{
	protected:
		ObjectBase()
		{}

		virtual ~ObjectBase()
		{}

		virtual void Internal_AddRef()
		{
			m_refcount.AddRef();
		}

		virtual void Internal_Release()
		{
			if (m_refcount.Release() == 0)
				Final_Release();
		}

		virtual void Final_Release()
		{
			delete this;
		}

	public:
		typedef Omega::IObject* (ObjectBase::*PFNMEMQI)(const Omega::guid_t& iid);

		struct QIEntry
		{
			const Omega::guid_t* pGuid;
			Omega::IObject* (*pfnQI)(const Omega::guid_t& iid, void* pThis, size_t offset, ObjectBase::PFNMEMQI pfnMemQI);
			size_t offset;
			PFNMEMQI pfnMemQI;
			const QIEntry* baseEntries;
		};

#if !defined(__BORLANDC__)
	protected:
#endif

		virtual Omega::IObject* Internal_QueryInterface(const Omega::guid_t& iid, const QIEntry* pEntries)
		{
			for (size_t i=0; pEntries && pEntries[i].pGuid!=0; ++i)
			{
				if (*(pEntries[i].pGuid) == iid ||
						*(pEntries[i].pGuid) == Omega::guid_t::Null() ||
						iid == OMEGA_GUIDOF(Omega::IObject))
				{
					return pEntries[i].pfnQI(iid,this,pEntries[i].offset-1,pEntries[i].pfnMemQI);
				}
			}

			return 0;
		}

		template <typename Interface, typename Implementation>
		static Omega::IObject* QIDelegate(const Omega::guid_t&, void* pThis, size_t, ObjectBase::PFNMEMQI)
		{
			/*******************************************
			*
			* If you get compiler errors here, make sure
			* you have derived from each class you have
			* included in your interface map!
			*
			********************************************/
			Interface* pI = static_cast<Interface*>(static_cast<Implementation*>(pThis));
			pI->AddRef();
			return pI;
		}

		template <typename Interface, typename Interface2, typename Implementation>
		static Omega::IObject* QIDelegate2(const Omega::guid_t&, void* pThis, size_t, ObjectBase::PFNMEMQI)
		{
			Interface* pI = static_cast<Interface*>(static_cast<Interface2*>(static_cast<Implementation*>(pThis)));
			pI->AddRef();
			return pI;
		}

		template <typename Base, typename Implementation>
		static Omega::IObject* QIChain(const Omega::guid_t& iid, void* pThis, size_t, ObjectBase::PFNMEMQI)
		{
			return static_cast<Implementation*>(pThis)->Internal_QueryInterface(iid,Base::getQIEntries());
		}

		static Omega::IObject* QIAggregate(const Omega::guid_t& iid, void* pThis, size_t offset, ObjectBase::PFNMEMQI)
		{
			return reinterpret_cast<Omega::IObject*>(reinterpret_cast<size_t>(pThis)+offset)->QueryInterface(iid);
		}

		template <typename Implementation>
		static Omega::IObject* QIFunction(const Omega::guid_t& iid, void* pThis, size_t, ObjectBase::PFNMEMQI pfnMemQI)
		{
			return (static_cast<Implementation*>(pThis)->*pfnMemQI)(iid);
		}

		static Omega::IObject* QIFail(const Omega::guid_t&, void*, size_t, ObjectBase::PFNMEMQI)
		{
			return 0;
		}

	protected:
		Omega::Threading::AtomicRefCount m_refcount;
	};

	class ModuleBase
	{
	public:
		bool HaveLocks() const;
		void IncLockCount();
		void DecLockCount();

		virtual void RegisterObjectFactories()
		{
			// Override this in a derived class, or do not call
		}

		virtual void RegisterObjectFactory(const Omega::guid_t& /*oid*/)
		{
			// Override this in a derived class, or do not call
		}

		virtual void UnregisterObjectFactories()
		{
			// Override this in a derived class, or do not call
		}

		virtual void UnregisterObjectFactory(const Omega::guid_t& /*oid*/)
		{
			// Override this in a derived class, or do not call
		}

		virtual void Run()
		{
			// Override this in a derived class, or do not call
		}

	protected:
		ModuleBase() {}
		virtual ~ModuleBase() {}

		struct CreatorEntry
		{
			const Omega::guid_t* (*pfnOid)();
			const Omega::Activation::RegisterFlags_t (*pfnRegistrationFlags)();
			Omega::IObject* (*pfnCreate)(const Omega::guid_t& iid);
			Omega::uint32_t cookie;
		};

		virtual CreatorEntry* getCreatorEntries() = 0;

	private:
		Omega::Threading::AtomicRefCount m_lockCount;

		ModuleBase(const ModuleBase&);
		ModuleBase& operator = (const ModuleBase&);
	};

	namespace Module
	{
		OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)();
	}

	inline static ModuleBase* GetModule()
	{
		return Module::OMEGA_PRIVATE_FN_CALL(GetModuleBase)();
	}

	template <typename ROOT>
	class ObjectImpl : public ROOT
	{
	public:
		static ObjectImpl<ROOT>* CreateInstance()
		{
			return new ObjectImpl<ROOT>();
		}

	private:
		ObjectImpl() : ROOT()
		{
			GetModule()->IncLockCount();
			this->AddRef();
		}

		virtual ~ObjectImpl()
		{
			GetModule()->DecLockCount();
		}

		ObjectImpl(const ObjectImpl& rhs);
		ObjectImpl& operator = (const ObjectImpl& rhs);

	// IObject members
	public:
		virtual void AddRef()
		{
			this->Internal_AddRef();
		}

		virtual void Release()
		{
			this->Internal_Release();
		}

		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return this->Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <typename ROOT>
	class NoLockObjectImpl : public ROOT
	{
	public:
		static NoLockObjectImpl<ROOT>* CreateInstance()
		{
			return new NoLockObjectImpl<ROOT>();
		}

	private:
		NoLockObjectImpl() : ROOT()
		{
			this->AddRef();
		}

		virtual ~NoLockObjectImpl()
		{ }

	// IObject members
	public:
		virtual void AddRef()
		{
			this->Internal_AddRef();
		}

		virtual void Release()
		{
			this->Internal_Release();
		}

		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return this->Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <typename ROOT>
	class SingletonObjectImpl : public ROOT
	{
		friend class Omega::Threading::Singleton<SingletonObjectImpl<ROOT>,Omega::Threading::ModuleDestructor<Omega::System::Internal::OMEGA_PRIVATE_TYPE(safe_module)> >;

	public:
		static SingletonObjectImpl<ROOT>* CreateInstance()
		{
			SingletonObjectImpl<ROOT>* pObject = Omega::Threading::Singleton<SingletonObjectImpl<ROOT>,Omega::Threading::ModuleDestructor<Omega::System::Internal::OMEGA_PRIVATE_TYPE(safe_module)> >::instance();
			pObject->AddRef();
			return pObject;
		}

	protected:
		SingletonObjectImpl() : ROOT()
		{ }

		virtual ~SingletonObjectImpl()
		{ }

	// IObject members
	public:
		virtual void AddRef()
		{
			GetModule()->IncLockCount();
		}

		virtual void Release()
		{
			GetModule()->DecLockCount();
		}

		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return this->Internal_QueryInterface(iid,ROOT::getQIEntries());
		}

	private:
		SingletonObjectImpl(const SingletonObjectImpl&);
		SingletonObjectImpl& operator = (const SingletonObjectImpl&);
	};

	template <typename ROOT>
	class StackObjectImpl : public ROOT
	{
	public:
		StackObjectImpl() : ROOT()
		{ }

		virtual ~StackObjectImpl()
		{ }

	// IObject members
	public:
		virtual void AddRef() { }
		virtual void Release() { }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return this->Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <const Omega::guid_t* pOID, const Omega::Activation::RegisterFlags_t flags = Omega::Activation::UserScope | Omega::Activation::MultipleUse>
	class ObjectFactoryBase :
		public OTL::ObjectBase,
		public Omega::Activation::IObjectFactory
	{
	public:
		static const Omega::guid_t* GetOid()
		{
			return pOID;
		}

		static const Omega::Activation::RegisterFlags_t GetRegistrationFlags()
		{
			return flags;
		}

		BEGIN_INTERFACE_MAP(ObjectFactoryBase)
			INTERFACE_ENTRY(Omega::Activation::IObjectFactory)
		END_INTERFACE_MAP()
	};

	template <typename T, const Omega::guid_t* pOID>
	class AutoObjectFactoryCallCreate
	{
	public:
		static Omega::IObject* CreateInstance(const Omega::guid_t& iid)
		{
			ObjectPtr<T> ptr = T::CreateInstance();
			Omega::IObject* ret = ptr->QueryInterface(iid);
			if (!ret)
				throw OOCore_INotFoundException_MissingIID(iid);
			return ret;
		}
	};

	template <typename T>
	class AutoObjectFactoryImpl :
			public ObjectBase,
			public Omega::Activation::IObjectFactory
	{
	public:
		BEGIN_INTERFACE_MAP(AutoObjectFactoryImpl)
			INTERFACE_ENTRY(Omega::Activation::IObjectFactory)
		END_INTERFACE_MAP()

	// IObjectFactory members
	public:
		void CreateInstance(const Omega::guid_t& iid, Omega::IObject*& pObject)
		{
			pObject = T::CreateInstance(iid);
		}
	};

	template <typename ROOT, const Omega::guid_t* pOID, const Omega::Activation::RegisterFlags_t flags = Omega::Activation::UserScope | Omega::Activation::MultipleUse>
	class AutoObjectFactory
	{
	public:
		typedef AutoObjectFactoryImpl<AutoObjectFactoryCallCreate<ObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

		static const Omega::guid_t* GetOid()
		{
			return pOID;
		}

		static const Omega::Activation::RegisterFlags_t GetRegistrationFlags()
		{
			return flags;
		}
	};

	template <typename ROOT, const Omega::guid_t* pOID, const Omega::Activation::RegisterFlags_t flags = Omega::Activation::UserScope | Omega::Activation::MultipleUse>
	class AutoObjectFactorySingleton : public AutoObjectFactory<ROOT,pOID,flags>
	{
	public:
		typedef AutoObjectFactoryImpl<AutoObjectFactoryCallCreate<SingletonObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

	protected:
		virtual ~AutoObjectFactorySingleton() {}
	};

	class LibraryModule : public ModuleBase
	{
	public:
		template <typename T>
		struct Creator
		{
			static Omega::IObject* Create(const Omega::guid_t& iid)
			{
				ObjectPtr<ObjectImpl<T> > ptr = ObjectImpl<T>::CreateInstance();
				Omega::IObject* pObject = ptr->QueryInterface(iid);
				if (!pObject)
					throw OOCore_INotFoundException_MissingIID(iid);
				return pObject;
			}
		};

		Omega::IObject* GetLibraryObject(const Omega::guid_t& oid, const Omega::guid_t& iid);

	protected:
		LibraryModule()
		{}
	};

	class ProcessModule : public ModuleBase
	{
	public:
		virtual void RegisterObjectFactories();
		virtual void RegisterObjectFactory(const Omega::guid_t& oid);
		virtual void UnregisterObjectFactories();
		virtual void UnregisterObjectFactory(const Omega::guid_t& oid);
		virtual void Run();

	protected:
		template <typename T>
		struct Creator
		{
			static Omega::IObject* Create(const Omega::guid_t& iid)
			{
				ObjectPtr<NoLockObjectImpl<T> > ptr = NoLockObjectImpl<T>::CreateInstance();
				Omega::IObject* pObject = ptr->QueryInterface(iid);
				if (!pObject)
					throw OOCore_INotFoundException_MissingIID(iid);
				return pObject;
			}
		};

		ProcessModule()
		{}
	};

#if defined(OMEGA_TYPEINFO_H_INCLUDED_)

	template <typename ROOT>
	class IProvideObjectInfoImpl :
			public Omega::TypeInfo::IProvideObjectInfo
	{
	private:
		Omega::TypeInfo::IProvideObjectInfo::iid_list_t WalkEntries(const ObjectBase::QIEntry* pEntries)
		{
			Omega::TypeInfo::IProvideObjectInfo::iid_list_t retval;

			for (size_t i=0; pEntries && pEntries[i].pGuid!=0; ++i)
			{
				if (*(pEntries[i].pGuid) != Omega::guid_t::Null())
				{
					if (!pEntries[i].pfnMemQI)
						retval.push_back(*(pEntries[i].pGuid));
					else
					{
						ObjectPtr<Omega::IObject> ptrObj = pEntries[i].pfnQI(*(pEntries[i].pGuid),this,pEntries[i].offset-1,pEntries[i].pfnMemQI);
						if (ptrObj)
							retval.push_back(*(pEntries[i].pGuid));
					}
				}
				else if (pEntries[i].offset != 0)
				{
					ObjectPtr<Omega::TypeInfo::IProvideObjectInfo> ptrAgg = static_cast<Omega::TypeInfo::IProvideObjectInfo*>(pEntries[i].pfnQI(OMEGA_GUIDOF(Omega::TypeInfo::IProvideObjectInfo),this,pEntries[i].offset-1,pEntries[i].pfnMemQI));
					if (ptrAgg)
					{
						Omega::TypeInfo::IProvideObjectInfo::iid_list_t agg = ptrAgg->EnumInterfaces();
						retval.insert(retval.end(),agg.begin(),agg.end());
					}
				}
				else if (pEntries[i].baseEntries)
				{
					Omega::TypeInfo::IProvideObjectInfo::iid_list_t base = WalkEntries(pEntries[i].baseEntries);
					retval.insert(retval.end(),base.begin(),base.end());
				}
			}

			return retval;
		}

	// IProvideObjectInfo members
	public:
		virtual Omega::TypeInfo::IProvideObjectInfo::iid_list_t EnumInterfaces()
		{
			return WalkEntries(ROOT::getQIEntries());
		}
	};

#endif // OMEGA_TYPEINFO_H_INCLUDED_
}

#include "OTL.inl"

#endif // OTL_H_INCLUDED_

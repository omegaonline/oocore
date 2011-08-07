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

#define END_LIBRARY_OBJECT_MAP_NO_ENTRYPOINT() \
		{ 0,0,0,0 } }; return CreatorEntries; } \
	}; \
	OMEGA_PRIVATE_FN_DECL(Module::OMEGA_PRIVATE_TYPE(LibraryModuleImpl)*,GetModule)() { return Omega::Threading::Singleton<Module::OMEGA_PRIVATE_TYPE(LibraryModuleImpl),Omega::Threading::ModuleDestructor<Omega::System::Internal::OMEGA_PRIVATE_TYPE(safe_module)> >::instance(); } \
	OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)() { return OMEGA_PRIVATE_FN_CALL(GetModule)(); } \
	} \
	}

#define END_LIBRARY_OBJECT_MAP() \
	END_LIBRARY_OBJECT_MAP_NO_ENTRYPOINT() \
	OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_GetLibraryObject,3,((in),const Omega::guid_t&,oid,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject)) \
	{ pObject = OTL::Module::OMEGA_PRIVATE_FN_CALL(GetModule)()->GetLibraryObject(oid,iid); }

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
	class ObjectPtrBase
	{
	public:
		ObjectPtrBase(OBJECT* obj) :
				m_ptr(obj)
		{
			if (m_ptr)
				m_ptr->AddRef();
		}

		ObjectPtrBase(const ObjectPtrBase& rhs) :
				m_ptr(rhs.m_ptr)
		{
			if (m_ptr)
				m_ptr->AddRef();
		}

		ObjectPtrBase(const Omega::any_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter) :
				m_ptr(0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::CreateInstance(oid,flags,pOuter,OMEGA_GUIDOF(OBJECT)));
		}

		virtual ~ObjectPtrBase()
		{
			Attach(NULL);
		}

		ObjectPtrBase& operator = (OBJECT* ptr)
		{
			if (ptr != m_ptr)
			{
				if (ptr)
					ptr->AddRef();

				OBJECT* old = m_ptr;
				m_ptr = ptr;

				if (old)
					old->Release();
			}

			return *this;
		}

		void Attach(OBJECT* obj)
		{
			OBJECT* old = m_ptr;
			m_ptr = obj;
			if (old)
				old->Release();
		}

		void Detach()
		{
			m_ptr = NULL;
		}

		OBJECT* AddRef()
		{
			if (m_ptr)
				m_ptr->AddRef();

			return m_ptr;
		}

		void Release()
		{
			Attach(0);
		}

		template <typename Q>
		Q* QueryInterface() const
		{
			return (m_ptr ? static_cast<Q*>(m_ptr->QueryInterface(OMEGA_GUIDOF(Q))) : NULL);
		}

		OBJECT* operator ->() const
		{
			assert(m_ptr != NULL);

			return m_ptr;
		}

		operator OBJECT*() const
		{
			return m_ptr;
		}

	protected:
		OBJECT* m_ptr;

	private:
		ObjectPtrBase& operator = (const ObjectPtrBase& rhs);
	};

	template <typename OBJECT>
	class ObjectPtr : public ObjectPtrBase<OBJECT>
	{
	public:
		ObjectPtr(OBJECT* obj = NULL) :
				ObjectPtrBase<OBJECT>(obj)
		{ }

		template <typename I>
		ObjectPtr(I* pObject) :
				ObjectPtrBase<OBJECT>(0)
		{
			if (pObject)
				this->m_ptr = static_cast<OBJECT*>(pObject->QueryInterface(OMEGA_GUIDOF(OBJECT)));
		}

		ObjectPtr(const ObjectPtr<OBJECT>& rhs) :
				ObjectPtrBase<OBJECT>(rhs)
		{ }

		template <typename I>
		ObjectPtr(const ObjectPtr<I>& rhs) :
				ObjectPtrBase<OBJECT>(0)
		{
			if (rhs)
				this->m_ptr = static_cast<OBJECT*>(rhs->QueryInterface(OMEGA_GUIDOF(OBJECT)));
		}

		ObjectPtr(const Omega::any_t& oid, Omega::Activation::Flags_t flags = Omega::Activation::Default, Omega::IObject* pOuter = NULL) :
				ObjectPtrBase<OBJECT>(oid,flags,pOuter)
		{ }

		ObjectPtr(const wchar_t* name, Omega::Activation::Flags_t flags = Omega::Activation::Default, Omega::IObject* pOuter = NULL) :
				ObjectPtrBase<OBJECT>(Omega::string_t(name,Omega::string_t::npos),flags,pOuter)
		{ }

		ObjectPtr& operator = (const ObjectPtr<OBJECT>& rhs)
		{
			if (this != &rhs)
				*this = rhs.m_ptr;

			return *this;
		}

		ObjectPtr& operator = (OBJECT* obj)
		{
			ObjectPtrBase<OBJECT>::operator = (obj);
			return *this;
		}
	};

	template <>
	class ObjectPtr<Omega::IObject> : public ObjectPtrBase<Omega::IObject>
	{
	public:
		ObjectPtr(Omega::IObject* obj = NULL) :
				ObjectPtrBase<Omega::IObject>(obj)
		{ }

		ObjectPtr(const ObjectPtr<Omega::IObject>& rhs) :
				ObjectPtrBase<Omega::IObject>(rhs)
		{ }

		ObjectPtr(const Omega::any_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter = NULL) :
				ObjectPtrBase<Omega::IObject>(oid,flags,pOuter)
		{ }

		ObjectPtr(const wchar_t* name, Omega::Activation::Flags_t flags = Omega::Activation::Default, Omega::IObject* pOuter = NULL) :
				ObjectPtrBase<Omega::IObject>(Omega::string_t(name,Omega::string_t::npos),flags,pOuter)
		{ }

		ObjectPtr& operator = (const ObjectPtr<Omega::IObject>& rhs)
		{
			if (this != &rhs)
				*this = rhs.m_ptr;

			return *this;
		}

		ObjectPtr& operator = (Omega::IObject* obj)
		{
			ObjectPtrBase<Omega::IObject>::operator = (obj);
			return *this;
		}
	};

	// If the compiler moans here, then you need to #include <OTL/Registry.h>
	template <>
	class ObjectPtr<Omega::Registry::IKey>;

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
			assert(!m_refcount.IsZero());

			if (m_refcount.Release() == 0)
				Final_Release();
		}

		virtual void Final_Release()
		{
			delete this;
		}

		virtual void Init_Once() 
		{}

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
		Omega::Threading::Mutex& GetLock();

		virtual void RegisterObjectFactories()
		{
			// Override this in a derived class, or do not call
			assert(false);
		}

		virtual void UnregisterObjectFactories()
		{
			// Override this in a derived class, or do not call
			assert(false);
		}

		virtual void Run()
		{
			// Override this in a derived class, or do not call
			assert(false);
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
		Omega::Threading::Mutex          m_csMain;
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

		static ObjectPtr<ObjectImpl<ROOT> > CreateInstancePtr()
		{
			ObjectPtr<ObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateInstance());
			return ptr;
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

		static ObjectPtr<NoLockObjectImpl<ROOT> > CreateInstancePtr()
		{
			ObjectPtr<NoLockObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateInstance());
			return ptr;
		}

	private:
		NoLockObjectImpl() : ROOT()
		{
			this->AddRef();
		}

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
	class AggregatedObjectImpl;

	template <typename ROOT>
	class ContainedObjectImpl : public ROOT
	{
		friend class AggregatedObjectImpl<ROOT>;

	public:
		ContainedObjectImpl(Omega::IObject* pOuter) :
				m_pOuter(pOuter)
		{
			Omega::System::PinObjectPointer(m_pOuter);
		}

		Omega::IObject* m_pOuter;

	private:
		ContainedObjectImpl(const ContainedObjectImpl& rhs);
		ContainedObjectImpl& operator = (const ContainedObjectImpl& rhs);

		virtual ~ContainedObjectImpl()
		{
			Omega::System::UnpinObjectPointer(m_pOuter);
		}

	// IObject members
	public:
		virtual void AddRef()
		{
			m_pOuter->AddRef();
		}

		virtual void Release()
		{
			m_pOuter->Release();
		}

		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return m_pOuter->QueryInterface(iid);
		}
	};

	template <typename ROOT>
	class AggregatedObjectImpl : public Omega::IObject, public Omega::System::Internal::ThrowingNew
	{
		AggregatedObjectImpl(Omega::IObject* pOuter) : m_contained(pOuter)
		{
			GetModule()->IncLockCount();
			AddRef();
		}

		virtual ~AggregatedObjectImpl()
		{
			GetModule()->DecLockCount();
		}

		// If the line below is flagged as the source of a compiler warning then
		// you have missed out at least one virtual function in an interface that
		// <ROOT> derives from
		ContainedObjectImpl<ROOT>        m_contained;
		Omega::Threading::AtomicRefCount m_refcount;

	public:
		static AggregatedObjectImpl* CreateInstance(Omega::IObject* pOuter)
		{
			return new AggregatedObjectImpl(pOuter);
		}

		static ObjectPtr<AggregatedObjectImpl> CreateInstancePtr(Omega::IObject* pOuter)
		{
			ObjectPtr<AggregatedObjectImpl> ptr;
			ptr.Attach(CreateInstance(pOuter));
			return ptr;
		}

		ROOT* ContainedObject()
		{
			return &m_contained;
		}

	// IObject members
	public:
		virtual void AddRef()
		{
			m_refcount.AddRef();
		}

		virtual void Release()
		{
			assert(!m_refcount.IsZero());

			if (m_refcount.Release() == 0)
				delete this;
		}

		Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return m_contained.Internal_QueryInterface(iid,ROOT::getQIEntries());
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

		static ObjectPtr<SingletonObjectImpl<ROOT> > CreateInstancePtr()
		{
			ObjectPtr<SingletonObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateInstance());
			return ptr;
		}

	protected:
		SingletonObjectImpl() : ROOT()
		{ 
			ROOT::Init_Once();
		}

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

	// IObject members
	public:
		virtual void AddRef() { }
		virtual void Release() { }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return this->Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};
	
	template <const Omega::guid_t* pOID, const Omega::Activation::RegisterFlags_t flags = Omega::Activation::ProcessLocal | Omega::Activation::UserLocal | Omega::Activation::MultipleUse>
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
		static Omega::IObject* CreateInstance(Omega::IObject* pOuter, const Omega::guid_t& iid)
		{
			Omega::IObject* ret = NULL;
			ObjectPtr<T> ptr = T::CreateInstancePtr(pOuter);
			if (iid != OMEGA_GUIDOF(Omega::IObject))
				ret = ptr->QueryInterface(iid);
			else
				ret = ptr.AddRef();
			if (!ret)
				throw Omega::INoInterfaceException::Create(iid);
			return ret;
		}

		static Omega::IObject* CreateInstance(const Omega::guid_t& iid)
		{
			Omega::IObject* ret = T::CreateInstancePtr()->QueryInterface(iid);
			if (!ret)
				throw Omega::INoInterfaceException::Create(iid);
			return ret;
		}
	};

	template <const Omega::guid_t* pOID>
	class AutoObjectFactoryCallCreateThrow
	{
	public:
		static Omega::IObject* CreateInstance(Omega::IObject*, const Omega::guid_t&)
		{
			throw Omega::Activation::INoAggregationException::Create(*pOID);
		}
	};

	template <typename T1, typename T2>
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
		void CreateInstance(Omega::IObject* pOuter, const Omega::guid_t& iid, Omega::IObject*& pObject)
		{
			if (pOuter)
				pObject = T1::CreateInstance(pOuter,iid);
			else
				pObject = T2::CreateInstance(iid);
		}
	};

	template <typename ROOT, const Omega::guid_t* pOID, const Omega::Activation::RegisterFlags_t flags = Omega::Activation::ProcessLocal | Omega::Activation::UserLocal | Omega::Activation::MultipleUse>
	class AutoObjectFactory
	{
	public:
		typedef AutoObjectFactoryImpl<AutoObjectFactoryCallCreate<AggregatedObjectImpl<ROOT>,pOID>,AutoObjectFactoryCallCreate<ObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

		static const Omega::guid_t* GetOid()
		{
			return pOID;
		}

		static const Omega::Activation::RegisterFlags_t GetRegistrationFlags()
		{
			return flags;
		}
	};

	template <typename ROOT, const Omega::guid_t* pOID, const Omega::Activation::RegisterFlags_t flags = Omega::Activation::ProcessLocal | Omega::Activation::UserLocal | Omega::Activation::MultipleUse>
	class AutoObjectFactorySingleton : public AutoObjectFactory<ROOT,pOID,flags>
	{
	public:
		typedef AutoObjectFactoryImpl<AutoObjectFactoryCallCreateThrow<pOID>,AutoObjectFactoryCallCreate<SingletonObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

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
				Omega::IObject* pObject = ObjectImpl<T>::CreateInstancePtr()->QueryInterface(iid);
				if (!pObject)
					throw Omega::INoInterfaceException::Create(iid);
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
		virtual void UnregisterObjectFactories();
		virtual void Run();

	protected:
		template <typename T>
		struct Creator
		{
			static Omega::IObject* Create(const Omega::guid_t& iid)
			{
				Omega::IObject* pObject = NoLockObjectImpl<T>::CreateInstancePtr()->QueryInterface(iid);
				if (!pObject)
					throw Omega::INoInterfaceException::Create(iid);
				return pObject;
			}
		};

		ProcessModule()
		{}
	};

	template <typename ROOT>
	class IProvideObjectInfoImpl :
			public Omega::TypeInfo::IProvideObjectInfo
	{
	private:
		Omega::TypeInfo::IProvideObjectInfo::iid_list_t WalkEntries(const ObjectBase::QIEntry* pEntries)
		{
			try
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
							ObjectPtr<Omega::IObject> ptrObj;
							ptrObj.Attach(pEntries[i].pfnQI(*(pEntries[i].pGuid),this,pEntries[i].offset-1,pEntries[i].pfnMemQI));
							if (ptrObj)
								retval.push_back(*(pEntries[i].pGuid));							
						}
					}
					else if (pEntries[i].offset != 0)
					{
						ObjectPtr<Omega::TypeInfo::IProvideObjectInfo> ptrAgg;
						ptrAgg.Attach(static_cast<Omega::TypeInfo::IProvideObjectInfo*>(pEntries[i].pfnQI(OMEGA_GUIDOF(Omega::TypeInfo::IProvideObjectInfo),this,pEntries[i].offset-1,pEntries[i].pfnMemQI)));
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
			catch (std::exception& e)
			{
				OMEGA_THROW(e.what());
			}
		}

	// IProvideObjectInfo members
	public:
		virtual Omega::TypeInfo::IProvideObjectInfo::iid_list_t EnumInterfaces()
		{
			return WalkEntries(ROOT::getQIEntries());
		}
	};
}

#include "OTL.inl"

#endif // OTL_H_INCLUDED_

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

#ifndef OTL_OTL_H_INCLUDED_
#define OTL_OTL_H_INCLUDED_

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
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIDelegate<iface,RootClass>, 0, 0 },

#define INTERFACE_ENTRY_IID(iid,iface) \
	{ &iid, &OTL::ObjectBase::QIDelegate<iface,RootClass>, 0, 0 },

#define INTERFACE_ENTRY2(iface,iface2) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIDelegate2<iface,iface2,RootClass>, 0, 0 },

#define INTERFACE_ENTRY2_IID(iid,iface,iface2) \
	{ &iid, &OTL::ObjectBase::QIDelegate2<iface,iface2,RootClass>, 0, 0 },

#define INTERFACE_ENTRY_CHAIN(baseClass) \
	{ &Omega::guid_t::Null(), &OTL::ObjectBase::QIChain<baseClass,RootClass>, 0, 0 },

#define INTERFACE_ENTRY_AGGREGATE(iface,member_object) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIAggregate, offsetof(RootClass,member_object), 0 },

#define INTERFACE_ENTRY_AGGREGATE_BLIND(member_object) \
	{ &Omega::guid_t::Null(), &OTL::ObjectBase::QIAggregate, offsetof(RootClass,member_object), 0 },

#define INTERFACE_ENTRY_FUNCTION(iface,pfn) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIFunction<RootClass>, 0, static_cast<OTL::ObjectBase::PFNMEMQI>(pfn) },

#define INTERFACE_ENTRY_FUNCTION_BLIND(pfn) \
	{ &Omega::guid_t::Null(), &OTL::ObjectBase::QIFunction<RootClass>, 0, static_cast<OTL::ObjectBase::PFNMEMQI>(pfn) },

#define INTERFACE_ENTRY_NOINTERFACE(iface) \
	{ &OMEGA_GUIDOF(iface), &OTL::ObjectBase::QIFail, 0, 0 },

#define END_INTERFACE_MAP() \
	{ 0,0,0,0 } }; return QIEntries; }

///////////////////////////////////////////////////////////////////
// Object map macros
//
// These are designed to be used as follows:
//
// For dynamic link libraries:
//
// BEGIN_LIBRARY_OBJECT_MAP()
//    OBJECT_MAP_ENTRY(class derived from AutoObjectFactory, Object name)
// END_LIBRARY_OBJECT_MAP()
//
// or, for Exe's
//
// BEGIN_PROCESS_OBJECT_MAP(L"app_name")
//    OBJECT_MAP_ENTRY(class derived from AutoObjectFactory, Object name)
// END_PROCESS_OBJECT_MAP()
//
// If "Object name" is NULL, then the object will not be registered
//
// If "module_name" is NULL, then no objects will be registered
//

#define BEGIN_LIBRARY_OBJECT_MAP() \
	namespace OTL { \
	namespace { \
	class LibraryModuleImpl : public LibraryModule \
	{ \
		ModuleBase::CreatorEntry* getCreatorEntries() { static ModuleBase::CreatorEntry CreatorEntries[] = {

#define OBJECT_MAP_ENTRY(obj,name) \
		{ &obj::GetOid, &obj::GetActivationFlags, &obj::GetRegistrationFlags, name, &Creator<obj::ObjectFactoryClass>::Create, 0 },

#define END_LIBRARY_OBJECT_MAP_NO_REGISTRATION() \
		{ 0,0,0,0,0,0 } }; return CreatorEntries; } \
	}; \
	} \
	OMEGA_PRIVATE LibraryModuleImpl* GetModule() { static LibraryModuleImpl i; return &i; } \
	OMEGA_PRIVATE ModuleBase* GetModuleBase() { return GetModule(); } \
	}

#define END_LIBRARY_OBJECT_MAP() \
	END_LIBRARY_OBJECT_MAP_NO_REGISTRATION() \
	OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_GetLibraryObject,4,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags,(in),const Omega::guid_t&,iid,(out)(iid_is(iid)),Omega::IObject*&,pObject)) \
	{ pObject = OTL::GetModule()->GetLibraryObject(oid,flags,iid); } \
	OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(Omega_RegisterLibrary,3,((in),Omega::bool_t,bInstall,(in),Omega::bool_t,bLocal,(in),const Omega::string_t&,strSubsts)) \
	{ OTL::GetModule()->RegisterLibrary(bInstall,bLocal,strSubsts); }

#define BEGIN_PROCESS_OBJECT_MAP(app_name) \
	namespace OTL { \
	namespace { \
	class ProcessModuleImpl : public ProcessModule \
	{ \
	public: \
		void RegisterObjects(Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strSubsts) \
			{ RegisterObjectsImpl(bInstall,bLocal,app_name,strSubsts); } \
	private: \
		ModuleBase::CreatorEntry* getCreatorEntries() { static ModuleBase::CreatorEntry CreatorEntries[] = {

#define END_PROCESS_OBJECT_MAP() \
		{ 0,0,0,0,0,0 } }; return CreatorEntries; } \
	}; \
	} \
	OMEGA_PRIVATE ProcessModuleImpl* GetModule() { static ProcessModuleImpl i; return &i; } \
	OMEGA_PRIVATE ModuleBase* GetModuleBase() { return GetModule(); } \
	}

#include <OOCore/OOCore.h>

namespace OTL
{
	template <class OBJECT>
	class ObjectPtrBase
	{
	public:
		explicit ObjectPtrBase(OBJECT* obj) :
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

		ObjectPtrBase(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter) :
			m_ptr(0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::CreateLocalInstance(oid,flags,pOuter,OMEGA_GUIDOF(OBJECT)));
		}

		ObjectPtrBase(const Omega::string_t& strURI, Omega::Activation::Flags_t flags, Omega::IObject* pOuter) :
			m_ptr(0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::CreateInstance(strURI,flags,pOuter,OMEGA_GUIDOF(OBJECT)));
		}

		virtual ~ObjectPtrBase()
		{
			Attach(0);
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
			m_ptr = 0;
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

		template <class Q>
		Q* QueryInterface()
		{
			return static_cast<Q*>(m_ptr->QueryInterface(OMEGA_GUIDOF(Q)));
		}

		OBJECT* operator ->() const
		{
			return m_ptr;
		}

		operator OBJECT* () const
		{
			return m_ptr;
		}

	protected:
		OBJECT* m_ptr;

	private:
		ObjectPtrBase& operator = (const ObjectPtrBase& rhs) { return *this; }
	};

	template <class OBJECT>
	class ObjectPtr : public ObjectPtrBase<OBJECT>
	{
	public:
		explicit ObjectPtr(OBJECT* obj = 0) :
		  ObjectPtrBase<OBJECT>(obj)
		{ }

		ObjectPtr(Omega::IObject* pObject) :
		  ObjectPtrBase<OBJECT>(0)
		{
			if (pObject)
				this->m_ptr = static_cast<OBJECT*>(pObject->QueryInterface(OMEGA_GUIDOF(OBJECT)));
		}

		ObjectPtr(const ObjectPtr<OBJECT>& rhs) :
		  ObjectPtrBase<OBJECT>(rhs)
		{ }

		ObjectPtr(const Omega::guid_t& oid, Omega::Activation::Flags_t flags = Omega::Activation::Any, Omega::IObject* pOuter = 0) :
		  ObjectPtrBase<OBJECT>(oid,flags,pOuter)
		{ }

		ObjectPtr(const Omega::string_t& name, Omega::Activation::Flags_t flags = Omega::Activation::Any, Omega::IObject* pOuter = 0) :
		  ObjectPtrBase<OBJECT>(name,flags,pOuter)
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
		ObjectPtr(Omega::IObject* obj = 0) :
		  ObjectPtrBase<Omega::IObject>(obj)
		{ }

		ObjectPtr(const ObjectPtr<Omega::IObject>& rhs) :
		  ObjectPtrBase<Omega::IObject>(rhs)
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

	class ObjectBase
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
			assert(m_refcount.m_debug_value > 0);

			if (m_refcount.Release())
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
		};

	#if !defined(__BORLANDC__)
	protected:
	#endif

		virtual Omega::IObject* Internal_QueryInterface(const Omega::guid_t& iid, const QIEntry* pEntries)
		{
			for (size_t i=0;pEntries && pEntries[i].pGuid!=0;++i)
			{
				if (*(pEntries[i].pGuid) == iid ||
					*(pEntries[i].pGuid) == Omega::guid_t::Null() ||
					iid == OMEGA_GUIDOF(Omega::IObject))
				{
					return pEntries[i].pfnQI(iid,this,pEntries[i].offset,pEntries[i].pfnMemQI);
				}
			}

			return 0;
		}

		template <class Interface, class Implementation>
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

		template <class Interface, class Interface2, class Implementation>
		static Omega::IObject* QIDelegate2(const Omega::guid_t&, void* pThis, size_t, ObjectBase::PFNMEMQI)
		{
			Interface* pI = static_cast<Interface*>(static_cast<Interface2*>(static_cast<Implementation*>(pThis)));
			pI->AddRef();
			return pI;
		}

		template <class Base, class Implementation>
		static Omega::IObject* QIChain(const Omega::guid_t& iid, void* pThis, size_t, ObjectBase::PFNMEMQI)
		{
			return static_cast<Implementation*>(pThis)->Internal_QueryInterface(iid,Base::getQIEntries());
		}

		static Omega::IObject* QIAggregate(const Omega::guid_t& iid, void* pThis, size_t offset, ObjectBase::PFNMEMQI)
		{
			return reinterpret_cast<Omega::IObject*>(reinterpret_cast<size_t>(pThis)+offset)->QueryInterface(iid);
		}

		template <class Implementation>
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
		inline bool HaveLocks() const;
		inline void IncLockCount();
		inline void DecLockCount();
		inline Omega::Threading::Mutex& GetLock();

		typedef void (*TERM_FUNC)(void* arg);
		inline void AddTermFunc(TERM_FUNC pfnTerm, void* arg);

	protected:
		ModuleBase() {}

		inline virtual ~ModuleBase();

		struct CreatorEntry
		{
			const Omega::guid_t* (*pfnOid)();
			const Omega::Activation::Flags_t (*pfnActivationFlags)();
			const Omega::Activation::RegisterFlags_t (*pfnRegistrationFlags)();
			const wchar_t* pszName;
			Omega::IObject* (*pfnCreate)(const Omega::guid_t& iid, Omega::Activation::Flags_t flags);
			Omega::uint32_t cookie;
		};

		virtual CreatorEntry* getCreatorEntries() = 0;
		inline void fini();

	private:
		Omega::Threading::Mutex          m_csMain;
		Omega::Threading::AtomicRefCount m_lockCount;

		struct Term
		{
			TERM_FUNC	pfn;
			void*		arg;
		};
		std::list<Term> m_listTerminators;
	};
	ModuleBase* GetModuleBase();

	template <class ROOT>
	class ObjectImpl : public ROOT
	{
	public:
		static ObjectImpl<ROOT>* CreateInstance(Omega::IObject* pOuter = 0)
		{
			if (pOuter)
				throw Omega::Activation::INoAggregationException::Create(Omega::guid_t::Null());

			ObjectImpl<ROOT>* pObject;
			OMEGA_NEW(pObject,ObjectImpl<ROOT>());
			return pObject;
		}

		static ObjectPtr<ObjectImpl<ROOT> > CreateInstancePtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<ObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateInstance(pOuter));
			return ptr;
		}

	private:
		ObjectImpl() : ROOT()
		{
			GetModuleBase()->IncLockCount();
			this->AddRef();
		}

		virtual ~ObjectImpl()
		{
			GetModuleBase()->DecLockCount();
		}

		ObjectImpl(const ObjectImpl& rhs)
		{}

		ObjectImpl& operator = (const ObjectImpl& rhs)
		{}

	// IObject members
	public:
		virtual void AddRef() { this->Internal_AddRef(); }
		virtual void Release() { this->Internal_Release(); }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <class ROOT>
	class NoLockObjectImpl : public ROOT
	{
	public:
		static NoLockObjectImpl<ROOT>* CreateInstance(Omega::IObject* pOuter = 0)
		{
			if (pOuter)
				throw Omega::Activation::INoAggregationException::Create(Omega::guid_t::Null());

			NoLockObjectImpl<ROOT>* pObject;
			OMEGA_NEW(pObject,NoLockObjectImpl<ROOT>());
			return pObject;
		}

		static ObjectPtr<NoLockObjectImpl<ROOT> > CreateInstancePtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<NoLockObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateInstance(pOuter));
			return ptr;
		}

	private:
		NoLockObjectImpl() : ROOT()
		{
			this->AddRef();
		}

	// IObject members
	public:
		virtual void AddRef() { this->Internal_AddRef(); }
		virtual void Release() { this->Internal_Release(); }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <class ROOT>
	class AggregatedObjectImpl;

	template <class ROOT>
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
		ContainedObjectImpl(const ContainedObjectImpl& rhs)
		{}

		ContainedObjectImpl& operator = (const ContainedObjectImpl& rhs)
		{}

		virtual ~ContainedObjectImpl()
		{
			Omega::System::UnpinObjectPointer(m_pOuter);
		}

	// IObject members
	public:
		virtual void AddRef() { m_pOuter->AddRef(); }
		virtual void Release() { m_pOuter->Release(); }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return m_pOuter->QueryInterface(iid);
		}
	};

	template <class ROOT>
	class AggregatedObjectImpl : public Omega::IObject
	{
		AggregatedObjectImpl(Omega::IObject* pOuter) : m_contained(pOuter)
		{
			AddRef();
			GetModuleBase()->IncLockCount();
		}

		virtual ~AggregatedObjectImpl()
		{
			GetModuleBase()->DecLockCount();
		}

		// If the line below is flagged as the source of a compiler warning then
		// you have missed out at least one virtual function in an interface that
		// <ROOT> derives from
		ContainedObjectImpl<ROOT>        m_contained;
		Omega::Threading::AtomicRefCount m_refcount;

	public:
		static AggregatedObjectImpl<ROOT>* CreateInstance(Omega::IObject* pOuter)
		{
			if (!pOuter)
				OMEGA_THROW(L"AggregatedObjectImpl must be aggregated");

			AggregatedObjectImpl<ROOT>* pObject;
			OMEGA_NEW(pObject,AggregatedObjectImpl<ROOT>(pOuter));
			return pObject;
		}

		static ObjectPtr<AggregatedObjectImpl<ROOT> > CreateInstancePtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<AggregatedObjectImpl<ROOT> > ptr;
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
			assert(m_refcount.m_debug_value > 0);

			if (m_refcount.Release())
				delete this;
		}

		Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			if (iid==OMEGA_GUIDOF(Omega::IObject))
			{
				AddRef();
				return this;
			}
			else
				return m_contained.Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <class ROOT>
	class SingletonObjectImpl : public ROOT
	{
		friend class Omega::Threading::Singleton<SingletonObjectImpl<ROOT> >;

	public:
		static SingletonObjectImpl<ROOT>* CreateInstance(Omega::IObject* = 0)
		{
			SingletonObjectImpl<ROOT>* pObject = Omega::Threading::Singleton<SingletonObjectImpl<ROOT> >::instance();
			pObject->AddRef();
			return pObject;
		}

		static ObjectPtr<SingletonObjectImpl<ROOT> > CreateInstancePtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<SingletonObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateInstance(pOuter));
			return ptr;
		}

	protected:
		SingletonObjectImpl() : ROOT()
		{ }

		virtual ~SingletonObjectImpl()
		{ }

	// IObject members
	public:
		virtual void AddRef() { GetModuleBase()->IncLockCount(); }
		virtual void Release() { GetModuleBase()->DecLockCount(); }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return Internal_QueryInterface(iid,ROOT::getQIEntries());
		}

	private:
		SingletonObjectImpl(const SingletonObjectImpl&) {}
		SingletonObjectImpl& operator = (const SingletonObjectImpl&) { return *this; }

		bool singleton_init()
		{
			GetModuleBase()->AddTermFunc(terminator,this);
			ROOT::Init();
			return true;
		}

		static void terminator(void* p)
		{
			delete static_cast<SingletonObjectImpl*>(p);
		}
	};

	template <class ROOT>
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
			return Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <class T, const Omega::guid_t* pOID>
	class ObjectFactoryCallCreate
	{
	public:
		static Omega::IObject* CreateInstance(Omega::IObject* pOuter, const Omega::guid_t& iid)
		{
			Omega::IObject* pObject = T::CreateInstancePtr(pOuter)->QueryInterface(iid);
			if (!pObject)
				throw Omega::INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
			return pObject;
		}
	};

	template <const Omega::guid_t* pOID>
	class ObjectFactoryCallCreateThrow
	{
	public:
		static Omega::IObject* CreateInstance(Omega::IObject*, const Omega::guid_t&)
		{
			throw Omega::Activation::INoAggregationException::Create(*pOID);
		}
	};

	template <class T1, class T2>
	class ObjectFactoryImpl :
		public ObjectBase,
		public Omega::Activation::IObjectFactory
	{
	public:
		BEGIN_INTERFACE_MAP(ObjectFactoryImpl)
			INTERFACE_ENTRY(Omega::Activation::IObjectFactory)
		END_INTERFACE_MAP()

	// IObjectFactory members
	public:
		void CreateInstance(Omega::IObject* pOuter, const Omega::guid_t& iid, Omega::IObject*& pObject)
		{
			if (pOuter)
				pObject = T1::CreateInstance(pOuter,iid);
			else
				pObject = T2::CreateInstance(0,iid);
		}
	};

	template <class ROOT, const Omega::guid_t* pOID = &Omega::guid_t::Null(), const Omega::Activation::Flags_t flags = Omega::Activation::Any, const Omega::Activation::RegisterFlags_t reg_flags = Omega::Activation::MultipleUse>
	class AutoObjectFactory
	{
	public:
		typedef ObjectFactoryImpl<ObjectFactoryCallCreate<AggregatedObjectImpl<ROOT>,pOID>,ObjectFactoryCallCreate<ObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

		static const Omega::guid_t* GetOid()
		{
			return pOID;
		}

		static const Omega::Activation::Flags_t GetActivationFlags()
		{
			return flags;
		}

		static const Omega::Activation::RegisterFlags_t GetRegistrationFlags()
		{
			return reg_flags;
		}
	};

	template <class ROOT, const Omega::guid_t* pOID, const Omega::Activation::Flags_t flags = Omega::Activation::Any, const Omega::Activation::RegisterFlags_t reg_flags = Omega::Activation::MultipleUse>
	class AutoObjectFactoryNoAggregation : public AutoObjectFactory<ROOT,pOID,flags,reg_flags>
	{
	public:
		typedef ObjectFactoryImpl<ObjectFactoryCallCreateThrow<pOID>,ObjectFactoryCallCreate<ObjectImpl<ROOT>,pOID> > ObjectFactoryClass;
	};

	template <class ROOT, const Omega::guid_t* pOID, const Omega::Activation::Flags_t flags = Omega::Activation::Any, const Omega::Activation::RegisterFlags_t reg_flags = Omega::Activation::MultipleUse>
	class AutoObjectFactorySingleton : public AutoObjectFactory<ROOT,pOID,flags,reg_flags>
	{
	public:
		typedef ObjectFactoryImpl<ObjectFactoryCallCreateThrow<pOID>,ObjectFactoryCallCreate<SingletonObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

	protected:
		virtual void Init() {}
	};

	class LibraryModule : public ModuleBase
	{
	public:
		template <class T>
		struct Creator
		{
			static Omega::IObject* Create(const Omega::guid_t& iid, Omega::Activation::Flags_t)
			{
				Omega::IObject* pObject = ObjectImpl<T>::CreateInstancePtr()->QueryInterface(iid);
				if (!pObject)
					throw Omega::INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
				return pObject;
			}
		};

		inline Omega::IObject* GetLibraryObject(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid);
		inline void RegisterLibrary(Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strSubsts);

	protected:
		LibraryModule()
		{}
	};

	class ProcessModule : public ModuleBase
	{
	public:
		// Register and unregister with the ROT
		inline void RegisterObjectFactories();
		inline void UnregisterObjectFactories();

		inline void Run();

	protected:
		template <class T>
		struct Creator
		{
			static Omega::IObject* Create(const Omega::guid_t& iid, Omega::Activation::Flags_t)
			{
				Omega::IObject* pObject = NoLockObjectImpl<T>::CreateInstancePtr()->QueryInterface(iid);
				if (!pObject)
					throw Omega::INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
				return pObject;
			}
		};

		ProcessModule()
		{}

		// Register and unregister with the OORegistry
		inline void RegisterObjectsImpl(Omega::bool_t bInstall, Omega::bool_t bLocal, const Omega::string_t& strAppName, const Omega::string_t& strSubsts);
	};

	template <class EnumIFace, class EnumType>
	class EnumSTL :
		public ObjectBase,
		public EnumIFace
	{
		typedef EnumSTL<EnumIFace,EnumType> MyType;

	public:
		template <class InputIterator>
		static EnumIFace* Create(InputIterator begin, InputIterator end)
		{
			try
			{
				ObjectPtr<ObjectImpl<MyType> > ptrThis = ObjectImpl<MyType>::CreateInstancePtr();
				ptrThis->m_listItems.assign(begin,end);
				ptrThis->m_pos = ptrThis->m_listItems.begin();
				return ptrThis.AddRef();
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}
		}

		void Init()
		{
			try
			{
				m_pos = m_listItems.begin();
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}
		}

		void Append(const EnumType& v)
		{
			try
			{
				m_listItems.push_back(v);
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}
		}

		bool Find(const EnumType& v)
		{
			try
			{
				for (typename std::list<EnumType>::const_iterator i=m_listItems.begin();i!=m_listItems.end();++i)
				{
					if (*i == v)
						return true;
				}
				return false;
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}
		}

		BEGIN_INTERFACE_MAP(MyType)
			INTERFACE_ENTRY(EnumIFace)
		END_INTERFACE_MAP()

	private:
		std::list<EnumType>                    m_listItems;
		typename std::list<EnumType>::iterator m_pos;
		Omega::Threading::Mutex                m_cs;

	// IEnumString members
	public:
		bool Next(Omega::uint32_t& count, EnumType* parrVals)
		{
			try
			{
				Omega::Threading::Guard<Omega::Threading::Mutex> guard(m_cs);

				Omega::uint32_t c = count;
				count = 0;
				while (m_pos!=m_listItems.end() && count < c)
				{
					parrVals[count] = *m_pos;
					++count;
					++m_pos;
				}

				return (m_pos!=m_listItems.end());
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}
		}

		bool Skip(Omega::uint32_t count)
		{
			try
			{
				Omega::Threading::Guard<Omega::Threading::Mutex> guard(m_cs);

				while (count > 0 && m_pos!=m_listItems.end())
				{
					++m_pos;
					--count;
				}

				return (m_pos!=m_listItems.end());
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}
		}

		void Reset()
		{
			try
			{
				Omega::Threading::Guard<Omega::Threading::Mutex> guard(m_cs);

				m_pos = m_listItems.begin();
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}
		}

		EnumIFace* Clone()
		{
			try
			{
				Omega::Threading::Guard<Omega::Threading::Mutex> guard(m_cs);

				ObjectPtr<ObjectImpl<MyType> > ptrNew = ObjectImpl<MyType>::CreateInstancePtr();
				ptrNew->m_listItems.assign(m_listItems.begin(),m_listItems.end());
				ptrNew->m_pos = ptrNew->m_listItems.begin();
				return ptrNew.AddRef();
			}
			catch (std::exception& e)
			{
				OMEGA_THROW(e);
			}
		}
	};

	typedef EnumSTL<Omega::IEnumString,Omega::string_t>	EnumString;
	typedef EnumSTL<Omega::IEnumGuid,Omega::guid_t>	EnumGuid;

	template <typename ROOT>
	class IProvideObjectInfoImpl :
		public Omega::TypeInfo::IProvideObjectInfo
	{
	// IProvideObjectInfo members
	public:
		virtual Omega::IEnumGuid* EnumInterfaces()
		{
			ObjectPtr<ObjectImpl<EnumGuid> > ptrEnum = ObjectImpl<EnumGuid>::CreateInstancePtr();

			const ObjectBase::QIEntry* pEntries = ROOT::getQIEntries();
			for (size_t i=0;pEntries && pEntries[i].pGuid!=0;++i)
			{
				if (*(pEntries[i].pGuid) != Omega::guid_t::Null())
				{
					if (*(pEntries[i].pGuid) != OMEGA_GUIDOF(Omega::TypeInfo::IProvideObjectInfo))
						ptrEnum->Append(*(pEntries[i].pGuid));
				}
				else
				{
					ObjectPtr<Omega::TypeInfo::IProvideObjectInfo> ptrPOI;
					ptrPOI.Attach(static_cast<Omega::TypeInfo::IProvideObjectInfo*>(pEntries[i].pfnQI(OMEGA_GUIDOF(Omega::TypeInfo::IProvideObjectInfo),this,pEntries[i].offset,pEntries[i].pfnMemQI)));
					if (ptrPOI)
					{
						// Add each entry in ptrPOI
						for (;;)
						{
							Omega::uint32_t count = 1;
							Omega::guid_t iid;
							ptrEnum->Next(count,&iid);
							if (count==0)
								break;

							if (!ptrEnum->Find(iid) && iid != OMEGA_GUIDOF(Omega::TypeInfo::IProvideObjectInfo))
								ptrEnum->Append(iid);
						}
					}
				}
			}

			ptrEnum->Init();
			return ptrEnum.AddRef();
		}
	};
}

#include <OTL/OTL.inl>

#endif // OTL_OTL_H_INCLUDED_

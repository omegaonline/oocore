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
	public: static const QIEntry* getQIEntries() {static const QIEntry QIEntries[] = {

#define INTERFACE_ENTRY(iface) \
	{ &Omega::MetaInfo::iid_traits<iface>::GetIID(), &QIDelegate<iface,RootClass>::QI, 0 },

#define INTERFACE_ENTRY_IID(iid,iface) \
	{ &iid, &QIDelegate<iface,RootClass>::QI, 0 },

#define INTERFACE_ENTRY2(iface,iface2) \
	{ &Omega::MetaInfo::iid_traits<iface>::GetIID(), &QIDelegate2<iface,iface2,RootClass>::QI, 0 },

#define INTERFACE_ENTRY2_IID(iid,iface,iface2) \
	{ &iid, &QIDelegate2<iface,iface2,RootClass>::QI, 0 },

#define INTERFACE_ENTRY_CHAIN(baseClass) \
	{ &Omega::guid_t::NIL, &QIChain<baseClass,RootClass>::QI, 0 },

#define INTERFACE_ENTRY_AGGREGATE(iid,member_object) \
	{ &iid, &QIAggregate::QI, reinterpret_cast<void*>(offsetof(RootClass,member_object)) },

#define INTERFACE_ENTRY_AGGREGATE_BLIND(member_object) \
	{ &Omega::guid_t::NIL, &QIAggregate::QI, reinterpret_cast<void*>(offsetof(RootClass,member_object)) },

#define INTERFACE_ENTRY_FUNCTION(iid,pfn,param) \
	{ &iid, &QIFunction<RootClass,pfn>::QI, param },

#define INTERFACE_ENTRY_FUNCTION_BLIND(pfn,param) \
	{ &Omega::guid_t::NIL, &QIFunction<RootClass,pfn>::QI, param },

#define INTERFACE_ENTRY_NOINTERFACE(iid) \
	{ &iid, &QIFail::QI, param },

#define END_INTERFACE_MAP() \
	{ 0 } }; return QIEntries; } \
	protected: virtual Omega::IObject* GetControllingObject() { \
	const QIEntry* g0 = RootClass::getQIEntries(); return g0->pfnQI(Omega::IID_IObject,this,g0->param); } \
	Omega::IObject* GetControllingObjectPtr() { \
	OTL::ObjectPtr<Omega::IObject> ptr; ptr.Attach(GetControllingObject()); return ptr; }

///////////////////////////////////////////////////////////////////
// Object map macros
//
// These are designed to be used as follows:
//
// For dynamic link libraries:
//
// BEGIN_LIBRARY_OBJECT_MAP(dll_name)
//    OBJECT_MAP_ENTRY(something derived from AutoObjectFactory)
// END_LIBRARY_OBJECT_MAP()
//
// or, for Exe's
//

#if defined(OTL_HAS_NONSTATICMODULE)
#define OTL_MODULE_INIT_BLOCK(name) \
	namespace OTL { \
	inline void ModuleInitialize() { name::instance(); } \
	inline void ModuleUninitialize() { name::fini(); } }
#else
#define OTL_MODULE_INIT_BLOCK(name) \
	namespace OTL { \
	inline void ModuleInitialize() {  } \
	inline void ModuleUninitialize() {  } \
	class ModuleStaticInitializer { \
	public: \
	ModuleStaticInitializer() { name::instance(); } \
	~ModuleStaticInitializer() { name::fini(); } \
	}; static ModuleStaticInitializer OMEGA_CONCAT(name,_static_instance__); }
#endif

#define BEGIN_LIBRARY_OBJECT_MAP(dll_name) \
	namespace { \
	class OMEGA_CONCAT(dll_name,_LibraryModule__); \
	typedef OTL::SingletonNoLock<OMEGA_CONCAT(dll_name,_LibraryModule__)> LibraryModule__; \
	class OMEGA_CONCAT(dll_name,_LibraryModule__) : public OTL::LibraryModule { \
		friend class OTL::SingletonNoLock<OMEGA_CONCAT(dll_name,_LibraryModule__)>; \
		const CreatorEntry* getCreatorEntries() const { static const CreatorEntry CreatorEntries[] = {

#define OBJECT_MAP_ENTRY(obj) \
	{ obj::GetOid, Creator<obj::ObjectFactoryClass>::Create },	

#define END_LIBRARY_OBJECT_MAP() \
		{ 0 } }; return CreatorEntries; } }; } \
	inline OTL::ModuleBase* OTL::GetModule() { return LibraryModule__::instance(); } \
	extern "C" OMEGA_EXPORT u_long OMEGA_CALL _get_dll_unload_policy() { return (LibraryModule__::instance()->GetLockCount()==0 ? /*ACE_DLL_UNLOAD_POLICY_DEFAULT*/ 1 : /*ACE_DLL_UNLOAD_POLICY_LAZY*/ 2); } \
	OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Activation::IObjectFactory*,Omega_GetObjectFactory,2,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags)) \
	{ return LibraryModule__::instance()->GetObjectFactory(oid,flags); } \
	OTL_MODULE_INIT_BLOCK(LibraryModule__)

#define BEGIN_PROCESS_OBJECT_MAP(process_class) \
	namespace { \
	class OMEGA_CONCAT(process_class,_Module__); \
	typedef OTL::SingletonNoLock<OMEGA_CONCAT(process_class,_Module__)> ProcessModule__; \
	class OMEGA_CONCAT(process_class,_Module__) : public process_class { \
		friend class OTL::SingletonNoLock<OMEGA_CONCAT(process_class,_Module__)>; \
		const CreatorEntry* getCreatorEntries() const { static const CreatorEntry CreatorEntries[] = {

#define END_PROCESS_OBJECT_MAP() \
		{ 0 } }; return CreatorEntries; } }; } \
	inline OTL::ModuleBase* OTL::GetModule() { return ProcessModule__::instance(); } \
	OTL_MODULE_INIT_BLOCK(ProcessModule__)

#include <OOCore/OOCore.h>

namespace OTL 
{
	template <class OBJECT>
	class ObjectPtrBase
	{
	public:
		ObjectPtrBase(OBJECT* obj) :
			m_ptr(obj)
		{
			if (m_ptr.value())
				m_ptr.value()->AddRef();
		}

		ObjectPtrBase(const ObjectPtrBase<OBJECT>& rhs) :
			m_ptr(rhs.m_ptr)
		{
			if (m_ptr.value())
				m_ptr.value()->AddRef();
		}

		ObjectPtrBase(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter) :
			m_ptr(0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::Activation::CreateObject(oid,flags,pOuter,Omega::MetaInfo::iid_traits<OBJECT>::GetIID()));
		}

		ObjectPtrBase(const Omega::string_t& object_name, Omega::Activation::Flags_t flags, Omega::IObject* pOuter) :
			m_ptr(0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::Activation::CreateObject(Omega::Activation::NameToOid(object_name),flags,pOuter,Omega::MetaInfo::iid_traits<OBJECT>::GetIID()));
		}

		virtual ~ObjectPtrBase()
		{
			if (m_ptr.value())
				m_ptr.value()->Release();
		}

		ObjectPtrBase& operator = (const ObjectPtrBase& rhs)
		{
			OBJECT* ptr = rhs;
			if (ptr)
				ptr->AddRef();

			OBJECT* old = m_ptr.exchange(ptr);
			
			if (old)
				old->Release();

			return *this;
		}

		ObjectPtrBase& operator = (OBJECT* ptr)
		{
			if (ptr)
				ptr->AddRef();

			OBJECT* old = m_ptr.exchange(ptr);

			if (old)
				old->Release();

			return *this;
		}

		bool operator ! () const
		{
			return (m_ptr.value() == 0);
		}

		bool operator == (Omega::IObject* rhs) const
		{
			ObjectPtr<Omega::IObject*> pObj1(rhs);
			ObjectPtr<Omega::IObject*> pObj2(*this);

			return (pObj1 == pObj2);
		}

		OBJECT* AddRefReturn()
		{
			if (m_ptr.value())
				m_ptr.value()->AddRef();
			
			return m_ptr.value();
		}

		void Attach(OBJECT* obj)
		{
			OBJECT* old = m_ptr.exchange(obj);
			if (old)
				old->Release();
		}

		OBJECT* Detach()
		{
			return m_ptr.exchange(0);
		}

		void Release()
		{
			Attach(0);
		}

		void CreateObject(const Omega::guid_t& oid, Omega::Activation::Flags_t flags = Omega::Activation::Any, Omega::IObject* pOuter = 0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::Activation::CreateObject(oid,flags,pOuter,Omega::MetaInfo::iid_traits<OBJECT>::GetIID()));
		}

		void CreateObject(const Omega::string_t& object_name, Omega::Activation::Flags_t flags = Omega::Activation::Any, Omega::IObject* pOuter = 0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::Activation::CreateObject(Omega::Activation::NameToOid(object_name),flags,pOuter,Omega::MetaInfo::iid_traits<OBJECT>::GetIID()));
		}

		OBJECT* operator ->()
		{
			return m_ptr.value();
		}

		OBJECT** operator &()
		{
			return &m_ptr;
		}

		operator OBJECT*&()
		{
			return m_ptr.value();
		}

		operator OBJECT*() const
		{
			return m_ptr.value();
		}
	
	protected:
		typename Omega::AtomicOp<OBJECT*>::type m_ptr;
	};

	template <class OBJECT>
	class ObjectPtr : public ObjectPtrBase<OBJECT>
	{
	public:
		ObjectPtr(OBJECT* obj = 0) : 
		  ObjectPtrBase<OBJECT>(obj)
		{ }

		ObjectPtr(Omega::IObject* pObject) :
		  ObjectPtrBase<OBJECT>(0)
		{
			if (pObject)
				m_ptr = static_cast<OBJECT*>(pObject->QueryInterface(Omega::MetaInfo::iid_traits<OBJECT>::GetIID()));
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

		ObjectPtr(const Omega::guid_t& oid, Omega::Activation::Flags_t flags = Omega::Activation::Any, Omega::IObject* pOuter = 0) :
		  ObjectPtrBase<Omega::IObject>(oid,flags,pOuter)
		{ }

		ObjectPtr(const Omega::string_t& name, Omega::Activation::Flags_t flags = Omega::Activation::Any, Omega::IObject* pOuter = 0) :
		  ObjectPtrBase<Omega::IObject>(name,flags,pOuter)
		{ }
	};

    class ObjectBase
	{
	protected:
		ObjectBase() : m_refcount(0)
		{ }

		virtual ~ObjectBase()
		{}

		void Internal_AddRef()
		{
			++m_refcount;
		}

		void Internal_Release()
		{
			if (--m_refcount==0)
				delete this;
		}

		struct QIEntry
		{
			const Omega::guid_t* pGuid;
            Omega::IObject* (*pfnQI)(const Omega::guid_t& iid, void* pThis, void* param);
			void* param;
		};

		template <class Interface, class Implementation>
		struct QIDelegate
		{
			static Omega::IObject* QI(const Omega::guid_t&, void* pThis, void*)
			{
				return static_cast<Interface*>(static_cast<Implementation*>(pThis));
			}
		};

		template <class Interface, class Interface2, class Implementation>
		struct QIDelegate2
		{
			static Omega::IObject* QI(const Omega::guid_t&, void* pThis, void*)
			{
				return static_cast<Interface*>(static_cast<Interface2*>(static_cast<Implementation*>(pThis)));
			}
		};

		template <class Base, class Implementation>
		struct QIChain
		{
			static Omega::IObject* QI(const Omega::guid_t& iid, void* pThis, void*)
			{
				return static_cast<Implementation*>(pThis)->Internal_QueryInterface(iid,Base::getQIEntries());
			}
		};

		struct QIAggregate
		{
			static Omega::IObject* QI(const Omega::guid_t& iid, void* pThis, void* param)
			{
				return reinterpret_cast<Omega::IObject*>(reinterpret_cast<size_t>(pThis)+reinterpret_cast<size_t>(param))->QueryInterface(iid);
			}
		};

		template <class Implementation, Omega::IObject* (Implementation::*pfnQI)(const Omega::guid_t& iid, void* param)>
		struct QIFunction
		{
			static Omega::IObject* QI(const Omega::guid_t& iid, void* pThis, void* param)
			{
				return (static_cast<Implementation*>(pThis)->*pfnQI)(iid,param);
			}
		};

		struct QIFail
		{
			static Omega::IObject* QI(const Omega::guid_t&, void*, void*)
			{
				return 0;
			}
		};

		virtual Omega::IObject* GetControllingObject() = 0;

		Omega::IObject* Internal_QueryInterface(const Omega::guid_t& iid, const QIEntry* pEntries)
		{
			for (size_t i=0;pEntries && pEntries[i].pGuid!=0;++i)
			{
				if (*(pEntries[i].pGuid) == iid ||
					*(pEntries[i].pGuid) == Omega::guid_t::NIL ||
					iid == Omega::IID_IObject)
				{
					Omega::IObject* pObj = pEntries[i].pfnQI(iid,this,pEntries[i].param);
					if (pObj)
						pObj->AddRef();
					return pObj;
				}
			}
			
			return 0;
		}
		
	private:
		Omega::AtomicOp<long>::type m_refcount;
	};

	template <class E>
	class ExceptionImpl :
		public ObjectBase,
		public E
	{
	public:
		ObjectPtr<Omega::IException>	m_ptrCause;
		Omega::string_t					m_strDesc;
		Omega::string_t					m_strSource;

		BEGIN_INTERFACE_MAP(ExceptionImpl)
			INTERFACE_ENTRY(Omega::IException)
			INTERFACE_ENTRY(E)
		END_INTERFACE_MAP()

	// IException members
	public:
		virtual Omega::guid_t GetActualIID()
		{
			return Omega::MetaInfo::iid_traits<E>::GetIID();
		}
		virtual Omega::IException* Cause()
		{
			return m_ptrCause.AddRefReturn();
		}
		virtual Omega::string_t Description()
		{
			return m_strDesc;
		}
		virtual Omega::string_t Source()
		{
			return m_strSource;
		}
	};

	class ModuleBase
	{
	public:
		size_t GetLockCount() const;
		void IncLockCount();
		void DecLockCount();
		Omega::Guard<Omega::CriticalSection> GetGuard();

		typedef void (*TERM_FUNC)(void* arg);
		void AddTermFunc(TERM_FUNC pfnTerm, void* arg);
		
	protected:
		ModuleBase() : 
			m_lockCount(0)
		{ }

		~ModuleBase();

		struct CreatorEntry
		{
			const Omega::guid_t* (*pfnOid)();
			Omega::IObject* (*pfnCreate)(const Omega::guid_t& iid, Omega::Activation::Flags_t flags);
		};

		virtual const CreatorEntry* getCreatorEntries() const = 0;

	private:
		Omega::CriticalSection			m_csMain;
		Omega::AtomicOp<long>::type		m_lockCount;

		struct Term
		{
			TERM_FUNC	pfn;
			void*		arg;
		};
		std::list<Term> m_listTerminators;

#if !defined(OTL_HAS_NONSTATICMODULE)
		friend class ModuleStaticInitializer;
#endif
		void fini();
	};

	extern ModuleBase* GetModule();

	template <class ROOT>
	class ObjectImpl : public ROOT
	{
	public:
		static ObjectImpl<ROOT>* CreateObject(Omega::IObject* pOuter = 0)
		{
			if (pOuter)
				Omega::IException::Throw("ObjectImpl does not support aggregation",OMEGA_FUNCNAME);
				
			ObjectImpl<ROOT>* pObject;
			OMEGA_NEW(pObject,ObjectImpl<ROOT>());
			return pObject;
		}

		static ObjectPtr<ObjectImpl<ROOT> > CreateObjectPtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<ObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateObject(pOuter));
			return ptr;
		}

		ObjectImpl() : ROOT()
		{ 
			GetModule()->IncLockCount();
			this->AddRef();
		}

	private:
		virtual ~ObjectImpl()
		{
			GetModule()->DecLockCount();
		}

		ObjectImpl(const ObjectImpl& rhs)
		{}

		ObjectImpl& operator = (const ObjectImpl& rhs)
		{}

	// IObject members
	public:
		virtual void AddRef() { Internal_AddRef(); }
		virtual void Release() { Internal_Release(); }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid) 
		{
			return Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <class ROOT>
	class NoLockObjectImpl : public ROOT
	{
	public:
		static NoLockObjectImpl<ROOT>* CreateObject()
		{
			ObjectImpl<ROOT>* pObject;
			OMEGA_NEW(pObject,ObjectImpl<ROOT>());
			return pObject;
		}

		static ObjectPtr<NoLockObjectImpl<ROOT> > CreateObjectPtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<NoLockObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateObject(pOuter));
			return ptr;
		}

	protected:
		virtual Omega::IObject* GetControllingObject()
		{
			return this;
		}

	private:
		NoLockObjectImpl() : ROOT()
		{ 
			pObject->AddRef();
		}

	// IObject members
	public:
		virtual void AddRef() { Internal_AddRef(); }
		virtual void Release() { Internal_Release(); }
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
			m_ptrOuter(pOuter)
		{ }

		ObjectPtr<Omega::IObject> m_ptrOuter;

		virtual Omega::IObject* GetControllingObject()
		{
			return m_ptrOuter;
		}

	private:
		ContainedObjectImpl(const ContainedObjectImpl& rhs)
		{}

		ContainedObjectImpl& operator = (const ContainedObjectImpl& rhs)
		{}

	// IObject members
	public:
		virtual void AddRef() { m_ptrOuter->AddRef(); }
		virtual void Release() { m_ptrOuter->Release(); }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			return m_ptrOuter->QueryInterface(iid);
		}
	};

	template <class ROOT>
	class AggregatedObjectImpl : public Omega::IObject
	{
		AggregatedObjectImpl(Omega::IObject* pOuter) : m_contained(pOuter), m_refcount(1)
		{ 
			GetModule()->IncLockCount();
		}

		virtual ~AggregatedObjectImpl()
		{
			GetModule()->DecLockCount();
		}

		// If the line below is flagged as the source of a compiler warning then 
		// you have missed out at least one virtual function in an interface that
		// <ROOT> derives from
		ContainedObjectImpl<ROOT>		m_contained;
		Omega::AtomicOp<long>::type		m_refcount;

	public:
		static AggregatedObjectImpl<ROOT>* CreateObject(Omega::IObject* pOuter)
		{
			if (!pOuter)
				Omega::IException::Throw("AggregatedObjectImpl must be aggregated",OMEGA_FUNCNAME);

			AggregatedObjectImpl<ROOT>* pObject;
			OMEGA_NEW(pObject,AggregatedObjectImpl<ROOT>(pOuter));
			return pObject;
		}

		static ObjectPtr<AggregatedObjectImpl<ROOT> > CreateObjectPtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<AggregatedObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateObject(pOuter));
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
			++m_refcount;
		}

		virtual void Release()
		{
			if (--m_refcount==0)
				delete this;
		}

		Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			if (iid==Omega::IID_IObject)
			{
				++m_refcount;
				return this;
			}
			else
				return m_contained.Internal_QueryInterface(iid,ROOT::getQIEntries());
		}		
	};

	template <class TYPE>
	class Singleton
	{
	public:
		// Global access point to the Singleton.
		static TYPE *instance(void)
		{
			Singleton<TYPE>*& singleton = Singleton<TYPE>::instance_i();
			if (!singleton)
			{
				Guard<CriticalSection> lock(GetModule()->GetGuard());
				if (!singleton)
				{
					OMEGA_NEW(singleton,Singleton<TYPE>());
					GetModule()->AddTermFunc(delete_this,singleton);
				}
			}

			return &singleton->m_instance;
		}

	protected:
		TYPE m_instance;

		Singleton() {}
		virtual ~Singleton() {}
		Singleton(const Singleton&) {}
		Singleton& operator = (const Singleton&) {}
		
		static Singleton<TYPE>*& instance_i()
		{
			static Singleton<TYPE>* singleton = 0;
			return singleton;
		}

		static void delete_this(void* pThis)
		{
			delete static_cast<Singleton<TYPE>*>(pThis);
			instance_i() = 0;
		}
	};

	template <class ROOT>
	class SingletonObjectImpl : public ROOT
	{
		friend class Singleton<SingletonObjectImpl<ROOT> >;
		typedef Singleton<SingletonObjectImpl<ROOT> > singleton;

	public:
		static SingletonObjectImpl<ROOT>* CreateObject(Omega::IObject* = 0)
		{
			SingletonObjectImpl<ROOT>* pObject = singleton::instance();
			pObject->AddRef();
			return pObject;
		}

		static ObjectPtr<SingletonObjectImpl<ROOT> > CreateObjectPtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<SingletonObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateObject(pOuter));
			return ptr;
		}

	/*protected:
		virtual Omega::IObject* GetControllingObject()
		{
			return this;
		}	*/

	protected:
		SingletonObjectImpl() : ROOT()
		{ }

		virtual ~SingletonObjectImpl()
		{ }

	// IObject members
	public:
		virtual void AddRef() { GetModule()->IncLockCount(); }
		virtual void Release() { GetModule()->DecLockCount(); }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid) 
		{
			return Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	template <class ROOT>
	class StackObjectImpl : public ROOT
	{
	public:
		StackObjectImpl() : ROOT()
		{ }

	protected:
		virtual Omega::IObject* GetControllingObject()
		{
			return this;
		}

	// IObject members
	public:
		virtual void AddRef() { }
		virtual void Release() { }
		virtual Omega::IObject* QueryInterface(const Omega::guid_t& iid) 
		{
			return Internal_QueryInterface(iid,ROOT::getQIEntries());
		}
	};

	class LibraryModule : public ModuleBase
	{
	public:
		template <class T> 
		struct Creator 
		{ 
			static Omega::IObject* Create(const Omega::guid_t& iid, Omega::Activation::Flags_t)
			{
				Omega::IObject* pObject = OTL::ObjectImpl<T>::CreateObjectPtr()->QueryInterface(iid); 
				if (!pObject)
					Omega::INoInterfaceException::Throw(iid,OMEGA_FUNCNAME);
				return pObject;
			}
		};

		Omega::Activation::IObjectFactory* GetObjectFactory(const Omega::guid_t& oid, Omega::Activation::Flags_t flags);

	protected:
		LibraryModule()
		{}
	};

	class ProcessModule : public ModuleBase
	{
	protected:
		template <class T>
		struct Creator
		{
			static Omega::IObject* Create(const Omega::guid_t& iid, Omega::Activation::Flags_t)
			{
				Omega::IObject* pObject = OTL::NoLockObjectImpl<T>::CreateObjectPtr()->QueryInterface(iid); 
				if (!pObject)
					Omega::INoInterfaceException::Throw(iid,OMEGA_FUNCNAME);
				return pObject;
			}
		};

		ProcessModule()
		{}
	};

	template <class T, const Omega::guid_t* pOID>
	class ObjectFactoryCallCreate
	{
	public:
		static Omega::IObject* CreateObject(Omega::IObject* pOuter, const Omega::guid_t& iid)
		{
			Omega::IObject* pObject = T::CreateObjectPtr(pOuter)->QueryInterface(iid);
			if (!pObject)
				Omega::INoInterfaceException::Throw(iid,OMEGA_FUNCNAME);
			return pObject;
		}
	};

	template <const Omega::guid_t* pOID>
	class ObjectFactoryCallCreate<bool,pOID>
	{
	public:
		static Omega::IObject* CreateObject(Omega::IObject*, const Omega::guid_t&)
		{
			Omega::Activation::INoAggregationException::Throw(*pOID);
			return 0;
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
		void CreateObject(Omega::IObject* pOuter, const Omega::guid_t& iid, Omega::IObject*& pObject)
		{
			if (pOuter)
				pObject = T1::CreateObject(pOuter,iid);
			else
				pObject = T2::CreateObject(0,iid);
		}
	};

	template <class ROOT, const Omega::guid_t* pOID>
	class AutoObjectFactory
	{
	public:
        typedef ObjectFactoryImpl<ObjectFactoryCallCreate<AggregatedObjectImpl<ROOT>,pOID>,ObjectFactoryCallCreate<ObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

		static const Omega::guid_t* GetOid()
		{
			return pOID;
		}
	};

	template <class ROOT, const Omega::guid_t* pOID>
	class AutoObjectFactoryNoAggregation
	{
	public:
        typedef ObjectFactoryImpl<ObjectFactoryCallCreate<bool,pOID>,ObjectFactoryCallCreate<ObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

		static const Omega::guid_t* GetOid()
		{
			return pOID;
		}
	};

	template <class ROOT, const Omega::guid_t* pOID>
	class AutoObjectFactorySingleton
	{
	public:
        typedef ObjectFactoryImpl<ObjectFactoryCallCreate<bool,pOID>,ObjectFactoryCallCreate<SingletonObjectImpl<ROOT>,pOID> > ObjectFactoryClass;

		static const Omega::guid_t* GetOid()
		{
			return pOID;
		}
	};

	template <class TYPE>
	class SingletonNoLock
	{
	public:
		// Global access point to the Singleton.
		static TYPE *instance(void)
		{
			SingletonNoLock<TYPE>*& singleton = SingletonNoLock<TYPE>::instance_i();
			if (!singleton)
			{
				singleton = new SingletonNoLock<TYPE>();
				if (!singleton)
					return 0;
			}

			return &singleton->m_instance;
		}

		static void fini()
		{
			SingletonNoLock<TYPE>*& singleton = SingletonNoLock<TYPE>::instance_i();
			delete singleton;
			singleton = 0;
		}

	protected:
		TYPE m_instance;

		SingletonNoLock() {}

		static SingletonNoLock<TYPE>*& instance_i()
		{
			static SingletonNoLock<TYPE>* singleton = 0;
			return singleton;
		}
	};	
};

#include <OTL/OTL.inl>

// Specialisations
#include <OTL/OTL_special.h>

#endif // OTL_OTL_H_INCLUDED_

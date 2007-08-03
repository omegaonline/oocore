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
	{ &OMEGA_UUIDOF(iface), &QIDelegate<iface,RootClass>, 0 },

#define INTERFACE_ENTRY_IID(iid,iface) \
	{ &iid, &QIDelegate<iface,RootClass>, 0 },

#define INTERFACE_ENTRY2(iface,iface2) \
	{ &OMEGA_UUIDOF(iface), &QIDelegate2<iface,iface2,RootClass>, 0 },

#define INTERFACE_ENTRY2_IID(iid,iface,iface2) \
	{ &iid, &QIDelegate2<iface,iface2,RootClass>, 0 },

#define INTERFACE_ENTRY_CHAIN(baseClass) \
	{ &Omega::guid_t::Null(), &QIChain<baseClass,RootClass>, 0 },

#define INTERFACE_ENTRY_AGGREGATE(iid,member_object) \
	{ &iid, &QIAggregate, reinterpret_cast<void*>(offsetof(RootClass,member_object)) },

#define INTERFACE_ENTRY_AGGREGATE_BLIND(member_object) \
	{ &Omega::guid_t::Null(), &QIAggregate, reinterpret_cast<void*>(offsetof(RootClass,member_object)) },

#define INTERFACE_ENTRY_FUNCTION(iid,pfn,param) \
	{ &iid, &pfn, param },

#define INTERFACE_ENTRY_FUNCTION_BLIND(pfn) \
	{ &Omega::guid_t::Null(), &pfn, 0 },

#define INTERFACE_ENTRY_NOINTERFACE(iid) \
	{ &iid, &QIFail, param },

#define END_INTERFACE_MAP() \
	{ 0,0,0 } }; return QIEntries; }
	/*protected: virtual Omega::IObject* GetControllingObject() { \
	const QIEntry* g0 = RootClass::getQIEntries(); return g0->pfnQI(OMEGA_UUIDOF(Omega::IObject),this,g0->param); } \
	Omega::IObject* GetControllingObjectPtr() { \
	OTL::ObjectPtr<Omega::IObject> ptr; ptr.Attach(GetControllingObject()); return ptr; } */

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
// BEGIN_PROCESS_OBJECT_MAP(dll_name)
//    OBJECT_MAP_ENTRY(something derived from AutoObjectFactory)
// END_PROCESS_OBJECT_MAP()
//

#if defined(OTL_HAS_NONSTATICMODULE)
#define OTL_MODULE_INIT_BLOCK(name) \
	namespace OTL { \
	void ModuleInitialize() { name::instance(); } \
	void ModuleUninitialize() { name::fini(); } }
#else
#define OTL_MODULE_INIT_BLOCK(name) \
	namespace OTL { \
	void ModuleInitialize() {  } \
	void ModuleUninitialize() {  } \
	class ModuleStaticInitializer { \
	public: \
	ModuleStaticInitializer() { name::instance(); } \
	~ModuleStaticInitializer() { name::fini(); } \
	}; static ModuleStaticInitializer OMEGA_CONCAT(name,_static_instance__); }
#endif

#define BEGIN_LIBRARY_OBJECT_MAP(dll_name) \
	namespace OTL { \
	class OMEGA_CONCAT(dll_name,_LibraryModule__); \
	typedef OTL::SingletonNoLock<OMEGA_CONCAT(dll_name,_LibraryModule__)> LibraryModule__; \
	class OMEGA_CONCAT(dll_name,_LibraryModule__) : public OTL::LibraryModule { \
		friend class OTL::SingletonNoLock<OMEGA_CONCAT(dll_name,_LibraryModule__)>; \
		const CreatorEntry* getCreatorEntries() const { static const CreatorEntry CreatorEntries[] = {

#define OBJECT_MAP_ENTRY(obj) \
	{ obj::GetOid, Creator<obj::ObjectFactoryClass>::Create },

#define END_LIBRARY_OBJECT_MAP() \
		{ 0,0 } }; return CreatorEntries; } }; } \
	OTL::ModuleBase* OTL::GetModule() { return LibraryModule__::instance(); } \
	extern "C" OMEGA_EXPORT unsigned long OMEGA_CALL _get_dll_unload_policy() { return (OTL::LibraryModule__::instance()->GetLockCount()==0 ? /*ACE_DLL_UNLOAD_POLICY_DEFAULT*/ 1 : /*ACE_DLL_UNLOAD_POLICY_LAZY*/ 2); } \
	OMEGA_DEFINE_EXPORTED_FUNCTION(Omega::Activation::IObjectFactory*,Omega_GetObjectFactory,2,((in),const Omega::guid_t&,oid,(in),Omega::Activation::Flags_t,flags)) \
	{ return OTL::LibraryModule__::instance()->GetObjectFactory(oid,flags); } \
	OTL_MODULE_INIT_BLOCK(LibraryModule__)


// THIS ALL NEEDS TO BE CHANGED TO USE THE SERVICE TABLE
#define BEGIN_PROCESS_OBJECT_MAP(process_class) \
	namespace OTL { \
	class OMEGA_CONCAT(process_class,_Module__); \
	typedef OTL::SingletonNoLock<OMEGA_CONCAT(process_class,_Module__)> ProcessModule__; \
	class OMEGA_CONCAT(process_class,_Module__) : public process_class { \
		friend class OTL::SingletonNoLock<OMEGA_CONCAT(process_class,_Module__)>; \
		const CreatorEntry* getCreatorEntries() const { static const CreatorEntry CreatorEntries[] = {

#define END_PROCESS_OBJECT_MAP() \
		{ 0,0 } }; return CreatorEntries; } }; } \
	OTL::ModuleBase* OTL::GetModule() { return ProcessModule__::instance(); } \
	OTL_MODULE_INIT_BLOCK(ProcessModule__)

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
			if (m_ptr.value())
				m_ptr.value()->AddRef();
		}

		ObjectPtrBase(const ObjectPtrBase& rhs) :
			m_ptr(rhs.m_ptr)
		{
			if (m_ptr.value())
				m_ptr.value()->AddRef();
		}

		ObjectPtrBase(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, Omega::IObject* pOuter) :
			m_ptr(0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::CreateInstance(oid,flags,pOuter,OMEGA_UUIDOF(OBJECT)));
		}

		ObjectPtrBase(const Omega::string_t& object_name, Omega::Activation::Flags_t flags, Omega::IObject* pOuter) :
			m_ptr(0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::CreateInstance(Omega::Activation::NameToOid(object_name),flags,pOuter,OMEGA_UUIDOF(OBJECT)));
		}

		virtual ~ObjectPtrBase()
		{
			if (m_ptr.value())
				m_ptr.value()->Release();
		}

		ObjectPtrBase& operator = (const ObjectPtrBase& rhs)
		{
			return this->operator = (static_cast<OBJECT*>(rhs));
		}

		ObjectPtrBase& operator = (OBJECT* ptr)
		{
			if (ptr != m_ptr.value())
			{
				if (ptr)
					ptr->AddRef();

				OBJECT* old = m_ptr.exchange(ptr);

				if (old)
					old->Release();
			}

			return *this;
		}

		bool operator ! () const
		{
			return (m_ptr.value() == 0);
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

		void CreateInstance(const Omega::guid_t& oid, Omega::Activation::Flags_t flags = Omega::Activation::Any, Omega::IObject* pOuter = 0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::CreateInstance(oid,flags,pOuter,OMEGA_UUIDOF(OBJECT)));
		}

		void CreateInstance(const Omega::string_t& object_name, Omega::Activation::Flags_t flags = Omega::Activation::Any, Omega::IObject* pOuter = 0)
		{
			m_ptr = static_cast<OBJECT*>(Omega::CreateInstance(Omega::Activation::NameToOid(object_name),flags,pOuter,OMEGA_UUIDOF(OBJECT)));
		}

		OBJECT* operator ->()
		{
			return m_ptr.value();
		}

		OBJECT* volatile * operator &()
		{
			return &m_ptr;
		}

		operator OBJECT* volatile &()
		{
			return m_ptr.value();
		}

		operator OBJECT*() const
		{
			return m_ptr.value();
		}

	protected:
		Omega::System::AtomicOp<OBJECT*> m_ptr;
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
				this->m_ptr = static_cast<OBJECT*>(pObject->QueryInterface(OMEGA_UUIDOF(OBJECT)));
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

		#if defined(__BORLANDC__)
		public:
		#endif
		Omega::IObject* Internal_QueryInterface(const Omega::guid_t& iid, const QIEntry* pEntries)
		{
			for (size_t i=0;pEntries && pEntries[i].pGuid!=0;++i)
			{
				if (*(pEntries[i].pGuid) == iid ||
					*(pEntries[i].pGuid) == Omega::guid_t::Null() ||
					iid == OMEGA_UUIDOF(Omega::IObject))
				{
					Omega::IObject* pObj = pEntries[i].pfnQI(iid,this,pEntries[i].param);
					if (pObj)
						pObj->AddRef();
					return pObj;
				}
			}

			return 0;
		}
		#if defined(__BORLANDC__)
		protected:
		#endif

		template <class Interface, class Implementation>
        static Omega::IObject* QIDelegate(const Omega::guid_t&, void* pThis, void*)
        {
            return static_cast<Interface*>(static_cast<Implementation*>(pThis));
        }

		template <class Interface, class Interface2, class Implementation>
		static Omega::IObject* QIDelegate2(const Omega::guid_t&, void* pThis, void*)
        {
			return static_cast<Interface*>(static_cast<Interface2*>(static_cast<Implementation*>(pThis)));
		}

        template <class Base, class Implementation>
        static Omega::IObject* QIChain(const Omega::guid_t& iid, void* pThis, void*)
        {
            return static_cast<Implementation*>(pThis)->Internal_QueryInterface(iid,Base::getQIEntries());
        }

        static Omega::IObject* QIAggregate(const Omega::guid_t& iid, void* pThis, void* param)
        {
            return reinterpret_cast<Omega::IObject*>(reinterpret_cast<size_t>(pThis)+reinterpret_cast<size_t>(param))->QueryInterface(iid);
        }

        template <class Implementation, Omega::IObject* (Implementation::*Method)(const Omega::guid_t&)>
        static Omega::IObject* QIFunction(const Omega::guid_t& iid, void* pThis, void* pfn)
        {
            return (static_cast<Implementation*>(pThis)->*Method)(iid);
        }

        static Omega::IObject* QIFail(const Omega::guid_t&, void*, void*)
        {
            return 0;
        }

		//virtual Omega::IObject* GetControllingObject() = 0;

	private:
		Omega::System::AtomicOp<Omega::uint32_t> m_refcount;
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
		virtual Omega::guid_t ActualIID()
		{
			return OMEGA_UUIDOF(E);
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
		inline size_t GetLockCount() const;
		inline void IncLockCount();
		inline void DecLockCount();
		inline Omega::System::CriticalSection& GetLock();

		typedef void (*TERM_FUNC)(void* arg);
		inline void AddTermFunc(TERM_FUNC pfnTerm, void* arg);

	protected:
		ModuleBase() :
			m_lockCount(0)
		{ }

		inline virtual ~ModuleBase();

		struct CreatorEntry
		{
			const Omega::guid_t* (*pfnOid)();
			Omega::IObject* (*pfnCreate)(const Omega::guid_t& iid, Omega::Activation::Flags_t flags);
		};

		virtual const CreatorEntry* getCreatorEntries() const = 0;

	private:
		Omega::System::CriticalSection           m_csMain;
		Omega::System::AtomicOp<Omega::uint32_t> m_lockCount;

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

	ModuleBase* GetModule();

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

	/*protected:
		virtual Omega::IObject* GetControllingObject()
		{
			return this;
		}*/

	private:
		NoLockObjectImpl() : ROOT()
		{
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
			m_ptrOuter(pOuter)
		{ }

		ObjectPtr<Omega::IObject> m_ptrOuter;

		/*virtual Omega::IObject* GetControllingObject()
		{
			return m_ptrOuter;
		}*/

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
		ContainedObjectImpl<ROOT>                m_contained;
		Omega::System::AtomicOp<Omega::uint32_t> m_refcount;

	public:
		static AggregatedObjectImpl<ROOT>* CreateInstance(Omega::IObject* pOuter)
		{
			if (!pOuter)
				throw Omega::IException::Create(L"AggregatedObjectImpl must be aggregated",OMEGA_SOURCE_INFO);

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
			++m_refcount;
		}

		virtual void Release()
		{
			if (--m_refcount==0)
				delete this;
		}

		Omega::IObject* QueryInterface(const Omega::guid_t& iid)
		{
			if (iid==OMEGA_UUIDOF(Omega::IObject))
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
				Omega::System::Guard guard(GetModule()->GetLock());
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
		static SingletonObjectImpl<ROOT>* CreateInstance(Omega::IObject* = 0)
		{
			SingletonObjectImpl<ROOT>* pObject = singleton::instance();
			pObject->AddRef();
			return pObject;
		}

		static ObjectPtr<SingletonObjectImpl<ROOT> > CreateInstancePtr(Omega::IObject* pOuter = 0)
		{
			ObjectPtr<SingletonObjectImpl<ROOT> > ptr;
			ptr.Attach(CreateInstance(pOuter));
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

	private:
		SingletonObjectImpl(const SingletonObjectImpl&) {}
		SingletonObjectImpl& operator = (const SingletonObjectImpl&) { return *this; }
	};

	template <class ROOT>
	class StackObjectImpl : public ROOT
	{
	public:
		StackObjectImpl() : ROOT()
		{ }

	/*protected:
		virtual Omega::IObject* GetControllingObject()
		{
			return this;
		}*/

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
				Omega::IObject* pObject = OTL::ObjectImpl<T>::CreateInstancePtr()->QueryInterface(iid);
				if (!pObject)
					throw Omega::INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
				return pObject;
			}
		};

		inline Omega::Activation::IObjectFactory* GetObjectFactory(const Omega::guid_t& oid, Omega::Activation::Flags_t flags);

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
				Omega::IObject* pObject = OTL::NoLockObjectImpl<T>::CreateInstancePtr()->QueryInterface(iid);
				if (!pObject)
					throw Omega::INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
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
		static Omega::IObject* CreateInstance(Omega::IObject* pOuter, const Omega::guid_t& iid)
		{
			Omega::IObject* pObject = T::CreateInstancePtr(pOuter)->QueryInterface(iid);
			if (!pObject)
				throw Omega::INoInterfaceException::Create(iid,OMEGA_SOURCE_INFO);
			return pObject;
		}
	};

	template <const Omega::guid_t* pOID>
	class ObjectFactoryCallCreate<bool,pOID>
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
				OMEGA_NEW(singleton,SingletonNoLock<TYPE>());
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

	template <class EnumIFace, class EnumType>
	class EnumSTL :
		public ObjectBase,
		public EnumIFace
	{
		typedef EnumSTL<EnumIFace,EnumType> MyType;

	public:
		template <class InputIterator>
		static EnumIFace* Create(InputIterator first, InputIterator last)
		{
			ObjectPtr<ObjectImpl<MyType> > ptrThis = ObjectImpl<MyType>::CreateInstancePtr();
			ptrThis->m_listItems.assign(first,last);
			ptrThis->m_pos = ptrThis->m_listItems.begin();
			return ptrThis.AddRefReturn();
		}

	private:
		BEGIN_INTERFACE_MAP(MyType)
			INTERFACE_ENTRY(EnumIFace)
		END_INTERFACE_MAP()

	private:
		std::list<EnumType>                    m_listItems;
		typename std::list<EnumType>::iterator m_pos;
		Omega::System::CriticalSection         m_cs;

	// IEnumString members
	public:
		bool Next(Omega::uint32_t& count, EnumType* parrVals)
		{
			Omega::System::Guard guard(m_cs);

			uint32_t c = count;
			count = 0;
			while (m_pos!=m_listItems.end() && count < c)
			{
				parrVals[count] = *m_pos;
				++count;
				++m_pos;
			}

			return (m_pos!=m_listItems.end());
		}

		bool Skip(Omega::uint32_t count)
		{
			Omega::System::Guard guard(m_cs);

			while (count > 0 && m_pos!=m_listItems.end())
			{
                ++m_pos;
				--count;
			}

			return (m_pos!=m_listItems.end());
		}

		void Reset()
		{
			Omega::System::Guard guard(m_cs);

			m_pos = m_listItems.begin();
		}

		EnumIFace* Clone()
		{
			Omega::System::Guard guard(m_cs);

			ObjectPtr<ObjectImpl<MyType> > ptrNew = ObjectImpl<MyType>::CreateInstancePtr();
			ptrNew->m_listItems.assign(m_listItems.begin(),m_listItems.end());
			ptrNew->m_pos = ptrNew->m_listItems.begin();
			return ptrNew.AddRefReturn();
		}
	};

	typedef EnumSTL<Omega::IEnumString,Omega::string_t>	EnumString;
};

#include <OTL/OTL.inl>

// Specialisations
#include <OTL/OTL_special.h>

#endif // OTL_OTL_H_INCLUDED_

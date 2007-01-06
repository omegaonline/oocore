#ifndef OOCORE_RTTI_H_INCLUDED_
#define OOCORE_RTTI_H_INCLUDED_

namespace Omega
{	
	namespace MetaInfo
	{
		template <class I>
		struct interface_info
		{
			typedef I safe_class;
			typedef I safe_stub;
			typedef I safe_proxy;
		};

		interface IException_Safe;
		interface IObject_Safe
		{
			virtual IException_Safe* OMEGA_CALL AddRef_Safe() = 0;
			virtual IException_Safe* OMEGA_CALL Release_Safe() = 0;
			virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(IObject_Safe** retval, const guid_t& iid) = 0;
		};

		IObject_Safe* lookup_stub(IObject* pObj, const guid_t& iid);
		IObject* lookup_proxy(IObject_Safe* pObjS, const guid_t& iid, bool bPartialAllowed);
		void throw_correct_exception(IException_Safe* pE);
		IException_Safe* return_correct_exception(IException* pE);

		template <class I>
		struct safe_stub_functor;
		template <class I>
		struct safe_stub_functor_out;
		template <class I>
		struct safe_proxy_functor;
		template <class I>
		struct safe_proxy_functor_out;

		template <class I, class Base> interface IObject_SafeStub;
		template <class I> interface IObject_SafeProxy;
		template <> struct interface_info<IObject>
		{
			typedef IObject_Safe safe_class;
			template <class I> struct safe_stub_factory
			{
				typedef IObject_SafeStub<I,typename interface_info<I>::safe_class> type;
			};
			template <class I> struct safe_proxy_factory
			{
				typedef IObject_SafeProxy<I> type;
			};
		};
		template <> struct interface_info<IObject*>
		{
			typedef interface_info<IObject>::safe_class* safe_class;
			typedef safe_stub_functor<IObject> safe_stub;
			typedef safe_proxy_functor<IObject> safe_proxy;
		};
		template <> struct interface_info<IObject**>
		{
			typedef interface_info<IObject>::safe_class** safe_class;
			typedef safe_stub_functor_out<IObject> safe_stub;
			typedef safe_proxy_functor_out<IObject> safe_proxy;
		}; 

		template <class I, class Base> struct IException_SafeStub;
		template <class I, class Base> struct IException_SafeProxy;
		template <> struct interface_info<Omega::IException>
		{
			typedef IException_Safe safe_class;
			template <class I> struct safe_stub_factory
			{
				typedef IException_SafeStub<I,typename interface_info<Omega::IObject>::safe_stub_factory<I>::type> type;
			};
			template <class I> struct safe_proxy_factory
			{
				typedef IException_SafeProxy<I,typename interface_info<Omega::IObject>::safe_proxy_factory<I>::type> type;
			};
		};
		template <> struct interface_info<Omega::IException*>
		{
			typedef interface_info<Omega::IException>::safe_class* safe_class;
			typedef safe_stub_functor<Omega::IException> safe_stub;
			typedef safe_proxy_functor<Omega::IException> safe_proxy;
		};
		template <> struct interface_info<Omega::IException**>
		{
			typedef interface_info<Omega::IException>::safe_class** safe_class;
			typedef safe_stub_functor_out<Omega::IException> safe_stub;
			typedef safe_proxy_functor_out<Omega::IException> safe_proxy;
		};

		interface IException_Safe : public IObject_Safe
		{
			OMEGA_DECLARE_SAFE_DECLARED_METHOD(Omega::guid_t,GetActualIID,0,());
			OMEGA_DECLARE_SAFE_DECLARED_METHOD(Omega::IException*,Cause,0,());
			OMEGA_DECLARE_SAFE_DECLARED_METHOD(Omega::string_t,Description,0,());
			OMEGA_DECLARE_SAFE_DECLARED_METHOD(Omega::string_t,Source,0,());
		};

		template <class I>
		struct safe_stub_functor
		{
			safe_stub_functor(I* pI);

			~safe_stub_functor()
			{
				if (m_pS)
					m_pS->Release_Safe();
			}

			operator typename interface_info<I*>::safe_class ()
			{
				return m_pS;
			}

		private:
			typename interface_info<I*>::safe_class m_pS;
		};

		template <class I>
		struct safe_stub_functor_out
		{
			safe_stub_functor_out(I** ppI, const guid_t& iid = iid_traits<I>::GetIID());
			~safe_stub_functor_out();

			operator typename interface_info<I**>::safe_class ()
			{
				return &m_pS;
			}

		private:
			typename interface_info<I*>::safe_class m_pS;
			I** m_ppI;
			const guid_t& m_iid;

			// Disallow
			safe_stub_functor_out& operator = (const safe_stub_functor_out&)
			{}
		};

		template <class I>
		struct safe_proxy_functor
		{
			safe_proxy_functor(typename interface_info<I*>::safe_class pS);

			~safe_proxy_functor()
			{
				if (m_pI)
					m_pI->Release();
			}

			operator I* ()
			{
				return m_pI;
			}

		private:
			I* m_pI;
		};

		template <class I>
		struct safe_proxy_functor_out
		{
			safe_proxy_functor_out(typename interface_info<I**>::safe_class ppS, const guid_t& iid = iid_traits<I>::GetIID());
			~safe_proxy_functor_out();

			operator I** ()
			{
				return &m_pI;
			}

		private:
			I* m_pI;
			typename interface_info<I**>::safe_class m_ppS;
			const guid_t& m_iid;

			// Disallow!
			safe_proxy_functor_out& operator = (const safe_proxy_functor_out&)
			{}
		};

		template <class I_SafeStub, class I>
		class SafeStubImpl : public IObject_Safe
		{
			struct Contained : public I_SafeStub
			{
				Contained(IObject_Safe* pOuter, I* pI) : 
					I_SafeStub(pI), m_pOuter(pOuter)
				{ }

				virtual IException_Safe* OMEGA_CALL AddRef_Safe() 
				{ 
					return m_pOuter->AddRef_Safe(); 
				}
				virtual IException_Safe* OMEGA_CALL Release_Safe() 
				{ 
					return m_pOuter->Release_Safe(); 
				}
				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(IObject_Safe** ppS, const guid_t& iid)
				{
					return m_pOuter->QueryInterface_Safe(ppS,iid);
				}

				IObject_Safe* m_pOuter;
			};
			Contained					m_contained;
			AtomicOp<uint32_t>::type	m_refcount;

			SafeStubImpl(IObject_Safe* pOuter, I* pI) : m_contained(pOuter,pI), m_refcount(1)
			{ }

			virtual ~SafeStubImpl()
			{}

		public:
			static IObject_Safe* Create(IObject_Safe* pOuter, IObject* pObj)
			{
				I* pI = static_cast<I*>(pObj->QueryInterface(iid_traits<I>::GetIID()));
				if (!pI)
					INoInterfaceException::Throw(iid_traits<I>::GetIID(),OMEGA_FUNCNAME);

				SafeStubImpl* pRet = 0;
				OMEGA_NEW(pRet,SafeStubImpl(pOuter,pI));
				return pRet;
			}

		// IObject_Safe members
		public:
			IException_Safe* OMEGA_CALL AddRef_Safe()
			{
				++m_refcount;
				return 0;
			}

			IException_Safe* OMEGA_CALL Release_Safe()
			{
				try
				{
					if (--m_refcount==0)
						delete this;
					return 0;
				}
				catch (IException* pE)
				{
					return return_correct_exception(pE);
				}
			}

			IException_Safe* OMEGA_CALL QueryInterface_Safe(IObject_Safe** ppS, const guid_t& iid)
			{
				if (iid==IID_IObject)
				{
					*ppS = this;
					return AddRef_Safe();
				}
				else
					return m_contained.Internal_QueryInterface_Safe(ppS,iid);
			}		
		};

		template <class I_SafeProxy, class I>
		class SafeProxyImpl : public IObject
		{
			struct Contained : public I_SafeProxy
			{
				Contained(IObject* pOuter, typename interface_info<I*>::safe_class pS) : 
					I_SafeProxy(pS), m_pOuter(pOuter)
				{ }

				virtual void AddRef() 
				{ 
					m_pOuter->AddRef(); 
				}
				virtual void Release() 
				{ 
					m_pOuter->Release(); 
				}
				virtual IObject* QueryInterface(const guid_t& iid)
				{
					return m_pOuter->QueryInterface(iid);
				}
				
				IObject* m_pOuter;
			};
			Contained					m_contained;
			AtomicOp<uint32_t>::type	m_refcount;

			virtual ~SafeProxyImpl()
			{}

		public:
			SafeProxyImpl(IObject* pOuter, typename interface_info<I*>::safe_class pS) : m_contained(pOuter,pS), m_refcount(1)
			{ }

			static IObject* Create(IObject* pOuter, IObject_Safe* pObjS)
			{
				IObject_Safe* pObjS2 = 0;
				IException_Safe* pSE = pObjS->QueryInterface_Safe(&pObjS2,iid_traits<I>::GetIID());
				if (pSE)
					throw_correct_exception(pSE);
				if (!pObjS2)
					INoInterfaceException::Throw(iid_traits<I>::GetIID(),OMEGA_FUNCNAME);

				SafeProxyImpl* pRet = 0;
				OMEGA_NEW(pRet,SafeProxyImpl(pOuter,static_cast<interface_info<I*>::safe_class>(pObjS2)));
				return pRet;
			}

		// IObject members
		public:
			void AddRef()
			{
				++m_refcount;
			}

			void Release()
			{
				if (--m_refcount==0)
					delete this;
			}

			IObject* QueryInterface(const guid_t& iid)
			{
				if (iid==IID_IObject)
				{
					++m_refcount;
					return this;
				}
				else
					return m_contained.Internal_QueryInterface(iid);
			}
		};

		template <class I>
		inline void SafeThrow(IException_Safe* pSE)
		{
			I* pI = static_cast<I*>(static_cast<IException*>(interface_info<IException*>::safe_proxy(pSE))->QueryInterface(iid_traits<I>::GetIID()));
			pSE->Release_Safe();
			if (!pI)
				OMEGA_THROW("No handler for exception interface");
			throw pI;
		}
		
		template <class I, class Base> 
		struct IObject_SafeStub : public Base
		{
			IObject_SafeStub(I* pI) : m_pI(pI)
			{ } 

			virtual ~IObject_SafeStub()
			{
				if (m_pI)
					m_pI->Release();
			}
			
			virtual IException_Safe* Internal_QueryInterface_Safe(IObject_Safe** ppS, const guid_t&)
			{
				*ppS = 0;
				return 0;
			}

			I* m_pI;
		};

		template <class I>
		struct IObject_SafeProxy : public I
		{
			IObject_SafeProxy(typename interface_info<I*>::safe_class pS) :  m_pS(pS)
			{ } 

			virtual ~IObject_SafeProxy()
			{
				if (m_pS)
				{
					IException_Safe* pSE = m_pS->Release_Safe();
					if (pSE)
						throw_correct_exception(pSE);
				}
			}
			
			virtual IObject* Internal_QueryInterface(const guid_t&)
			{
				return 0;
			}

			typename interface_info<I*>::safe_class m_pS;
		};

		template <class I, class Base>
		struct IException_SafeStub : public Base
		{
			IException_SafeStub(I* pI) : Base(pI)
			{ }
			
			virtual IException_Safe* Internal_QueryInterface_Safe(IObject_Safe** ppS, const guid_t& iid)
			{
				if (iid == IID_IException)
				{
					*ppS = this;
					return this->AddRef_Safe();
				}

				return Base::Internal_QueryInterface_Safe(ppS,iid);
			}

			OMEGA_DECLARE_STUB_DECLARED_METHOD(guid_t,GetActualIID,0,());
			OMEGA_DECLARE_STUB_DECLARED_METHOD(IException*,Cause,0,());
			OMEGA_DECLARE_STUB_DECLARED_METHOD(string_t,Description,0,());
			OMEGA_DECLARE_STUB_DECLARED_METHOD(string_t,Source,0,());
		};

		template <class I, class Base>
		struct IException_SafeProxy : public Base
		{
			IException_SafeProxy(typename interface_info<I*>::safe_class pS) : Base(pS)
			{ } 

			virtual IObject* Internal_QueryInterface(const guid_t& iid)
			{
				if (iid == IID_IException)
				{
					this->AddRef();
					return this;
				}

				return Base::Internal_QueryInterface(iid);
			}

			OMEGA_DECLARE_PROXY_DECLARED_METHOD(guid_t,GetActualIID,0,())
			OMEGA_DECLARE_PROXY_DECLARED_METHOD(IException*,Cause,0,())
			OMEGA_DECLARE_PROXY_DECLARED_METHOD(string_t,Description,0,())
			OMEGA_DECLARE_PROXY_DECLARED_METHOD(string_t,Source,0,())	
		};

		interface IWireStub;
		template <class I>
		static IWireStub* CreateWireStub(IObject* /*pObject*/)
		{
			OMEGA_THROW("No remote handler for interface");
			return 0;
		}
				
		struct qi_rtti
		{
			IObject_Safe* (*pfnCreateSafeStub)(IObject_Safe* pOuter, IObject* pObj);
			IObject* (*pfnCreateSafeProxy)(IObject* pOuter, IObject_Safe* pObjS);
			void (*pfnSafeThrow)(IException_Safe* pSE);
			IWireStub* (*pfnCreateWireStub)(IObject* pObject);
		};

		no_t get_qi_rtti(const qi_rtti** ppRtti, ...);

		OMEGA_QI_MAGIC(Omega,IObject)
		OMEGA_QI_MAGIC(Omega,IException)

		template <bool more = false>
		struct get_qi_rtti_info_impl
		{
			template <class I>
			static void execute(const qi_rtti** ppRtti, I*, const guid_t&)
			{
				*ppRtti = 0;
			}
		};

		template <>
		struct get_qi_rtti_info_impl<true>
		{
			template <class I>
			static void execute(const qi_rtti** ppRtti, I* i, const guid_t& iid)
			{
				if (!get_qi_rtti(ppRtti,i,iid))
					get_qi_rtti_info_impl<sizeof(get_qi_rtti(ppRtti,(typename I::next::type*)0,iid)) == sizeof(yes_t)>::execute(ppRtti,(typename I::next::type*)0,iid);
			}
		};

		const qi_rtti* get_qi_rtti_info(const guid_t& iid);
		
		struct SafeProxyStubMap
		{
			CriticalSection m_cs;
			std::map<void*,void*> m_map;
		};

		SafeProxyStubMap& get_proxy_map();
		SafeProxyStubMap& get_stub_map();

		struct SafeStub : public IObject_Safe
		{
			SafeStub(IObject* pObj) : m_refcount(1), m_pObj(pObj)
			{
				m_pObj->AddRef();
			}

			virtual ~SafeStub()
			{
				for (std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					if (i->second)
						i->second->Release_Safe();
				}
				m_pObj->Release();

				// Remove ourselves from the stub_map
				SafeProxyStubMap& stub_map = get_stub_map();
				Guard<CriticalSection> guard(stub_map.m_cs);

				stub_map.m_map.erase(m_pObj);
			}

			IException_Safe* OMEGA_CALL AddRef_Safe()
			{
				++m_refcount;
				return 0;
			}

			IException_Safe* OMEGA_CALL Release_Safe()
			{
				try
				{
					if (--m_refcount==0)
						delete this;
					
					return 0;
				}
				catch (IException* pE)
				{
					return return_correct_exception(pE);
				}
			}

			IException_Safe* OMEGA_CALL QueryInterface_Safe(IObject_Safe** retval, const guid_t& iid);

		private:
			AtomicOp<uint32_t>::type m_refcount;
			CriticalSection m_cs;
			std::map<const guid_t,IObject_Safe*> m_iid_map;
			IObject* m_pObj;
		};

		OMEGA_DECLARE_IID(SafeProxy);
		struct SafeProxy : public IObject
		{
			SafeProxy(IObject_Safe* pObjS) : m_refcount(1), m_pS(pObjS)
			{
				IException_Safe* pSE = m_pS->AddRef_Safe();
				if (pSE)
					throw_correct_exception(pSE);
			}

			virtual ~SafeProxy()
			{
				for (std::map<const guid_t,IObject*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					if (i->second)
						i->second->Release();
				}
				m_pS->Release_Safe();

				// Remove ourselves from the proxy_map
				SafeProxyStubMap& proxy_map = get_proxy_map();
				Guard<CriticalSection> guard(proxy_map.m_cs);

				proxy_map.m_map.erase(m_pS);
			}

			void AddRef()
			{
				++m_refcount;
			}

			void Release()
			{
				if (--m_refcount==0)
					delete this;
			}

			IObject* QueryInterface(const guid_t& iid);

			IObject_Safe* GetSafeStub()
			{
				return m_pS;
			}

		private:
			AtomicOp<uint32_t>::type m_refcount;
			CriticalSection m_cs;
			std::map<const guid_t,IObject*> m_iid_map;
			IObject_Safe* m_pS;
		};
	}
}

// This IID is used to detect a SafeProxy - it has no other purpose
OMEGA_DEFINE_IID(Omega::MetaInfo,SafeProxy,0xc55f671d, 0xac67, 0x4a8d, 0xb6, 0x10, 0x3b, 0x8, 0xbe, 0x15, 0xa5, 0xea);

#endif // OOCORE_RTTI_H_INCLUDED_

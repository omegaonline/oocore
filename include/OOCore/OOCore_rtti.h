#ifndef OOCORE_RTTI_H_INCLUDED_
#define OOCORE_RTTI_H_INCLUDED_

namespace Omega
{	
	namespace MetaInfo
	{
		template <class I>
		struct interface_info
		{
			typedef I safe;
			typedef I stub;
			typedef I proxy;
		};

		// MSVC gets really confused by Omega::uint32_t sometimes...
		/*template <>
		struct interface_info<Omega::uint32_t>
		{
			typedef Omega::uint32_t safe;
			typedef Omega::uint32_t stub;
			typedef Omega::uint32_t proxy;
		};*/
		
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
		struct stub_functor;
		template <class I>
		struct stub_functor_out;
		template <class I>
		struct proxy_functor;
		template <class I>
		struct proxy_functor_out;

		template <class I, class Base> interface IObject_Stub;
		template <class I> interface IObject_Proxy;
		template <>
		struct interface_info<IObject>
		{
			typedef IObject_Safe safe_class;
			template <class I> struct stub_class
			{
				typedef IObject_Stub<I,typename interface_info<I>::safe_class> type;
			};
			template <class I> struct proxy_class
			{
				typedef IObject_Proxy<I> type;
			};
		};
		template <> struct interface_info<IObject*>
		{
			typedef interface_info<IObject>::safe_class* safe;
			typedef stub_functor<IObject> stub;
			typedef proxy_functor<IObject> proxy;
		};
		template <> struct interface_info<IObject**>
		{
			typedef interface_info<IObject>::safe_class** safe;
			typedef stub_functor_out<IObject> stub;
			typedef proxy_functor_out<IObject> proxy;
		}; 

		template <class I, class Base> struct IException_Stub;
		template <class I, class Base> struct IException_Proxy;
		template <>
		struct interface_info<Omega::IException>
		{
			typedef IException_Safe safe_class;
			template <class I> struct stub_class
			{
				typedef IException_Stub<I,typename interface_info<Omega::IObject>::stub_class<I>::type> type;
			};
			template <class I> struct proxy_class
			{
				typedef IException_Proxy<I,typename interface_info<Omega::IObject>::proxy_class<I>::type> type;
			};
		};
		template <> struct interface_info<Omega::IException*>
		{
			typedef interface_info<Omega::IException>::safe_class* safe;
			typedef stub_functor<Omega::IException> stub;
			typedef proxy_functor<Omega::IException> proxy;
		};
		template <> struct interface_info<Omega::IException**>
		{
			typedef interface_info<Omega::IException>::safe_class** safe;
			typedef stub_functor_out<Omega::IException> stub;
			typedef proxy_functor_out<Omega::IException> proxy;
		};

		interface IException_Safe : public IObject_Safe
		{
			OMEGA_DECLARE_SAFE_DECLARED_METHOD(Omega::guid_t,GetIID,0,());
			OMEGA_DECLARE_SAFE_DECLARED_METHOD(Omega::IException*,Cause,0,());
			OMEGA_DECLARE_SAFE_DECLARED_METHOD(Omega::string_t,Description,0,());
			OMEGA_DECLARE_SAFE_DECLARED_METHOD(Omega::string_t,Source,0,());
		};

		template <class I>
		struct stub_functor
		{
			stub_functor(I* pI);

			~stub_functor()
			{
				if (m_pS)
					m_pS->Release_Safe();
			}

			operator typename interface_info<I*>::safe ()
			{
				return m_pS;
			}

		private:
			typename interface_info<I*>::safe m_pS;
		};

		template <class I>
		struct stub_functor_out
		{
			stub_functor_out(I** ppI, const guid_t& iid = iid_traits<I>::GetIID());
			~stub_functor_out();

			operator typename interface_info<I**>::safe ()
			{
				return &m_pS;
			}

		private:
			typename interface_info<I*>::safe m_pS;
			I** m_ppI;
			const guid_t& m_iid;

			// Disallow
			stub_functor_out& operator = (const stub_functor_out&)
			{}
		};

		template <class I>
		struct proxy_functor
		{
			proxy_functor(typename interface_info<I*>::safe pS);

			~proxy_functor()
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
		struct proxy_functor_out
		{
			proxy_functor_out(typename interface_info<I**>::safe ppS, const guid_t& iid = iid_traits<I>::GetIID());
			~proxy_functor_out();

			operator I** ()
			{
				return &m_pI;
			}

		private:
			I* m_pI;
			typename interface_info<I**>::safe m_ppS;
			const guid_t& m_iid;

			// Disallow!
			proxy_functor_out& operator = (const proxy_functor_out&)
			{}
		};

		template <class I_Stub, class I>
		class StubImpl : public IObject_Safe
		{
			struct Contained : public I_Stub
			{
				Contained(IObject_Safe* pOuter, I* pI) : 
					I_Stub(pI), m_pOuter(pOuter)
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

			StubImpl(IObject_Safe* pOuter, I* pI) : m_contained(pOuter,pI), m_refcount(1)
			{ }

			virtual ~StubImpl()
			{}

		public:
			static IObject_Safe* Create(IObject_Safe* pOuter, IObject* pObj)
			{
				I* pI = static_cast<I*>(pObj->QueryInterface(iid_traits<I>::GetIID()));
				if (!pI)
					INoInterfaceException::Throw(iid_traits<I>::GetIID(),OMEGA_FUNCNAME);

				StubImpl* pRet = 0;
				OMEGA_NEW(pRet,StubImpl(pOuter,pI));
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

		template <class I_Proxy, class I>
		class ProxyImpl : public IObject
		{
			struct Contained : public I_Proxy
			{
				Contained(IObject* pOuter, typename interface_info<I*>::safe pS) : 
					I_Proxy(pS), m_pOuter(pOuter)
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

			virtual ~ProxyImpl()
			{}

		public:
			ProxyImpl(IObject* pOuter, typename interface_info<I*>::safe pS) : m_contained(pOuter,pS), m_refcount(1)
			{ }

			static IObject* Create(IObject* pOuter, IObject_Safe* pObjS)
			{
				IObject_Safe* pObjS2 = 0;
				IException_Safe* pSE = pObjS->QueryInterface_Safe(&pObjS2,iid_traits<I>::GetIID());
				if (pSE)
					throw_correct_exception(pSE);
				if (!pObjS2)
					INoInterfaceException::Throw(iid_traits<I>::GetIID(),OMEGA_FUNCNAME);

				ProxyImpl* pRet = 0;
				OMEGA_NEW(pRet,ProxyImpl(pOuter,static_cast<interface_info<I*>::safe>(pObjS2)));
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
		struct ThrowImpl
		{
			static void Throw(IException_Safe* pSE)
			{
				I* pI = static_cast<I*>(static_cast<IException*>(interface_info<IException*>::proxy(pSE))->QueryInterface(iid_traits<I>::GetIID()));
				pSE->Release_Safe();
				throw pI;
			}
		};

		template <class I, class Base> 
		struct IObject_Stub : public Base
		{
			IObject_Stub(I* pI) : m_pI(pI)
			{ } 

			virtual ~IObject_Stub()
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
		struct IObject_Proxy : public I
		{
			IObject_Proxy(typename interface_info<I*>::safe pS) :  m_pS(pS)
			{ } 

			virtual ~IObject_Proxy()
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

			typename interface_info<I*>::safe m_pS;
		};

		template <class I, class Base>
		struct IException_Stub : public Base
		{
			IException_Stub(I* pI) : Base(pI)
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

			OMEGA_DECLARE_STUB_DECLARED_METHOD(guid_t,GetIID,0,());
			OMEGA_DECLARE_STUB_DECLARED_METHOD(IException*,Cause,0,());
			OMEGA_DECLARE_STUB_DECLARED_METHOD(string_t,Description,0,());
			OMEGA_DECLARE_STUB_DECLARED_METHOD(string_t,Source,0,());
		};

		template <class I, class Base>
		struct IException_Proxy : public Base
		{
			IException_Proxy(typename interface_info<I*>::safe pS) : Base(pS)
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

			OMEGA_DECLARE_PROXY_DECLARED_METHOD(guid_t,GetIID,0,())
			OMEGA_DECLARE_PROXY_DECLARED_METHOD(IException*,Cause,0,())
			OMEGA_DECLARE_PROXY_DECLARED_METHOD(string_t,Description,0,())
			OMEGA_DECLARE_PROXY_DECLARED_METHOD(string_t,Source,0,())	
		};

		struct qi_rtti
		{
			IObject_Safe* (*pfnCreateStub)(IObject_Safe* pOuter, IObject* pObj);
			IObject* (*pfnCreateProxy)(IObject* pOuter, IObject_Safe* pObjS);
			void (*pfnThrow)(IException_Safe* pSE);
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
		
		struct ProxyStubMap
		{
			CriticalSection m_cs;
			std::map<void*,void*> m_map;
		};

		ProxyStubMap& get_proxy_map();
		ProxyStubMap& get_stub_map();

		struct Stub : public IObject_Safe
		{
			Stub(IObject* pObj) : m_refcount(1), m_pObj(pObj)
			{
				m_pObj->AddRef();
			}

			virtual ~Stub()
			{
				for (std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					if (i->second)
						i->second->Release_Safe();
				}
				m_pObj->Release();

				// Remove ourselves from the stub_map
				ProxyStubMap& stub_map = get_stub_map();
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

		OMEGA_DECLARE_IID(Proxy);
		struct Proxy : public IObject
		{
			Proxy(IObject_Safe* pObjS) : m_refcount(1), m_pS(pObjS)
			{
				IException_Safe* pSE = m_pS->AddRef_Safe();
				if (pSE)
					throw_correct_exception(pSE);
			}

			virtual ~Proxy()
			{
				for (std::map<const guid_t,IObject*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
				{
					if (i->second)
						i->second->Release();
				}
				m_pS->Release_Safe();

				// Remove ourselves from the proxy_map
				ProxyStubMap& proxy_map = get_proxy_map();
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

			IObject_Safe* GetStub()
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

// This IID is used to detect a proxy - it has no other purpose
OMEGA_DEFINE_IID(Omega::MetaInfo,Proxy,0xc55f671d, 0xac67, 0x4a8d, 0xb6, 0x10, 0x3b, 0x8, 0xbe, 0x15, 0xa5, 0xea);

#endif // OOCORE_RTTI_H_INCLUDED_

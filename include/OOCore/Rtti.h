#ifndef OOCORE_RTTI_H_INCLUDED_
#define OOCORE_RTTI_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		namespace MetaInfo
		{
			template <class T> struct std_safe_functor
			{
				std_safe_functor()
				{}

				std_safe_functor(const T& val) : m_val(val)
				{}

				// BorlandC is broken, and misses the const specialisation!
				// So we must explicitly remove the const here, to allow it through
				#if defined(__BORLANDC__)
				operator remove_const<T>::type&()
				{
					return const_cast<remove_const<T>::type&>(m_val);
				}
				#else
				operator T&()
				{
					return m_val;
				}
				#endif

				void detach(T& result)
				{
					result = m_val;
				}

			private:
				T m_val;

				std_safe_functor(const std_safe_functor&) {}
				std_safe_functor& operator = (const std_safe_functor&) {}
			};

			template <> struct std_safe_functor<void*>
			{
				std_safe_functor(void* val) : m_val(val)
				{}

				operator void*&()
				{
					return m_val;
				}

				void detach(void*& result)
				{
					result = m_val;
				}

			private:
				void* m_val;

				std_safe_functor(const std_safe_functor&) {}
				std_safe_functor& operator = (const std_safe_functor&) { return *this; }
			};

			template <class T> struct std_stub_functor;
			template <class T> struct std_proxy_functor;
			template <class T> struct std_wire_type;
			template <class T> struct std_wire_type_array;

			template <class T> struct interface_info
			{
				typedef T safe_class;
				typedef std_safe_functor<T> stub_functor;
				typedef std_safe_functor<T> proxy_functor;
				typedef std_wire_type<T> wire_type;
			};
			template <class T> struct interface_info<T&>
			{
				typedef typename interface_info<T>::safe_class* safe_class;
				typedef std_stub_functor<T&> stub_functor;
				typedef std_proxy_functor<T&> proxy_functor;
				typedef std_wire_type<T&> wire_type;
			};
			template <class T> struct interface_info<const T>
			{
				typedef const typename interface_info<T>::safe_class safe_class;
				typedef std_stub_functor<const T> stub_functor;
				typedef std_proxy_functor<const T> proxy_functor;
				typedef std_wire_type<const T> wire_type;
			};
			template <class T> struct interface_info<volatile T>
			{
				typedef typename interface_info<T>::safe_class safe_class;
				typedef std_stub_functor<T> stub_functor;
				typedef std_proxy_functor<volatile T> proxy_functor;
				typedef std_wire_type<T> wire_type;
			};
			template <class T> struct interface_info<T*>
			{
				typedef typename interface_info<T>::safe_class* safe_class;
				typedef std_safe_functor<T*> stub_functor;
				typedef std_safe_functor<T*> proxy_functor;
				typedef std_wire_type_array<T> wire_type;
			};

			template <class T> struct std_stub_functor<const T>
			{
				std_stub_functor(const typename interface_info<T>::safe_class& val) : m_actual(val)
				{}

				operator const T& ()
				{
					return m_actual;
				}

				void attach(const T& val_ref, const typename interface_info<T>::safe_class& val)
				{
					m_actual.attach(const_cast<T&>(val_ref),val);
				}

				void detach(const typename interface_info<T>::safe_class&)
				{}

				void detach(const typename interface_info<T>::safe_class&, const guid_t&)
				{}

			private:
				typename interface_info<T>::stub_functor	m_actual;

				std_stub_functor(const std_stub_functor&) {}
				std_stub_functor& operator = (const std_stub_functor&) {}
			};

			template <class T> struct std_proxy_functor<const T>
			{
				std_proxy_functor(const T& val) : m_actual(val)
				{}

				operator const typename interface_info<T>::safe_class& ()
				{
					return m_actual;
				}

				void attach(const typename interface_info<T>::safe_class& val_ref, const T& val)
				{
					m_actual.attach(const_cast<typename interface_info<T>::safe_class&>(val_ref),val);
				}

				void detach(const T&)
				{}

				void detach(const T&, const guid_t&)
				{}

			private:
				typename interface_info<T>::proxy_functor	m_actual;

				std_proxy_functor(const std_proxy_functor&) {}
				std_proxy_functor& operator = (const std_proxy_functor&) {}
			};

			template <class T> struct std_proxy_functor<volatile T>
			{
				std_proxy_functor(volatile T& val) : m_actual(val)
				{}

				operator typename interface_info<T>::safe_class& ()
				{
					return m_actual;
				}

				/*void attach(typename interface_info<T>::safe_class& val_ref, const T& val)
				{
					m_actual.attach(val_ref,val);
				}*/

				void detach(T volatile & val)
				{
					m_actual.detach(val);
				}

				/*void detach(T volatile & val, const guid_t& iid)
				{
					m_actual.detach(val,iid);
				}*/

			private:
				typename interface_info<T>::proxy_functor	m_actual;

				std_proxy_functor(const std_proxy_functor&) {}
				std_proxy_functor& operator = (const std_proxy_functor&) {}
			};

			template <class T> struct std_stub_functor<T&>
			{
				std_stub_functor(typename interface_info<T>::safe_class* val) :
					m_result(val), m_actual(*val)
				{}

				std_stub_functor(typename interface_info<T>::safe_class* val, const guid_t* iid) :
					m_result(val), m_actual(*val,iid)
				{}

				~std_stub_functor()
				{
					m_actual.detach(*m_result);
				}

				operator T& ()
				{
					return m_actual;
				}

			private:
				typename interface_info<T>::safe_class* m_result;
				typename interface_info<T>::stub_functor m_actual;

				std_stub_functor(const std_stub_functor&) {}
				std_stub_functor& operator = (const std_stub_functor&) {}
			};

			template <class T> struct std_proxy_functor<T&>
			{
				std_proxy_functor(T& val) :
					m_result(val), m_actual(m_result)
				{}

				std_proxy_functor(T& val, const guid_t& iid) :
					m_result(val), m_actual(m_result,iid)
				{}

				~std_proxy_functor()
				{
					m_actual.detach(m_result);
				}

				// BorlandC is broken, and misses the const specialisation!
				// So we must explicitly remove the const here, to allow it through
				#if defined(__BORLANDC__)
				operator typename interface_info<typename remove_const<T>::type>::safe_class* ()
				{
					return const_cast<typename interface_info<typename remove_const<T>::type>::safe_class*>(&static_cast<typename interface_info<T>::safe_class&>(m_actual));
				}
				#else
				operator typename interface_info<T>::safe_class* ()
				{
					return &static_cast<typename interface_info<T>::safe_class&>(m_actual);
				}
				#endif

			private:
				T&	m_result;
				typename interface_info<T>::proxy_functor	m_actual;

				std_proxy_functor(const std_proxy_functor&) {}
				std_proxy_functor& operator = (const std_proxy_functor&) {}
			};

			template <class I> struct iface_stub_functor
			{
				iface_stub_functor(typename interface_info<I>::safe_class* pS = 0, const guid_t* piid = 0) :
					m_fixed(0), m_pI(m_fixed)
				{
					init(pS,piid ? *piid : OMEGA_UUIDOF(I));
				}

				~iface_stub_functor()
				{
					if (m_pI)
						m_pI->Release();
				}

				operator I*& ()
				{
					return m_pI;
				}

				void attach(I*& val_ref, typename interface_info<I>::safe_class* pS, const guid_t* piid = 0)
				{
					m_pI = val_ref;
					init(pS,piid ? *piid : OMEGA_UUIDOF(I));
				}

				inline void detach(typename interface_info<I>::safe_class*& result, const guid_t& iid = OMEGA_UUIDOF(I));

			private:
				I* m_fixed;
				I*& m_pI;

				inline void init(typename interface_info<I>::safe_class* pS, const guid_t& iid);

				iface_stub_functor(const iface_stub_functor&) {}
				iface_stub_functor& operator = (const iface_stub_functor&) {}
			};

			template <class I> struct iface_proxy_functor
			{
				iface_proxy_functor(I* pI = 0, const guid_t& iid = OMEGA_UUIDOF(I)) :
					m_fixed(0), m_pS(m_fixed)
				{
					init(pI,iid);
				}

				~iface_proxy_functor()
				{
					if (m_pS)
						m_pS->Release_Safe();
				}

				operator typename interface_info<I*>::safe_class& ()
				{
					return m_pS;
				}

				void attach(typename interface_info<I>::safe_class*& val_ref, I* pI, const guid_t& iid = OMEGA_UUIDOF(I))
				{
					m_pS = val_ref;
					init(pI,iid);
				}

				inline void detach(I* volatile & result);

			private:
				typename interface_info<I>::safe_class* m_fixed;
				typename interface_info<I>::safe_class*& m_pS;
				guid_t	m_iid;

				inline void init(I* pI, const guid_t& iid);

				iface_proxy_functor(const iface_proxy_functor&) {}
				iface_proxy_functor& operator = (const iface_proxy_functor&) {}
			};

			template <class I> struct iface_stub_functor_array
			{
				iface_stub_functor_array(typename interface_info<I>::safe_class* pVals, uint32_t cbSize = s_default) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(cbSize), m_alloc_size(cbSize),
					m_piids(0), m_iid(OMEGA_UUIDOF(I))
				{
					init(pVals);
				}

				iface_stub_functor_array(typename interface_info<I>::safe_class* pVals, const guid_t& iid, uint32_t cbSize = s_default) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(cbSize), m_alloc_size(cbSize),
					m_piids(0), m_iid(iid)
				{
					init(pVals);
				}

				iface_stub_functor_array(typename interface_info<I>::safe_class* pVals, const uint32_t* pcbSize) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(*pcbSize), m_alloc_size(*pcbSize),
					m_piids(0), m_iid(OMEGA_UUIDOF(I))
				{
					init(pVals);
				}

				iface_stub_functor_array(typename interface_info<I>::safe_class* pVals, const guid_t& iid, const uint32_t* pcbSize) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(*pcbSize), m_alloc_size(*pcbSize),
					m_piids(0), m_iid(iid)
				{
					init(pVals);
				}

				iface_stub_functor_array(typename interface_info<I>::safe_class* pVals, const guid_t* piids, uint32_t cbSize = s_default) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(cbSize), m_alloc_size(cbSize),
					m_piids(piids), m_iid(guid_t::Null())
				{
					init(pVals);
				}

				iface_stub_functor_array(typename interface_info<I>::safe_class* pVals, const guid_t* piids, const uint32_t* pcbSize) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(*pcbSize), m_alloc_size(*pcbSize),
					m_piids(piids), m_iid(guid_t::Null())
				{
					init(pVals);
				}

				inline ~iface_stub_functor_array();

				operator I* ()
				{
					return m_pVals;
				}

			private:
				static const uint32_t s_default = 1;
				typename interface_info<I>::stub_functor* m_pFunctors;
				I* m_pVals;
				typename interface_info<I>::safe_class* m_pResults;
				uint32_t m_cbSize;
				const uint32_t m_alloc_size;
				const guid_t* m_piids;
				const guid_t& m_iid;

				inline void init(typename interface_info<I>::safe_class* pVals);

				iface_stub_functor_array(const iface_stub_functor_array&) {}
				iface_stub_functor_array& operator = (const iface_stub_functor_array&) {}
			};

			template <class I> struct iface_proxy_functor_array
			{
				iface_proxy_functor_array(I* pVals, uint32_t cbSize = s_default) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(cbSize), m_alloc_size(cbSize),
					m_piids(0), m_iid(OMEGA_UUIDOF(I))
				{
					init(pVals);
				}

				iface_proxy_functor_array(I* pVals, const guid_t& iid, uint32_t cbSize = s_default) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(cbSize), m_alloc_size(cbSize),
					m_piids(0), m_iid(iid)
				{
					init(pVals);
				}

				iface_proxy_functor_array(I* pVals, const guid_t* piids, uint32_t cbSize = s_default) :
					m_pFunctors(0), m_pVals(0), m_pResults(pVals),
					m_cbSize(cbSize), m_alloc_size(cbSize),
					m_piids(piids), m_iid(OMEGA_UUIDOF(I))
				{
					init(pVals);
				}

				inline ~iface_proxy_functor_array();

				operator typename interface_info<I>::safe_class* ()
				{
					return m_pVals;
				}

			private:
				static const uint32_t s_default = 1;
				typename interface_info<I>::proxy_functor* m_pFunctors;
				typename interface_info<I>::safe_class* m_pVals;
				I* m_pResults;
				uint32_t m_cbSize;
				const uint32_t m_alloc_size;
				const guid_t* m_piids;
				const guid_t& m_iid;

				inline void init(I* pVals);

				iface_proxy_functor_array(const iface_proxy_functor_array&) {}
				iface_proxy_functor_array& operator = (const iface_proxy_functor_array&) {}
			};

			interface IException_Safe;
			interface IObject_Safe
			{
				virtual void OMEGA_CALL AddRef_Safe() = 0;
				virtual void OMEGA_CALL Release_Safe() = 0;
				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(IObject_Safe** retval, const guid_t& iid) = 0;

            protected:
                virtual ~IObject_Safe() {};
			};


			inline IObject_Safe* lookup_stub(IObject* pObj, const guid_t& iid);
			inline IObject* lookup_proxy(IObject_Safe* pObjS, const guid_t& iid, bool bPartialAllowed);
			inline void throw_correct_exception(IException_Safe* pE);
			inline IException_Safe* return_safe_exception(IException* pE);
			inline const Omega::string_t& lookup_iid(const guid_t& iid);

			template <class I, class Base> struct IObject_SafeStub;
			template <class I> struct IObject_SafeProxy;
			template <class I> struct IObject_WireStub;
			template <class I> struct IObject_WireProxy;

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
				template <class I> struct wire_stub_factory
				{
					typedef IObject_WireStub<I> type;
				};
				template <class I> struct wire_proxy_factory
				{
					typedef IObject_WireProxy<I> type;
				};
			};

			template <class I> struct iface_wire_type;

			template <> struct interface_info<IObject*>
			{
				typedef interface_info<IObject>::safe_class* safe_class;
				typedef iface_stub_functor<IObject> stub_functor;
				typedef iface_proxy_functor<IObject> proxy_functor;
				typedef iface_wire_type<IObject> wire_type;
			};
			template <> struct interface_info<IObject**>
			{
				typedef interface_info<IObject*>::safe_class* safe_class;
				typedef iface_stub_functor_array<IObject> stub_functor;
				typedef iface_proxy_functor_array<IObject> proxy_functor;
				typedef std_wire_type_array<IObject> wire_type;
			};

			template <class I, class Base> struct IException_SafeStub;
			template <class I, class Base> struct IException_SafeProxy;
			template <class I, class Base> struct IException_WireStub;
			template <class I, class Base> struct IException_WireProxy;

			template <> struct interface_info<IException>
			{
				typedef IException_Safe safe_class;
				template <class I> struct safe_stub_factory
				{
					typedef IException_SafeStub<I,typename interface_info<IObject>::safe_stub_factory<I>::type> type;
				};
				template <class I> struct safe_proxy_factory
				{
					typedef IException_SafeProxy<I,typename interface_info<IObject>::safe_proxy_factory<I>::type> type;
				};
				template <class I> struct wire_stub_factory
				{
					typedef IException_WireStub<I,typename interface_info<IObject>::wire_stub_factory<I>::type> type;
				};
				template <class I> struct wire_proxy_factory
				{
					typedef IException_WireProxy<I,typename interface_info<IObject>::wire_proxy_factory<I>::type> type;
				};
			};
			template <> struct interface_info<IException*>
			{
				typedef interface_info<IException>::safe_class* safe_class;
				typedef iface_stub_functor<IException> stub_functor;
				typedef iface_proxy_functor<IException> proxy_functor;
				typedef iface_wire_type<IException> wire_type;
			};
			template <> struct interface_info<IException**>
			{
				typedef interface_info<IException*>::safe_class* safe_class;
				typedef iface_stub_functor_array<IException> stub_functor;
				typedef iface_proxy_functor_array<IException> proxy_functor;
				typedef std_wire_type_array<IException> wire_type;
			};

			interface IException_Safe : public IObject_Safe
			{
				OMEGA_DECLARE_SAFE_DECLARED_METHOD(0,guid_t,ActualIID,0,());
				OMEGA_DECLARE_SAFE_DECLARED_METHOD(0,IException*,Cause,0,());
				OMEGA_DECLARE_SAFE_DECLARED_METHOD(0,string_t,Description,0,());
				OMEGA_DECLARE_SAFE_DECLARED_METHOD(0,string_t,Source,0,());
			};

			template <class I> struct auto_iface_ptr
			{
				auto_iface_ptr(I* pI = 0) : m_pI(pI)
				{}

				~auto_iface_ptr()
				{
					if (m_pI)
						m_pI->Release();
				}

				auto_iface_ptr& operator = (I* pI)
				{
					if (pI != m_pI)
					{
						if (m_pI)
							m_pI->Release();

						m_pI = pI;

						if (m_pI)
							m_pI->AddRef();
					}

					return *this;
				}

				operator I* ()
				{
					return m_pI;
				}

				I* operator ->()
				{
					return m_pI;
				}

				void detach()
				{
					m_pI = 0;
				}

			private:
				I* m_pI;

				auto_iface_ptr(const auto_iface_ptr& rhs) {};
				auto_iface_ptr& operator = (const auto_iface_ptr& rhs) {};
			};

			template <class S> struct auto_iface_safe_ptr
			{
				auto_iface_safe_ptr(S* pS = 0) : m_pS(pS)
				{}

				~auto_iface_safe_ptr()
				{
					if (m_pS)
						m_pS->Release_Safe();
				}

				auto_iface_safe_ptr& operator = (S* pS)
				{
					if (pS != m_pS)
					{
						if (m_pS)
							m_pS->Release_Safe();

						m_pS = pS;

						if (m_pS)
							m_pS->AddRef_Safe();
					}
					return *this;
				}

				operator S* ()
				{
					return m_pS;
				}

				S* operator ->()
				{
					return m_pS;
				}

				void detach()
				{
					m_pS = 0;
				}

			private:
				S* m_pS;

				auto_iface_safe_ptr(const auto_iface_safe_ptr& rhs) {};
				auto_iface_safe_ptr& operator = (const auto_iface_safe_ptr& rhs) {};
			};

			template <class I_SafeStub, class I>
			class SafeStubImpl : public IObject_Safe
			{
				struct Contained : public I_SafeStub
				{
					Contained(IObject_Safe* pOuter, I* pI) :
						I_SafeStub(pI), m_pOuter(pOuter)
					{ }

					virtual void OMEGA_CALL AddRef_Safe()
					{
						m_pOuter->AddRef_Safe();
					}
					virtual void OMEGA_CALL Release_Safe()
					{
						m_pOuter->Release_Safe();
					}
					virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(IObject_Safe** ppS, const guid_t& iid)
					{
						return m_pOuter->QueryInterface_Safe(ppS,iid);
					}

					IObject_Safe* m_pOuter;
				};
				Contained                  m_contained;
				System::AtomicOp<uint32_t> m_refcount;

				SafeStubImpl(IObject_Safe* pOuter, I* pI) : m_contained(pOuter,pI), m_refcount(1)
				{}

				virtual ~SafeStubImpl()
				{}

			public:
				static IObject_Safe* Create(IObject_Safe* pOuter, IObject* pObj)
				{
					auto_iface_ptr<I> ptrI(static_cast<I*>(pObj->QueryInterface(OMEGA_UUIDOF(I))));
					if (!ptrI)
						return 0;

					SafeStubImpl* pRet = 0;
					OMEGA_NEW(pRet,SafeStubImpl(pOuter,ptrI));
					ptrI.detach();
					return pRet;
				}

			// IObject_Safe members
			public:
				void OMEGA_CALL AddRef_Safe()
				{
					++m_refcount;
				}

				void OMEGA_CALL Release_Safe()
				{
					if (--m_refcount==0)
						delete this;
				}

				IException_Safe* OMEGA_CALL QueryInterface_Safe(IObject_Safe** ppS, const guid_t& iid)
				{
					if (iid==OMEGA_UUIDOF(IObject))
					{
						*ppS = this;
						AddRef_Safe();
						return 0;
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
					{}

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

				private:
					IObject* m_pOuter;

					Contained(const Contained&) {};
					Contained& operator = (const Contained&) {};
				};
				Contained                  m_contained;
				System::AtomicOp<uint32_t> m_refcount;

				SafeProxyImpl(const SafeProxyImpl&) {};
				SafeProxyImpl& operator = (const SafeProxyImpl&) {};

			public:
				SafeProxyImpl(IObject* pOuter, typename interface_info<I*>::safe_class pS) : m_contained(pOuter,pS), m_refcount(1)
				{}

				virtual ~SafeProxyImpl()
				{}

				static IObject* Create(IObject* pOuter, IObject_Safe* pObjS)
				{
					IObject_Safe* pObjS2 = 0;
					IException_Safe* pSE = pObjS->QueryInterface_Safe(&pObjS2,OMEGA_UUIDOF(I));
					if (pSE)
						throw_correct_exception(pSE);
					if (!pObjS2)
						return 0;

					SafeProxyImpl* pRet = 0;
					auto_iface_safe_ptr<IObject_Safe> ptrObjS2(pObjS2);
					OMEGA_NEW(pRet,SafeProxyImpl(pOuter,static_cast<typename interface_info<I*>::safe_class>(pObjS2)));
					ptrObjS2.detach();
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
					if (iid==OMEGA_UUIDOF(IObject))
					{
						++m_refcount;
						return this;
					}
					else
						return m_contained.Internal_QueryInterface(iid);
				}
			};

			template <class I>
			void SafeThrow(IException_Safe* pSE)
			{
				auto_iface_safe_ptr<IException_Safe> ptrSE(pSE);
				I* pI = static_cast<I*>(static_cast<IException*>(interface_info<IException*>::stub_functor(pSE))->QueryInterface(OMEGA_UUIDOF(I)));
				if (!pI)
					throw INoInterfaceException::Create(OMEGA_UUIDOF(I),OMEGA_SOURCE_INFO);
				throw pI;
			}

			template <class I>
			void DynamicThrow(IException* pE)
			{
				auto_iface_ptr<IException> ptrE(pE);
				I* pI = static_cast<I*>(pE->QueryInterface(OMEGA_UUIDOF(I)));
				if (!pI)
					throw INoInterfaceException::Create(OMEGA_UUIDOF(I),OMEGA_SOURCE_INFO);
				throw pI;
			}

			template <class I, class Base>
			struct IObject_SafeStub : public Base
			{
				IObject_SafeStub(I* pI) : m_pI(pI)
				{}

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
						m_pS->Release_Safe();
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
					if (iid == OMEGA_UUIDOF(IException))
					{
						*ppS = this;
						this->AddRef_Safe();
						return 0;
					}

					return Base::Internal_QueryInterface_Safe(ppS,iid);
				}

				OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD(0,guid_t,ActualIID,0,());
				OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD(0,IException*,Cause,0,());
				OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD(0,string_t,Description,0,());
				OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD(0,string_t,Source,0,());
			};

			template <class I, class Base>
			struct IException_SafeProxy : public Base
			{
				IException_SafeProxy(typename interface_info<I*>::safe_class pS) : Base(pS)
				{ }

				virtual IObject* Internal_QueryInterface(const guid_t& iid)
				{
					if (iid == OMEGA_UUIDOF(IException))
					{
						this->AddRef();
						return this;
					}

					return Base::Internal_QueryInterface(iid);
				}

				OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD(0,guid_t,ActualIID,0,())
				OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD(0,IException*,Cause,0,())
				OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD(0,string_t,Description,0,())
				OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD(0,string_t,Source,0,())
			};

			interface IWireStub;
			interface IWireManager;

			template <class T> static IWireStub* CreateWireStub(IWireManager*, IObject*, uint32_t);
			template <class T, class B> class WireProxyImpl;

			struct qi_rtti
			{
				IObject_Safe* (*pfnCreateSafeStub)(IObject_Safe* pOuter, IObject* pObj);
				IObject* (*pfnCreateSafeProxy)(IObject* pOuter, IObject_Safe* pObjS);
				void (*pfnSafeThrow)(IException_Safe* pSE);
				void (*pfnThrow)(IException* pE);
				IWireStub* (*pfnCreateWireStub)(IWireManager* pManager, IObject* pObject, uint32_t id);
				IObject* (*pfnCreateWireProxy)(IObject* pOuter, IWireManager* pManager);
				string_t strName;
			};

			template <bool C, typename Ta, typename Tb>
			struct if_then_else_;

			template <typename Ta, typename Tb>
			struct if_then_else_<true, Ta, Tb>
			{
				typedef Ta type;
			};

			template <typename Ta, typename Tb>
			struct if_then_else_<false, Ta, Tb>
			{
				typedef Tb type;
			};

			no_t get_qi_rtti(const qi_rtti** ppRtti, ...);

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

			inline const qi_rtti* get_qi_rtti_info(const guid_t& iid);

			struct SafeProxyStubMap
			{
				System::ReaderWriterLock m_lock;
				std::map<void*,void*>    m_map;
			};

			inline SafeProxyStubMap& get_proxy_map();
			inline SafeProxyStubMap& get_stub_map();

			struct SafeStub : public IObject_Safe
			{
				SafeStub(IObject* pObj) : m_refcount(0), m_pObj(pObj)
				{
					m_pObj->AddRef();
				}

				virtual ~SafeStub()
				{
					try
					{
						for (std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
						{
							if (i->second)
								i->second->Release_Safe();
						}
					}
					catch (...)
					{}

					// Remove ourselves from the stub_map
					SafeProxyStubMap& stub_map = get_stub_map();

					System::WriteGuard guard(stub_map.m_lock);
					stub_map.m_map.erase(m_pObj);
				}

				void OMEGA_CALL AddRef_Safe()
				{
					++m_refcount;
				}

				void OMEGA_CALL Release_Safe()
				{
					if (--m_refcount==0)
						delete this;
				}

				inline IException_Safe* OMEGA_CALL QueryInterface_Safe(IObject_Safe** retval, const guid_t& iid);

			private:
				System::AtomicOp<uint32_t>           m_refcount;
				System::ReaderWriterLock             m_lock;
				std::map<const guid_t,IObject_Safe*> m_iid_map;
				auto_iface_ptr<IObject>              m_pObj;
			};

			struct SafeProxy : public IObject
			{
				SafeProxy(IObject_Safe* pObjS) : m_refcount(0), m_pS(pObjS)
				{
					m_pS->AddRef_Safe();
				}

				virtual ~SafeProxy()
				{
					try
					{
						for (std::map<const guid_t,IObject*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
						{
							if (i->second)
								i->second->Release();
						}
					}
					catch (...)
					{}

					// Remove ourselves from the proxy_map
					SafeProxyStubMap& proxy_map = get_proxy_map();

					System::WriteGuard guard(proxy_map.m_lock);
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

				inline IObject* QueryInterface(const guid_t& iid);

				IObject_Safe* GetSafeStub()
				{
					return m_pS;
				}

			private:
				System::AtomicOp<uint32_t>        m_refcount;
				System::ReaderWriterLock          m_lock;
				std::map<const guid_t,IObject*>   m_iid_map;
				auto_iface_safe_ptr<IObject_Safe> m_pS;
			};
		}
	}
}

// This IID is used to detect a SafeProxy - it has no other purpose
OMEGA_DEFINE_IID(Omega::System::MetaInfo,SafeProxy,"{ADFB60D2-3125-4046-9EEB-0CC898E989E8}");

#endif // OOCORE_RTTI_H_INCLUDED_

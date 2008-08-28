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

#ifndef OOCORE_RTTI_H_INCLUDED_
#define OOCORE_RTTI_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		inline bool PinObjectPointer(IObject* pObject);
		inline void UnpinObjectPointer(IObject* pObject);

		inline string_t IIDToName(const guid_t& iid);

		namespace MetaInfo
		{
			template <class T>
			class std_safe_type
			{
			public:
				class micro_ref
				{
				public:
					micro_ref(T& val) : m_val(val)
					{}

					micro_ref(const micro_ref& rhs) : m_val(rhs.m_val)
					{}

					micro_ref& operator = (const micro_ref& rhs)
					{
						m_val = rhs.m_val;
						return *this;
					}

					void update(T& dest)
					{
						dest = m_val;
					}

					operator T& ()
					{
						return m_val;
					}

					T* operator & ()
					{
						return &m_val;
					}

					T& m_val;
				};

				typedef T type;
				typedef micro_ref ref_type;
				typedef micro_ref ref_safe_type;

				static T& coerce(T& val)
				{
					return val;
				}
			};

			template <class T> class std_wire_type;
			template <class T> class std_wire_type_array;

			template <class T> struct marshal_info
			{
				typedef std_safe_type<T> safe_type;
				typedef std_wire_type<T> wire_type;
			};

			#if defined(_MSC_VER) && defined(_Wp64)
			// VC gets badly confused with size_t
			template <> struct marshal_info<size_t>
			{
			#if defined(OMEGA_64)
				typedef std_safe_type<uint64_t> safe_type;
				typedef std_wire_type<uint64_t> wire_type;
			#else
				typedef std_safe_type<uint32_t> safe_type;
				typedef std_wire_type<uint32_t> wire_type;
			#endif
			};
			#endif

			template <class T> struct marshal_info<T*>
			{
				typedef std_safe_type<T*> safe_type;
				typedef std_wire_type_array<T> wire_type;
			};

			interface IObject_Safe;

			template <class T>
			class std_safe_type<T&>
			{
			public:
				class ref_holder
				{
				public:
					ref_holder(typename marshal_info<T>::safe_type::type* val) :
						m_val(*val), m_dest(val)
					{}

					ref_holder(typename marshal_info<T>::safe_type::type* val, const guid_t* piid) :
						m_val(*val,piid), m_dest(val)
					{}

					~ref_holder()
					{
						m_val.update(*m_dest);
					}

					operator T&()
					{
						return m_val;
					}

				private:
					typename marshal_info<T>::safe_type::ref_type m_val;
					typename marshal_info<T>::safe_type::type* m_dest;
				};

				class ref_holder_safe
				{
				public:
					ref_holder_safe(T& val) : m_val(val), m_dest(&val)
					{}

					ref_holder_safe(T& val, const guid_t& iid) : m_val(val,iid), m_dest(&val)
					{}

					~ref_holder_safe()
					{
						m_val.update(*m_dest);
					}

					operator typename marshal_info<T>::safe_type::type*()
					{
						return &m_val;
					}

				private:
					typename marshal_info<T>::safe_type::ref_safe_type m_val;
					T* m_dest;
				};
				typedef typename marshal_info<T>::safe_type::type* type;

				static ref_holder_safe coerce(T& val)
				{
					return ref_holder_safe(val);
				}

				static ref_holder_safe coerce(T& val, const guid_t& iid)
				{
					return ref_holder_safe(val,iid);
				}

				static ref_holder coerce(type val)
				{
					return ref_holder(val);
				}

				static ref_holder coerce(type val, const guid_t* piid)
				{
					return ref_holder(val,piid);
				}
			};

			template <class T>
			class std_safe_type<const T&>
			{
			public:
				typedef const typename marshal_info<T>::safe_type::type* type;

				static type coerce(const T& val)
				{
					return &marshal_info<T>::safe_type::coerce(const_cast<T&>(val));
				}

				static type coerce(const T& val, const guid_t& iid)
				{
					return &marshal_info<T>::safe_type::coerce(const_cast<T&>(val),iid);
				}

				static const T& coerce(type val)
				{
					return marshal_info<T>::safe_type::coerce(*const_cast<typename marshal_info<T>::safe_type::type*>(val));
				}

				static const T& coerce(type val, const guid_t* piid)
				{
					return marshal_info<T>::safe_type::coerce(*const_cast<typename marshal_info<T>::safe_type::type*>(val),piid);
				}
			};

			template <class Base> interface IException_Impl_Safe;
			typedef IException_Impl_Safe<IObject_Safe> IException_Safe;

			interface IObject_Safe
			{
				virtual void OMEGA_CALL AddRef_Safe() = 0;
				virtual void OMEGA_CALL Release_Safe() = 0;
				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, IObject_Safe** ppS) = 0;

				virtual void OMEGA_CALL Pin() = 0;
				virtual void OMEGA_CALL Unpin() = 0;
			};

			template <class I, class Base> class IObject_SafeStub;
			template <class I> class IObject_SafeProxy;
			template <class I> class IObject_Stub;
			template <class I> class IObject_Proxy;

			template <class I> struct interface_info;

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
					typedef IObject_Stub<I> type;
				};
				template <class I> struct wire_proxy_factory
				{
					typedef IObject_Proxy<I> type;
				};
			};

			template <class I>
			inline I* lookup_proxy(typename interface_info<I>::safe_class* pObjS, const guid_t& iid, bool bPartialAllowed);

			template <class I>
			inline typename interface_info<I>::safe_class* lookup_stub(I* pObj, const guid_t& iid);

			template <class I>
			class iface_stub_functor
			{
			public:
				iface_stub_functor(typename interface_info<I>::safe_class* pS, const guid_t& iid) :
					m_pI(0)
				{
					m_pI = lookup_proxy<I>(pS,iid,false);
				}

				iface_stub_functor(const iface_stub_functor& rhs) :
					m_pI(rhs.m_pI)
				{
					if (m_pI)
						m_pI->AddRef();
				}

				~iface_stub_functor()
				{
					// If this blows, then you have released an (in) parameter!
					if (m_pI)
						m_pI->Release();
				}

				operator I* ()
				{
					return m_pI;
				}

				I* operator -> ()
				{
					return m_pI;
				}

			protected:
				I* m_pI;

				iface_stub_functor& operator = (const iface_stub_functor&) {};
			};

			template <class I>
			class iface_stub_functor_ref : public iface_stub_functor<I>
			{
			public:
				iface_stub_functor_ref(typename interface_info<I>::safe_class* pS, const guid_t* piid = 0) :
				  iface_stub_functor<I>(pS,piid ? *piid : OMEGA_GUIDOF(I)), m_piid(piid ? piid : &OMEGA_GUIDOF(I))
				{
				}

				void update(typename interface_info<I>::safe_class*& pS)
				{
					if (pS)
						pS->Release_Safe();

					pS = lookup_stub<I>(this->m_pI,*m_piid);
				}

				operator I*& ()
				{
					return this->m_pI;
				}

			private:
				const guid_t* m_piid;
			};

			template <class I>
			class iface_proxy_functor
			{
			public:
				iface_proxy_functor(I* pI, const guid_t& iid) :
					m_pS(0)
				{
					m_pS = lookup_stub<I>(pI,iid);
				}

				iface_proxy_functor(const iface_proxy_functor& rhs) :
					m_pS(rhs.m_pS)
				{
					if (m_pS)
						m_pS->AddRef_Safe();
				}

				~iface_proxy_functor()
				{
					if (m_pS)
						m_pS->Release_Safe();
				}

				operator typename interface_info<I>::safe_class* ()
				{
					return m_pS;
				}

				typename interface_info<I>::safe_class* operator -> ()
				{
					return m_pS;
				}

			protected:
				typename interface_info<I>::safe_class* m_pS;

				iface_proxy_functor& operator = (const iface_proxy_functor&) {}
			};

			template <class I>
			class iface_proxy_functor_ref : public iface_proxy_functor<I>
			{
			public:
				iface_proxy_functor_ref(I* pI, const guid_t& iid = OMEGA_GUIDOF(I)) :
					iface_proxy_functor<I>(pI,iid), m_piid(&iid)
				{
				}

				void update(I*& pI)
				{
					if (pI)
						pI->Release();

					pI = lookup_proxy<I>(this->m_pS,*m_piid,true);
				}

				typename interface_info<I>::safe_class** operator & ()
				{
					return &this->m_pS;
				}

			private:
				const guid_t* m_piid;
			};

			template <class I>
			class iface_safe_type
			{
			public:
				typedef typename interface_info<I>::safe_class* type;

				typedef iface_proxy_functor_ref<I> ref_safe_type;
				typedef iface_stub_functor_ref<I> ref_type;

				static iface_proxy_functor<I> coerce(I* val, const guid_t& iid = OMEGA_GUIDOF(I))
				{
					return iface_proxy_functor<I>(val,iid);
				}

				static iface_stub_functor<I> coerce(type val, const guid_t* piid = 0)
				{
					return iface_stub_functor<I>(val,piid ? *piid : OMEGA_GUIDOF(I));
				}
			};

			template <class I> class iface_wire_type;

			template <> struct marshal_info<IObject*>
			{
				typedef iface_safe_type<IObject> safe_type;
				typedef iface_wire_type<IObject> wire_type;
			};

			inline void throw_correct_exception(IException_Safe* pE);
			inline IException_Safe* return_safe_exception(IException* pE);
			
			OMEGA_DECLARE_FORWARDS(IException,Omega,IException,Omega,IObject)

			template <class I> class auto_iface_ptr
			{
			public:
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

				void attach(I* pI)
				{
					if (m_pI)
						m_pI->Release();

					m_pI = pI;
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

			template <class S> class auto_iface_safe_ptr
			{
			public:
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

				void attach(S* pS)
				{
					if (m_pS)
						m_pS->Release_Safe();

					m_pS = pS;
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

			template <class I>
			void SafeThrow(IException_Safe* pSE)
			{
				auto_iface_safe_ptr<IException_Safe> ptrSE(pSE);
				I* pI = static_cast<I*>(marshal_info<IException*>::safe_type::coerce(pSE)->QueryInterface(OMEGA_GUIDOF(I)));
				if (!pI)
					throw INoInterfaceException::Create(OMEGA_GUIDOF(I),OMEGA_SOURCE_INFO);
				throw pI;
			}

			class SafeStub;

			template <class I_SafeStub, class I>
			class SafeStubImpl : public IObject_Safe
			{
			public:
				virtual void OMEGA_CALL AddRef_Safe()
				{
					++m_refcount;
				}

				virtual void OMEGA_CALL Release_Safe()
				{
					if (--m_refcount==0)
					{
						m_contained.Release_Iface();

						if (m_pincount == 0)
							delete this;
					}
				}

				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, IObject_Safe** ppS)
				{
					if (*piid == OMEGA_GUIDOF(IObject))
					{
						*ppS = this;
						(*ppS)->AddRef_Safe();
						return 0;
					}
					return m_contained.Internal_QueryInterface_Safe(false,piid,ppS);
				}

				virtual void OMEGA_CALL Pin()
				{
					++m_pincount;
				}

				virtual void OMEGA_CALL Unpin()
				{
					if (--m_pincount==0 && m_refcount==0)
						delete this;
				}

				static IObject_Safe* Create(SafeStub* pStub, IObject* pObj)
				{
					IObject_Safe* pRet = 0;
					OMEGA_NEW(pRet,SafeStubImpl(pStub,static_cast<I*>(pObj)));
					return pRet;
				}

			private:
				Threading::AtomicOp<uint32_t> m_refcount;
				Threading::AtomicOp<uint32_t> m_pincount;
				I_SafeStub                    m_contained;

				SafeStubImpl(SafeStub* pStub, I* pI) :
					m_refcount(0), m_pincount(0), m_contained(pStub,pI)
				{ }

				virtual ~SafeStubImpl()
				{ }
			};

			template <class I, class Base>
			class IObject_SafeStub : public Base
			{
			public:
				IObject_SafeStub(SafeStub* pStub, I* pI) :
					m_pI(pI), m_pStub(pStub)
				{
					this->m_pI->AddRef();
				}

				virtual void OMEGA_CALL AddRef_Safe()
				{
					this->m_pStub->AddRef_Safe();
				}

				virtual void OMEGA_CALL Release_Safe()
				{
					this->m_pStub->Release_Safe();
				}

				virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, IObject_Safe** ppS)
				{
					return Internal_QueryInterface_Safe(true,piid,ppS);
				}

				virtual void OMEGA_CALL Pin()
				{
					this->m_pStub->Pin();
				}

				virtual void OMEGA_CALL Unpin()
				{
					this->m_pStub->Unpin();
				}

				virtual IException_Safe* Internal_QueryInterface_Safe(bool bRecurse, const guid_t* piid, IObject_Safe** ppS)
				{
					if (!bRecurse)
					{
						*ppS = 0;
						return 0;
					}
					return this->m_pStub->QueryInterface_Safe(piid,ppS);
				}

				virtual void Release_Iface()
				{
					this->m_pI->Release();
				}

			protected:
				I* m_pI;

			private:
				SafeStub* m_pStub;
			};

			interface ISafeProxy : public IObject
			{
				virtual IObject_Safe* GetSafeStub() = 0;
				virtual void Pin() = 0;
				virtual void Unpin() = 0;
				virtual IObject* ProxyQI(const guid_t& iid, bool bPartialAllowed) = 0;
			};

			template <class I_SafeProxy, class I>
			class SafeProxyImpl : public IObject
			{
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

				virtual IObject* QueryInterface(const guid_t& iid)
				{
					if (iid == OMEGA_GUIDOF(IObject))
					{
						AddRef();
						return this;
					}
					return m_contained.Internal_QueryInterface(false,iid);
				}

				static IObject* Create(IObject* pOuter, IObject_Safe* pObjS)
				{
					IObject* pRet = 0;
					OMEGA_NEW(pRet,SafeProxyImpl(pOuter,static_cast<typename interface_info<I>::safe_class*>(pObjS)));
					return pRet;
				}
			private:
				Threading::AtomicOp<uint32_t> m_refcount;
				I_SafeProxy                   m_contained;

				SafeProxyImpl(IObject* pOuter, typename interface_info<I>::safe_class* pS) :
					m_refcount(0), m_contained(pOuter,pS)
				{ }

				virtual ~SafeProxyImpl()
				{ }
			};

			template <class I>
			class IObject_SafeProxy : public I
			{
			public:
				IObject_SafeProxy(IObject* pOuter, typename interface_info<I>::safe_class* pS) :
					m_pS(pS), m_pOuter(pOuter)
				{
					m_pS->AddRef_Safe();
				}

				virtual ~IObject_SafeProxy()
				{
					m_pS->Release_Safe();
				}

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
					return Internal_QueryInterface(true,iid);
				}

				virtual IObject* Internal_QueryInterface(bool bRecurse, const guid_t& iid)
				{
					if (!bRecurse)
						return 0;

					return m_pOuter->QueryInterface(iid);
				}

			protected:
				typename interface_info<I>::safe_class* m_pS;

			private:
				IObject* m_pOuter;
			};

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega,IException,

				OMEGA_METHOD(guid_t,ThrownIID,0,())
				OMEGA_METHOD(IException*,Cause,0,())
				OMEGA_METHOD(string_t,Description,0,())
				OMEGA_METHOD(string_t,Source,0,())
			)

			struct qi_rtti
			{
				IObject_Safe* (*pfnCreateSafeStub)(SafeStub* pStub, IObject* pObj);
				IObject* (*pfnCreateSafeProxy)(IObject* pOuter, IObject_Safe* pObjS);
				void (*pfnSafeThrow)(IException_Safe* pSE);
				const wchar_t* strName;
			};

			struct qi_holder
			{
				static qi_holder& instance()
				{
					static qi_holder i;
					return i;
				}

				std::map<guid_t,const qi_rtti*> map;
			};

			inline void register_rtti_info(const guid_t& iid, const qi_rtti* pRtti)
			{
				try
				{
					qi_holder::instance().map.insert(std::map<guid_t,const qi_rtti*>::value_type(iid,pRtti));
				}
				catch (...)
				{}
			}

			inline const qi_rtti* get_qi_rtti_info(const guid_t& iid)
			{
				try
				{
					std::map<guid_t,const qi_rtti*>::const_iterator i=qi_holder::instance().map.find(iid);
					if (i == qi_holder::instance().map.end())
						return 0;
					else
						return i->second;
				}
				catch (...)
				{
					return 0;
				}
			}

			struct SafeStubMap
			{
				bool                             m_bSafetyCheck;
				Threading::ReaderWriterLock      m_lock;
				std::map<IObject*,IObject_Safe*> m_map;

				SafeStubMap() : m_bSafetyCheck(true) {}
				~SafeStubMap() { m_bSafetyCheck = false; }
			};
			inline SafeStubMap& get_stub_map();

			class SafeStub : public IObject_Safe
			{
			public:
				SafeStub(IObject* pObj) :
					m_refcount(0), m_pincount(0), m_pObj(pObj)
				{
					m_pObj->AddRef();
				}

				virtual ~SafeStub()
				{
				}

				virtual void OMEGA_CALL AddRef_Safe()
				{
					++m_refcount;
				}

				virtual void OMEGA_CALL Release_Safe()
				{
					if (--m_refcount==0)
					{
						// Remove ourselves from the stub_map
						SafeStubMap& stub_map = get_stub_map();
						if (stub_map.m_bSafetyCheck)
						{
							Threading::WriteGuard guard(stub_map.m_lock);
							stub_map.m_map.erase(m_pObj);
						}

						try
						{
							// Release all interfaces
							for (std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
							{
								i->second->Release_Safe();
							}
						}
						catch (std::exception& e)
						{
							OMEGA_THROW(e);
						}

						// If this blows up then you have failed to AddRef() an out parameter
						// or a return value.  It gets me all the time!
						m_pObj->Release();

						if (m_pincount == 0)
							delete this;
					}
				}

				inline virtual IException_Safe* OMEGA_CALL QueryInterface_Safe(const guid_t* piid, IObject_Safe** retval);

				virtual void OMEGA_CALL Pin()
				{
					++m_pincount;

					try
					{
						// Pin all interfaces
						for (std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
						{
							i->second->Pin();
						}
					}
					catch (std::exception& e)
					{
						OMEGA_THROW(e);
					}
				}

				virtual void OMEGA_CALL Unpin()
				{
					try
					{
						// Unpin all interfaces
						for (std::map<const guid_t,IObject_Safe*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
						{
							i->second->Unpin();
						}
					}
					catch (std::exception& e)
					{
						OMEGA_THROW(e);
					}

					if (--m_pincount==0 && m_refcount==0)
						delete this;
				}

			private:
				Threading::AtomicOp<uint32_t>        m_refcount;
				Threading::AtomicOp<uint32_t>        m_pincount;
				Threading::ReaderWriterLock          m_lock;
				std::map<const guid_t,IObject_Safe*> m_iid_map;
				IObject*                             m_pObj;
			};

			struct SafeProxyMap
			{
				bool                                m_bSafetyCheck;
				Threading::ReaderWriterLock         m_lock;
				std::map<IObject_Safe*,ISafeProxy*> m_map;

				SafeProxyMap() : m_bSafetyCheck(true) {}
				~SafeProxyMap() { m_bSafetyCheck = false; }
			};
			inline SafeProxyMap& get_proxy_map();

			struct SafeProxy : public ISafeProxy
			{
			public:
				SafeProxy(IObject_Safe* pObjS) :
					m_refcount(0), m_pS(pObjS)
				{
					m_pS->AddRef_Safe();
				}

				virtual void AddRef()
				{
					++m_refcount;
				}

				virtual void Release()
				{
					if (--m_refcount==0)
						delete this;
				}

				IObject_Safe* GetSafeStub()
				{
					return m_pS;
				}

				void Pin()
				{
					m_pS->Pin();
				}

				void Unpin()
				{
					m_pS->Unpin();
				}

				virtual IObject* QueryInterface(const guid_t& iid)
				{
					return ProxyQI(iid,false);
				}

				inline virtual IObject* ProxyQI(const guid_t& iid, bool bPartialAllowed);

			private:
				Threading::AtomicOp<uint32_t>    m_refcount;
				Threading::ReaderWriterLock      m_lock;
				std::map<const guid_t,IObject*>  m_iid_map;
				IObject_Safe*                    m_pS;

				virtual ~SafeProxy()
				{
					// Remove ourselves from the proxy_map
					SafeProxyMap& proxy_map = get_proxy_map();
					if (proxy_map.m_bSafetyCheck)
					{
						Threading::WriteGuard guard(proxy_map.m_lock);
						proxy_map.m_map.erase(m_pS);
					}

					try
					{
						// Release all interfaces
						for (std::map<const guid_t,IObject*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
						{
							i->second->Release();
						}
					}
					catch (std::exception& e)
					{
						OMEGA_THROW(e);
					}

					m_iid_map.clear();

					m_pS->Release_Safe();
				}

				SafeProxy(const SafeProxy& rhs) : ISafeProxy(rhs) {}
				SafeProxy& operator = (const SafeProxy&) { return *this; }
			};

			OMEGA_QI_MAGIC(Omega,IObject)
			OMEGA_QI_MAGIC(Omega,IException)
		}
	}
}

// This IID is used to detect a SafeProxy - it has no other purpose
OMEGA_SET_GUIDOF(Omega::System::MetaInfo,ISafeProxy,"{ADFB60D2-3125-4046-9EEB-0CC898E989E8}")

#endif // OOCORE_RTTI_H_INCLUDED_

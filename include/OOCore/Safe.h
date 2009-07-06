///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#ifndef OOCORE_SAFE_H_INCLUDED_
#define OOCORE_SAFE_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		inline bool PinObjectPointer(IObject* pObject);
		inline void UnpinObjectPointer(IObject* pObject);

		namespace MetaInfo
		{
			template <class I>
			class auto_iface_ptr
			{
			public:
				auto_iface_ptr(I* pI = 0) : m_pI(pI)
				{}

				auto_iface_ptr(const auto_iface_ptr& rhs) : m_pI(rhs.m_pI)
				{
					AddRef();
				}

				auto_iface_ptr& operator = (const auto_iface_ptr& rhs)
				{
					if (&rhs != this)
					{
						Release();

						m_pI = rhs.m_pI;
						
						AddRef();
					}
					return *this;
				}

				~auto_iface_ptr()
				{
					Release();
				}

				operator I*&()
				{
					return m_pI;
				}

				I* operator ->() const
				{
					return m_pI;
				}

				void attach(I* pI)
				{
					Release();
					m_pI = pI;
				}

			private:
				void AddRef()
				{
					if (m_pI)
						m_pI->AddRef();
				}

				void Release()
				{
					if (m_pI)
						m_pI->Release();
				}

				I* m_pI;
			};

			struct SafeShim
			{
				const void* m_vtable;
				void* m_stub;
				const guid_t* m_iid;
			};

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

					void update(T& dest, const guid_t*, const SafeShim*)
					{
						dest = m_val;
					}

					void update(T& dest, const guid_t*, const IObject*)
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

			template <>
			class std_safe_type<bool_t>
			{
			public:
				struct bool_2_int
				{
					bool_2_int(bool_t val = false) : m_val(val ? 1 : 0)
					{}

					void update(bool_t& dest, const guid_t*, IObject*)
					{
						dest = (m_val != 0);
					}
					
					operator int& ()
					{
						return m_val;
					}

					int* operator & ()
					{
						return &m_val;
					}

					int m_val;
				};

				struct int_2_bool
				{
					int_2_bool(int val) : m_val(val != 0)
					{}

					void update(int& dest, const guid_t*, const SafeShim*)
					{
						dest = (m_val ? 1 : 0);
					}

					operator bool_t&()
					{
						return m_val;
					}

					bool_t m_val;
				};

				typedef int type;
				typedef int_2_bool ref_type;
				typedef bool_2_int ref_safe_type;

				static bool_2_int coerce(bool_t val)
				{
					return bool_2_int(val);
				}

				static int_2_bool coerce(int val)
				{
					return int_2_bool(val);
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

			template <class T>
			class std_safe_type<T&>
			{
			public:
				class ref_holder
				{
				public:
					ref_holder(typename marshal_info<T>::safe_type::type* val, const guid_t* piid, const SafeShim* pOuter) :
						m_val(*val), m_dest(val), m_piid(piid), m_pOuter(pOuter)
					{}

					~ref_holder()
					{
						m_val.update(*m_dest,m_piid,m_pOuter);
					}

					operator T&()
					{
						return m_val;
					}

				private:
					typename marshal_info<T>::safe_type::ref_type m_val;
					typename marshal_info<T>::safe_type::type* m_dest;
					const guid_t* m_piid;
					const SafeShim* m_pOuter;
				};

				class ref_holder_safe
				{
				public:
					ref_holder_safe(T& val, const guid_t* piid = 0, IObject* pOuter = 0) : 
						m_val(val), m_dest(&val), m_piid(piid), m_pOuter(pOuter)
					{}

					~ref_holder_safe()
					{
						m_val.update(*m_dest,m_piid,m_pOuter);
					}

					operator typename marshal_info<T>::safe_type::type*()
					{
						return &m_val;
					}

				private:
					typename marshal_info<T>::safe_type::ref_safe_type m_val;
					T*            m_dest;
					const guid_t* m_piid;
					IObject*      m_pOuter;
				};
				typedef typename marshal_info<T>::safe_type::type* type;

				static ref_holder_safe coerce(T& val)
				{
					return ref_holder_safe(val);
				}

				static ref_holder_safe coerce(T& val, const guid_t& iid, IObject* pOuter = 0)
				{
					return ref_holder_safe(val,&iid,pOuter);
				}

				static ref_holder coerce(type val, const guid_t* piid = 0, const SafeShim* pOuter = 0)
				{
					return ref_holder(val,piid,pOuter);
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

				static const T& coerce(type val)
				{
					return marshal_info<T>::safe_type::coerce(*const_cast<typename marshal_info<T>::safe_type::type*>(val));
				}
			};

			template <>
			class std_safe_type<const bool_t&>
			{
			public:
				typedef const int type;

				static const int coerce(const bool_t& val)
				{
					return (val ? 1 : 0);
				}

				static const bool_t coerce(const int val)
				{
					return (val != 0);
				}
			};

			struct ISafeProxy : public IObject
			{
				virtual void Pin() = 0;
				virtual void Unpin() = 0;
				virtual const SafeShim* GetStub(const Omega::guid_t& iid) = 0;
			};

			class Safe_Proxy_Owner;
			
			inline IObject* create_proxy(const SafeShim* shim, IObject* pOuter = 0);
			
			class Safe_Stub_Owner;

			inline const SafeShim* create_stub(IObject* proxy, const guid_t& iid);

			inline void throw_correct_exception(const SafeShim* except);
			inline const SafeShim* return_safe_exception(IException* pE);

			struct IObject_Safe_VTable
			{
				const SafeShim* (OMEGA_CALL* pfnAddRef_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnRelease_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnQueryInterface_Safe)(const SafeShim* shim, const SafeShim** retval, const guid_t* iid);
				const SafeShim* (OMEGA_CALL* pfnGetBaseShim_Safe)(const SafeShim* shim, const SafeShim** retval);
				const SafeShim* (OMEGA_CALL* pfnPin_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnUnpin_Safe)(const SafeShim* shim);
			};

			class auto_safe_shim
			{
			public:
				auto_safe_shim(const SafeShim* pS = 0) : m_pS(pS)
				{}

				auto_safe_shim(const auto_safe_shim& rhs) : m_pS(rhs.m_pS)
				{
					AddRef();
				}

				auto_safe_shim& operator = (const auto_safe_shim& rhs)
				{
					if (&rhs != this)
					{
						Release();

						m_pS = rhs.m_pS;
						
						AddRef();
					}
					return *this;
				}

				~auto_safe_shim()
				{
					Release();
				}

				operator const SafeShim*&()
				{
					return m_pS;
				}

				const SafeShim** operator &()
				{
					return &m_pS;
				}

				const SafeShim* operator ->()
				{
					return m_pS;
				}

				void attach(const SafeShim* pS)
				{
					Release();
					m_pS = pS;
				}

				const SafeShim* detach()
				{
					const SafeShim* ret = m_pS;
					m_pS = 0;
					return ret;
				}

			private:
				const SafeShim* m_pS;

				void AddRef()
				{
					if (m_pS)
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnAddRef_Safe(m_pS);
						if (except)
							throw_correct_exception(except);
					}
				}

				void Release()
				{
					if (m_pS)
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnRelease_Safe(m_pS);
						if (except)
							throw_correct_exception(except);
					}
				}
			};

			template <class I>
			class iface_proxy_functor
			{
			public:
				iface_proxy_functor(I* pI, const guid_t& iid) :
					m_pS(0)
				{
					m_pS = create_stub(pI,iid);
				}

				iface_proxy_functor(const iface_proxy_functor& rhs) :
					m_pS(rhs.m_pS)
				{
					if (m_pS)
						static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnAddRef_Safe(m_pS);
				}

				~iface_proxy_functor()
				{
					if (m_pS)
						static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnRelease_Safe(m_pS);
				}

				operator const SafeShim* ()
				{
					return m_pS;
				}

				const SafeShim* operator -> ()
				{
					return m_pS;
				}

			protected:
				const SafeShim* m_pS;

				iface_proxy_functor& operator = (const iface_proxy_functor&) {}
			};

			template <class I>
			class iface_proxy_functor_ref : public iface_proxy_functor<I>
			{
			public:
				iface_proxy_functor_ref(I* pI) :
					iface_proxy_functor<I>(pI,OMEGA_GUIDOF(I))
				{
				}

				void update(I*& pI, const guid_t*, IObject* pOuter)
				{
					if (pI)
						pI->Release();

					pI = static_cast<I*>(create_proxy(this->m_pS,pOuter));
				}

				const SafeShim** operator & ()
				{
					return &this->m_pS;
				}
			};

			template <class I>
			class iface_stub_functor
			{
			public:
				iface_stub_functor(const SafeShim* pS, const guid_t*) :
					m_pI(0)
				{
					m_pI = static_cast<I*>(create_proxy(pS));
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
				iface_stub_functor_ref(const SafeShim* pS) :
				  iface_stub_functor<I>(pS,0)
				{
				}

				void update(const SafeShim*& pS, const guid_t* piid, const SafeShim*)
				{
					if (pS)
						static_cast<const IObject_Safe_VTable*>(pS->m_vtable)->pfnRelease_Safe(pS);
					
					pS = create_stub(this->m_pI,piid ? *piid : OMEGA_GUIDOF(I));
				}

				operator I*& ()
				{
					return this->m_pI;
				}
			};	

			template <class I>
			class iface_safe_type
			{
			public:
				typedef const SafeShim* type;

				typedef iface_proxy_functor_ref<I> ref_safe_type;
				typedef iface_stub_functor_ref<I> ref_type;

				static iface_proxy_functor<I> coerce(I* val, const guid_t& iid = OMEGA_GUIDOF(I))
				{
					return iface_proxy_functor<I>(val,iid);
				}

				static iface_stub_functor<I> coerce(type val, const guid_t* piid = &OMEGA_GUIDOF(I))
				{
					return iface_stub_functor<I>(val,piid);
				}
			};

			template <class I> class iface_wire_type;

			template <> struct marshal_info<IObject*>
			{
				typedef iface_safe_type<IObject> safe_type;
				typedef iface_wire_type<IObject> wire_type;
			};

			template <class I> struct vtable_info;

			template <> struct vtable_info<IObject>
			{
				typedef IObject_Safe_VTable type;
			};

			struct Safe_Proxy_Base
			{
				virtual void IncRef() = 0;
				virtual void DecRef() = 0;
				virtual bool IsDerived(const guid_t& iid) const = 0;
				virtual IObject* QIReturn() = 0;
				virtual const SafeShim* GetShim() = 0;
				virtual void Throw() = 0;
			};

			struct Safe_Stub_Base
			{
				virtual void IncRef() = 0;
				virtual void DecRef() = 0;
				virtual bool IsDerived(const guid_t& iid) const = 0;
				virtual const SafeShim* GetShim() = 0;
			};

			class Safe_Proxy_Owner : public IObject
			{
			public:
				Safe_Proxy_Owner(const SafeShim* shim, IObject* pOuter) : 
					m_shim(shim), m_pOuter(pOuter)
				{
					//printf("SPO %p Init with SSO %p (%p)\n\t",this,shim->m_stub,pOuter);

					m_agg_object.m_pOwner = this;
					m_internal_safe_proxy.m_pOwner = this;

					Internal_AddRef();
				}

				void AddRef()
				{
					if (m_pOuter)
					{
						//printf("SPO %p Owner AddRef\t\n",this);

						m_pOuter->AddRef();
					}
					
					Internal_AddRef();					
				}

				void Internal_AddRef()
				{
					//printf("SPO %p AddRef > %u P: %u\n",this,m_refcount.m_debug_value+1,m_pincount.m_debug_value);

					if (m_refcount.AddRef())
					{
						//printf("\t");

						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnAddRef_Safe(m_shim);
						if (except)
							throw_correct_exception(except);
					}
				}

				void Private_AddRef()
				{
					m_privcount.AddRef();
				}

				void Release()
				{
					if (m_pOuter)
					{
						//printf("SPO %p Owner Release\t\n",this);
						
						m_pOuter->Release();
					}
					
					Internal_Release();
				}

				void Internal_Release()
				{
					assert(m_refcount.m_debug_value > 0);

					//printf("SPO %p Release < %u P: %u\n",this,m_refcount.m_debug_value-1,m_pincount.m_debug_value);

					if (m_refcount.Release())
					{
						//printf("\t");

						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnRelease_Safe(m_shim);
						if (except)
							throw_correct_exception(except);

						if (m_pincount.IsZero() && m_privcount.IsZero())
							delete this;
					}
				}

				void Private_Release()
				{
					assert(m_privcount.m_debug_value > 0);

					if (m_privcount.Release() && m_pincount.IsZero() && m_refcount.IsZero())
						delete this;
				}

				inline IObject* QueryInterface(const guid_t& iid)
				{
					return QueryInterface(iid,0);
				}

				inline IObject* QueryInterface(const guid_t& iid, const SafeShim* shim);
				
				void Pin()
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnPin_Safe(m_shim);
					if (except)
						throw_correct_exception(except);

					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(m_pincount.m_debug_value > 0);

					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnUnpin_Safe(m_shim);
					if (except)
						throw_correct_exception(except);

					if (m_pincount.Release() && m_refcount.IsZero() && m_privcount.IsZero())
						delete this;
				}

				inline void Throw(const guid_t& iid, const SafeShim* shim);

			private:
				Safe_Proxy_Owner(const Safe_Proxy_Owner&) {};
				Safe_Proxy_Owner& operator =(const Safe_Proxy_Owner&) { return *this; };

				Threading::Mutex                  m_lock;
				std::map<guid_t,Safe_Proxy_Base*> m_iid_map;
				const SafeShim*                   m_shim;
				IObject*                          m_pOuter;
				Threading::AtomicRefCount         m_refcount;
				Threading::AtomicRefCount         m_pincount;
				Threading::AtomicRefCount         m_privcount;
				
				class AggObject : public IObject
				{
				public:
					AggObject() : m_pOwner(0)
					{}

					void AddRef()
					{
						m_pOwner->Internal_AddRef();
					}

					void Release()
					{
						m_pOwner->Internal_Release();
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						return m_pOwner->QueryInterface(iid);
					}

					Safe_Proxy_Owner* m_pOwner;
				};
				AggObject m_agg_object;

				class InternalSafeProxy : public ISafeProxy
				{
				public:
					InternalSafeProxy() : m_pOwner(0)
					{}

					void AddRef()
					{
						m_pOwner->Private_AddRef();
					}

					void Release()
					{
						m_pOwner->Private_Release();
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						return m_pOwner->QueryInterface(iid);
					}

					void Pin()
					{
						m_pOwner->Pin();
					}

					void Unpin()
					{
						m_pOwner->Unpin();
					}

					const SafeShim* GetStub(const Omega::guid_t& iid)
					{
						return m_pOwner->GetStub(iid);
					}

					Safe_Proxy_Owner* m_pOwner;
				};
				InternalSafeProxy m_internal_safe_proxy;
				
				inline virtual ~Safe_Proxy_Owner();

				inline Safe_Proxy_Base* GetBase(const Omega::guid_t& iid, const SafeShim* shim = 0);

				const SafeShim* GetStub(const Omega::guid_t& iid)
				{
					if (iid == OMEGA_GUIDOF(IObject))
					{
						Internal_AddRef();
						return m_shim;
					}

					Safe_Proxy_Base* pBase = GetBase(iid);
					if (!pBase)
						return 0;
					
					// AddRef the shim
					const SafeShim* shim = pBase->GetShim();
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnAddRef_Safe(shim);
					if (except)
						throw_correct_exception(except);

					return shim;
				}
			};

			class proxy_holder
			{
			public:
				inline void remove(const SafeShim* shim);
				inline Safe_Proxy_Owner* find(const SafeShim* shim);
				inline Safe_Proxy_Owner* add(const SafeShim* shim, Safe_Proxy_Owner* pOwner);

				bool singleton_init() { return false; }

			private:
				Threading::Mutex                            m_lock;
				std::map<const SafeShim*,Safe_Proxy_Owner*> m_map;
			};
			typedef Threading::Singleton<proxy_holder> PROXY_HOLDER;

			inline Safe_Proxy_Owner* create_proxy_owner(const SafeShim* shim, IObject* pOuter);

			class Safe_Stub_Owner
			{
			public:
				Safe_Stub_Owner(IObject* pObject) :
					m_pObject(pObject)
				{
					static const IObject_Safe_VTable vt = 
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&GetBaseShim_Safe,
						&Pin_Safe,
						&Unpin_Safe
					};
					m_shim.m_vtable = &vt;
					m_shim.m_stub = this;
					m_shim.m_iid = &OMEGA_GUIDOF(IObject);

					//printf("SSO %p Init with Obj %p\n\t",this,m_pObject);

					AddRef();
				}

				virtual void AddRef()
				{
					//printf("SSO %p AddRef > %u P: %u\n",this,m_refcount.m_debug_value+1,m_pincount.m_debug_value);

					if (m_refcount.AddRef())
						m_pObject->AddRef();
				}

				virtual void Release()
				{
					assert(m_refcount.m_debug_value > 0);
					
					//printf("SSO %p Release < %u P: %u\n",this,m_refcount.m_debug_value-1,m_pincount.m_debug_value);

					if (m_refcount.Release())
					{
						// Release our pointer
						m_pObject->Release();		

						//printf("\t");
						
						if (m_pincount.IsZero())
							delete this;
					}
				}

				const SafeShim* QueryInterface(const guid_t& iid)
				{
					return QueryInterface(iid,0);
				}

				inline const SafeShim* QueryInterface(const guid_t& iid, IObject* pObj);

				const SafeShim* GetBaseShim() const
				{
					return &m_shim;
				}

				void Pin()
				{
					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(m_pincount.m_debug_value > 0);

					if (m_pincount.Release() && m_refcount.IsZero())
						delete this;
				}

			private:
				Safe_Stub_Owner(const Safe_Stub_Owner&) {};
				Safe_Stub_Owner& operator =(const Safe_Stub_Owner&) { return *this; };

				Threading::Mutex                 m_lock;
				std::map<guid_t,Safe_Stub_Base*> m_iid_map;
				IObject*                         m_pObject;
				Threading::AtomicRefCount        m_refcount;
				Threading::AtomicRefCount        m_pincount;
				SafeShim                         m_shim;

				inline virtual ~Safe_Stub_Owner();

				inline Safe_Stub_Base* GetBase(const guid_t& iid, IObject* pObj);

				static const SafeShim* OMEGA_CALL AddRef_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub_Owner*>(shim->m_stub)->AddRef();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Release_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub_Owner*>(shim->m_stub)->Release();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL QueryInterface_Safe(const SafeShim* shim, const SafeShim** retval, const guid_t* iid)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub_Owner*>(shim->m_stub)->QueryInterface(*iid);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL GetBaseShim_Safe(const SafeShim* shim, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub_Owner*>(shim->m_stub)->GetBaseShim();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Pin_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub_Owner*>(shim->m_stub)->Pin();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Unpin_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub_Owner*>(shim->m_stub)->Unpin();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
			};

			inline auto_iface_ptr<Safe_Stub_Owner> create_stub_owner(IObject* pObject);

			struct qi_rtti
			{
				Safe_Proxy_Base* (*pfnCreateSafeProxy)(const SafeShim* shim, Safe_Proxy_Owner* pOwner);
				Safe_Stub_Base* (*pfnCreateSafeStub)(IObject* pI, Safe_Stub_Owner* pOwner);
				const wchar_t* pszName;
			};

			struct qi_holder
			{
				static qi_holder& instance()
				{
					// This is a 'cheap' singleton as we only use it to read
					// after the dll/so is init'ed
					static qi_holder i;
					return i;
				}

				std::map<guid_t,const qi_rtti*> iid_map;
			};

			inline static const qi_rtti* get_qi_rtti_info(const guid_t& iid)
			{
				try
				{
					qi_holder& instance = qi_holder::instance();
					std::map<guid_t,const qi_rtti*>::const_iterator i=instance.iid_map.find(iid);
					if (i != instance.iid_map.end())
						return i->second;
				}
				catch (...)
				{}
				
				return 0;
			}

			template <class I, class D>
			class Safe_Proxy;

			template <class D>
			class Safe_Proxy<IObject,D> : public Safe_Proxy_Base, public D
			{
			public:
				static Safe_Proxy_Base* bind(const SafeShim* shim, Safe_Proxy_Owner* pOwner)
				{
					Safe_Proxy* pThis;
					OMEGA_NEW(pThis,Safe_Proxy(shim,pOwner));
					return pThis;
				}

			protected:
				Safe_Proxy(const SafeShim* shim, Safe_Proxy_Owner* pOwner) : m_shim(shim), m_pOwner(pOwner)
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnAddRef_Safe(m_shim);
					if (except)
						throw_correct_exception(except);

					m_refcount.AddRef();
				}

				virtual ~Safe_Proxy()
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnRelease_Safe(m_shim);
					if (except)
						throw_correct_exception(except);
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

				const SafeShim* m_shim;

			private:
				Threading::AtomicRefCount m_refcount;
				Safe_Proxy_Owner*         m_pOwner;

				Safe_Proxy(const Safe_Proxy&) {};
				Safe_Proxy& operator =(const Safe_Proxy&) { return *this; };
				
				void IncRef()
				{
					m_refcount.AddRef();
				}

				void DecRef()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release())
						delete this;
				}

				IObject* QIReturn()
				{
					return static_cast<D*>(this);
				}

				const SafeShim* GetShim()
				{
					return m_shim;
				}

				void Throw()
				{
					throw static_cast<D*>(this);
				}
	
			// IObject members
			public:
				virtual void AddRef()
				{
					m_pOwner->AddRef();
				}

				virtual void Release()
				{
					m_pOwner->Release();
				}

				virtual IObject* QueryInterface(const guid_t& iid)
				{
					return m_pOwner->QueryInterface(iid);
				}
			};

			template <class I>
			class Safe_Stub;

			template <>
			class Safe_Stub<IObject> : public Safe_Stub_Base
			{
			public:
				static Safe_Stub_Base* create(IObject* pI, Safe_Stub_Owner* pOwner)
				{
					Safe_Stub* pThis;
					OMEGA_NEW(pThis,Safe_Stub(pI,&OMEGA_GUIDOF(IObject),pOwner));
					return pThis;					
				}

			protected:
				Safe_Stub(IObject* pI, const guid_t* iid, Safe_Stub_Owner* pOwner) : 
					 m_pI(pI), m_pOwner(pOwner)
				{
					IncRef(); 

					m_shim.m_vtable = get_vt();
					m_shim.m_stub = this;
					m_shim.m_iid = iid;
				}

				virtual ~Safe_Stub()
				{
				}
				
				static const IObject_Safe_VTable* get_vt()
				{
					static const IObject_Safe_VTable vt = 
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&GetBaseShim_Safe,
						&Pin_Safe,
						&Unpin_Safe
					};
					return &vt;
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}
				
				SafeShim         m_shim;
				IObject*         m_pI;
				
			private:
				Safe_Stub_Owner*          m_pOwner;
				Threading::AtomicRefCount m_refcount;

				Safe_Stub(const Safe_Stub&) {};
				Safe_Stub& operator =(const Safe_Stub&) { return *this; };

				void IncRef()
				{
					//printf("SS %p IncRef > %u\n",this,m_refcount.m_debug_value+1);

					m_refcount.AddRef();
				}

				void DecRef()
				{
					assert(m_refcount.m_debug_value > 0);

					//printf("SS %p DecRef < %u\n",this,m_refcount.m_debug_value-1);

					if (m_refcount.Release())
						delete this;
				}

				const SafeShim* GetShim()
				{
					return &m_shim;
				}

				void AddRef()
				{
					m_pOwner->AddRef();
				}

				void Release()
				{
					m_pOwner->Release();
				}

				const SafeShim* QueryInterface(const guid_t& iid)
				{
					if (IsDerived(iid))
					{
						AddRef();
						return GetShim();
					}

					return m_pOwner->QueryInterface(iid);
				}

				void Pin()
				{
					m_pOwner->Pin();
				}

				void Unpin()
				{
					m_pOwner->Unpin();
				}

				static const SafeShim* OMEGA_CALL AddRef_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub*>(shim->m_stub)->AddRef();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Release_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub*>(shim->m_stub)->Release();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL QueryInterface_Safe(const SafeShim* shim, const SafeShim** retval, const guid_t* iid)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub*>(shim->m_stub)->QueryInterface(*iid);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL GetBaseShim_Safe(const SafeShim* shim, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub*>(shim->m_stub)->m_pOwner->GetBaseShim();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Pin_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub*>(shim->m_stub)->Pin();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Unpin_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Safe_Stub*>(shim->m_stub)->Unpin();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
			};
						
			OMEGA_DECLARE_FORWARDS(Omega,IException)
			OMEGA_DECLARE_FORWARDS(Omega::TypeInfo,ITypeInfo)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega, IException,

				OMEGA_METHOD(guid_t,GetThrownIID,0,())
				OMEGA_METHOD(IException*,GetCause,0,())
				OMEGA_METHOD(string_t,GetDescription,0,())
				OMEGA_METHOD(string_t,GetSource,0,())
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE
			(
				Omega::TypeInfo, ITypeInfo,

				OMEGA_METHOD(string_t,GetName,0,())
				OMEGA_METHOD(guid_t,GetIID,0,())
				OMEGA_METHOD(Omega::TypeInfo::ITypeInfo*,GetBaseType,0,())
				OMEGA_METHOD(uint32_t,GetMethodCount,0,())				
				OMEGA_METHOD_VOID(GetMethodInfo,6,((in),uint32_t,method_idx,(out),string_t&,strName,(out),TypeInfo::MethodAttributes_t&,attribs,(out),uint32_t&,timeout,(out),byte_t&,param_count,(out),TypeInfo::Types_t&,return_type))
				OMEGA_METHOD_VOID(GetParamInfo,5,((in),uint32_t,method_idx,(in),byte_t,param_idx,(out),string_t&,strName,(out),TypeInfo::Types_t&,type,(out),TypeInfo::ParamAttributes_t&,attribs))
				OMEGA_METHOD(byte_t,GetAttributeRef,3,((in),uint32_t,method_idx,(in),byte_t,param_idx,(in),TypeInfo::ParamAttributes_t,attrib))
				OMEGA_METHOD(guid_t,GetParamIid,2,((in),uint32_t,method_idx,(in),byte_t,param_idx))
			)

			inline static void register_rtti_info(const guid_t& iid, const qi_rtti* pRtti)
			{
				try
				{
					qi_holder::instance().iid_map.insert(std::map<guid_t,const qi_rtti*>::value_type(iid,pRtti));
				}
				catch (std::exception& e)
				{
					OMEGA_THROW(e);
				}
			}

			OMEGA_QI_MAGIC(Omega,IObject)
			OMEGA_QI_MAGIC(Omega,IException)
			OMEGA_QI_MAGIC(Omega::TypeInfo,ITypeInfo)			
		}
	}
}

// This IID is used to detect a SafeProxy - it has no other purpose
OMEGA_SET_GUIDOF(Omega::System::MetaInfo,ISafeProxy,"{ADFB60D2-3125-4046-9EEB-0CC898E989E8}")

#endif // OOCORE_SAFE_H_INCLUDED_

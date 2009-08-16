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
			template <typename I>
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

			template <typename T>
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

			template <typename T> class std_wire_type;
			template <typename T> class std_wire_type_array;

			template <typename T> struct marshal_info
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

			template <typename T> struct marshal_info<T*>
			{
				typedef std_safe_type<T*> safe_type;
				typedef std_wire_type_array<T> wire_type;
			};

			template <typename T>
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

			template <typename T>
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
				virtual const SafeShim* GetShim(const Omega::guid_t& iid) = 0;
				virtual const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid) = 0;
			};

			inline void throw_correct_exception(const SafeShim* except);
			inline const SafeShim* return_safe_exception(IException* pE);

			struct IObject_Safe_VTable
			{
				const SafeShim* (OMEGA_CALL* pfnAddRef_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnRelease_Safe)(const SafeShim* shim);
				const SafeShim* (OMEGA_CALL* pfnQueryInterface_Safe)(const SafeShim* shim, const SafeShim** retval, const guid_t* iid);
				const SafeShim* (OMEGA_CALL* pfnGetBaseShim_Safe)(const SafeShim* shim, const SafeShim** retval);
				const SafeShim* (OMEGA_CALL* pfnCreateWireStub_Safe)(const SafeShim* shim, const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const SafeShim** retval);
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

			template <typename I>
			class iface_proxy_functor
			{
			public:
				iface_proxy_functor(I* pI, const guid_t& iid) :
					m_pS(0)
				{
					m_pS = create_safe_stub(pI,iid);
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

				iface_proxy_functor& operator = (const iface_proxy_functor&);
			};

			template <typename I>
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

					pI = static_cast<I*>(create_safe_proxy(this->m_pS,pOuter));
				}

				const SafeShim** operator & ()
				{
					return &this->m_pS;
				}
			};

			inline IObject* create_safe_proxy(const SafeShim* shim, IObject* pOuter = 0);

			template <typename I>
			class iface_stub_functor
			{
			public:
				iface_stub_functor(const SafeShim* pS, const guid_t*) :
					m_pI(0)
				{
					m_pI = static_cast<I*>(create_safe_proxy(pS));
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

				iface_stub_functor& operator = (const iface_stub_functor&);
			};

			template <typename I>
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
					
					pS = create_safe_stub(this->m_pI,piid ? *piid : OMEGA_GUIDOF(I));
				}

				operator I*& ()
				{
					return this->m_pI;
				}
			};	

			template <typename I>
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

			template <typename I> class iface_wire_type;

			template <> struct marshal_info<IObject*>
			{
				typedef iface_safe_type<IObject> safe_type;
				typedef iface_wire_type<IObject> wire_type;
			};

			template <typename I> struct vtable_info;

			template <> struct vtable_info<IObject>
			{
				typedef IObject_Safe_VTable type;
			};

			class Safe_Proxy_Owner;
			
			class Safe_Proxy_Base
			{
			public:
				virtual bool IsDerived(const guid_t& iid) const = 0;
				virtual IObject* QIReturn() = 0;
				virtual void Throw() = 0;

				void AddRef()
				{
					if (m_refcount.AddRef())
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnAddRef_Safe(m_shim);
						if (except)
							throw_correct_exception(except);
					}
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release())
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnRelease_Safe(m_shim);
						if (except)
							throw_correct_exception(except);

						if (m_pincount.IsZero())
							delete this;
					}
				}

				inline IObject* QueryInterface(const guid_t& iid);

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

					if (m_pincount.Release() && m_refcount.IsZero())
						delete this;
				}

				inline const SafeShim* GetShim(const Omega::guid_t& iid);
				inline const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid);
				
				const SafeShim* GetShim()
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnAddRef_Safe(m_shim);
					if (except)
						throw_correct_exception(except);
				
					return m_shim;
				}

				const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller)
				{
					const SafeShim* ret = 0;
					if (static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnCreateWireStub_Safe)
					{
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnCreateWireStub_Safe(m_shim,shim_Controller,shim_Marshaller,&ret);
						if (except)
							throw_correct_exception(except);
					}
				
					return ret;
				}

			protected:
				Safe_Proxy_Base(const SafeShim* shim, Safe_Proxy_Owner* pOwner) : m_shim(shim), m_pOwner(pOwner)
				{
					m_internal.m_pOuter = this;
					AddRef();
				}

				inline virtual ~Safe_Proxy_Base();

				const SafeShim* m_shim;
				
			private:
				Safe_Proxy_Base(const Safe_Proxy_Base&);
				Safe_Proxy_Base& operator =(const Safe_Proxy_Base&);	

				Safe_Proxy_Owner*         m_pOwner;
				Threading::AtomicRefCount m_refcount;
				Threading::AtomicRefCount m_pincount;

				struct Internal : public ISafeProxy
				{
					void AddRef() 
					{ 
						m_pOuter->AddRef(); 
					}

					void Release() 
					{ 
						m_pOuter->Release(); 
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						return m_pOuter->QueryInterface(iid);
					}

					void Pin()
					{
						m_pOuter->Pin();
					}

					void Unpin()
					{
						m_pOuter->Unpin();
					}

					const SafeShim* GetShim(const Omega::guid_t& iid)
					{
						return m_pOuter->GetShim(iid);
					}

					const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid)
					{
						return m_pOuter->CreateWireStub(shim_Controller,shim_Marshaller,iid);
					}

					Safe_Proxy_Base* m_pOuter;
				};
				Internal m_internal;
			};

			template <typename I, typename D>
			class Safe_Proxy;

			template <typename D>
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
				Safe_Proxy(const SafeShim* shim, Safe_Proxy_Owner* pOwner) : 
					 Safe_Proxy_Base(shim,pOwner)
				{}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

			private:
				IObject* QIReturn()
				{
					AddRef();
					return static_cast<D*>(this);
				}

				void Throw()
				{
					AddRef();
					throw static_cast<D*>(this);
				}
	
			// IObject members
			public:
				virtual void AddRef()
				{
					Safe_Proxy_Base::AddRef();
				}

				virtual void Release()
				{
					Safe_Proxy_Base::Release();
				}

				virtual IObject* QueryInterface(const guid_t& iid)
				{
					if (iid != OMEGA_GUIDOF(IObject) && IsDerived(iid))
						return QIReturn();
					
					return Safe_Proxy_Base::QueryInterface(iid);
				}
			};

			class Safe_Stub_Owner;

			inline const SafeShim* create_safe_stub(IObject* pObject, const guid_t& iid);

			class Safe_Stub_Base
			{
			public:
				void AddRef()
				{
					if (m_refcount.AddRef())
						m_pI->AddRef();
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release())
					{
						m_pI->Release();

						if (m_pincount.IsZero())
							delete this;
					}
				}

				const SafeShim* GetShim()
				{
					AddRef();
					return &m_shim;
				}

				virtual bool IsDerived(const guid_t& iid) const = 0;
				
			protected:
				SafeShim         m_shim;
				IObject*         m_pI;
				
				Safe_Stub_Base(IObject* pI, Safe_Stub_Owner* pOwner) : 
					 m_pI(pI), m_pOwner(pOwner)
				{
					AddRef(); 
				}

				inline virtual ~Safe_Stub_Base();

				inline const SafeShim* QueryInterface(const guid_t& iid);

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

				inline const SafeShim* GetBaseShim();

				virtual const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller) = 0;
								
			private:
				Safe_Stub_Owner*          m_pOwner;
				Threading::AtomicRefCount m_refcount;
				Threading::AtomicRefCount m_pincount;

				Safe_Stub_Base(const Safe_Stub_Base&);
				Safe_Stub_Base& operator =(const Safe_Stub_Base&);
			};

			template <typename I>
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
					 Safe_Stub_Base(pI,pOwner)
				{
					m_shim.m_vtable = get_vt();
					m_shim.m_stub = this;
					m_shim.m_iid = iid;
				}

				static const IObject_Safe_VTable* get_vt()
				{
					static const IObject_Safe_VTable vt = 
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&GetBaseShim_Safe,
						&CreateWireStub_Safe,
						&Pin_Safe,
						&Unpin_Safe
					};
					return &vt;
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}
				
			private:
				virtual const SafeShim* CreateWireStub(const SafeShim*, const SafeShim*)
				{
					OMEGA_THROW(L"Attempting to remote IObject?");
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
						*retval = static_cast<Safe_Stub*>(shim->m_stub)->GetBaseShim();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL CreateWireStub_Safe(const SafeShim* shim, const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub*>(shim->m_stub)->CreateWireStub(shim_Controller,shim_Marshaller);
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

			class Safe_Proxy_Owner
			{
			public:
				Safe_Proxy_Owner(const SafeShim* shim, IObject* pOuter) : m_base_shim(shim), m_pOuter(pOuter)
				{
					// Pin the base_shim
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_base_shim->m_vtable)->pfnPin_Safe(m_base_shim);
					if (except)
						throw_correct_exception(except);

					m_internal.m_pOwner = this;	
					m_safe_proxy.m_pOwner = this;	
					AddRef();
				}

				void AddRef()
				{
					m_refcount.AddRef();
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release() && m_pincount.IsZero() && ListEmpty())
						delete this;
				}

				inline void RemoveBase(Safe_Proxy_Base* pProxy);
				inline IObject* QueryInterface(const guid_t& iid);
				inline IObject* CreateProxy(const SafeShim* shim);
				inline void Throw(const SafeShim* shim);
				inline const SafeShim* GetShim(const guid_t& iid);
				inline const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid);

			private:
				Safe_Proxy_Owner(const Safe_Proxy_Owner&);
				Safe_Proxy_Owner& operator =(const Safe_Proxy_Owner&);

				Threading::Mutex                  m_lock;
				std::map<guid_t,Safe_Proxy_Base*> m_iid_map;
				const SafeShim*                   m_base_shim;
				IObject*                          m_pOuter;
				Threading::AtomicRefCount         m_refcount;
				Threading::AtomicRefCount         m_pincount;

				struct Internal : public IObject
				{
					void AddRef()
					{
						if (m_refcount.AddRef())
						{
							m_pOwner->AddRef();

							const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pOwner->m_base_shim->m_vtable)->pfnAddRef_Safe(m_pOwner->m_base_shim);
							if (except)
								throw_correct_exception(except);
						}
					}

					void Release()
					{
						assert(m_refcount.m_debug_value > 0);

						if (m_refcount.Release())
						{
							const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pOwner->m_base_shim->m_vtable)->pfnRelease_Safe(m_pOwner->m_base_shim);
							if (except)
								throw_correct_exception(except);

							m_pOwner->Release();
						}
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						return m_pOwner->QueryInterface(iid);
					}

					Safe_Proxy_Owner* m_pOwner;

				private:
					Threading::AtomicRefCount m_refcount;
				};
				//friend struct Internal;
				Internal m_internal;

				struct SafeProxy : public ISafeProxy
				{
					void AddRef()
					{
						m_pOwner->AddRef();
					}

					void Release()
					{
						m_pOwner->Release();
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

					const SafeShim* GetShim(const Omega::guid_t& iid)
					{
						return m_pOwner->GetShim(iid);
					}

					const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid)
					{
						return m_pOwner->CreateWireStub(shim_Controller,shim_Marshaller,iid);
					}

					Safe_Proxy_Owner* m_pOwner;
				};
				//friend struct SafeProxy;
				SafeProxy m_safe_proxy;
								
				inline virtual ~Safe_Proxy_Owner();

				void Pin()
				{
					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(m_pincount.m_debug_value > 0);

					if (m_pincount.Release() && m_refcount.IsZero() && ListEmpty())
						delete this;
				}

				bool ListEmpty()
				{
					try
					{
						Threading::Guard<Threading::Mutex> guard(m_lock);
						return m_iid_map.empty();
					}
					catch (std::exception& e)
					{
						OMEGA_THROW(e);
					}
				}

				inline auto_iface_ptr<Safe_Proxy_Base> GetProxyBase(const guid_t& iid, const SafeShim* shim, bool bAllowPartial);
			};

			inline Omega::System::MetaInfo::auto_iface_ptr<Omega::System::MetaInfo::Safe_Proxy_Owner> create_safe_proxy_owner(const SafeShim* shim, IObject* pOuter);
			
			class Safe_Stub_Owner
			{
			public:
				Safe_Stub_Owner(IObject* pObject) :
					m_pI(pObject)
				{
					static const IObject_Safe_VTable vt = 
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&GetBaseShim_Safe,
						0,
						&Pin_Safe,
						&Unpin_Safe
					};
					m_base_shim.m_vtable = &vt;
					m_base_shim.m_stub = this;
					m_base_shim.m_iid = &OMEGA_GUIDOF(IObject);

					AddRef();
				}

				virtual void AddRef()
				{
					if (m_refcount.AddRef())
						m_pI->AddRef();
				}

				virtual void Release()
				{
					assert(m_refcount.m_debug_value > 0);
					
					if (m_refcount.Release())
					{
						m_pI->Release();

						if (m_pincount.IsZero() && ListEmpty())
							delete this;
					}
				}

				inline const SafeShim* QueryInterface(const guid_t& iid, IObject* pObj);

				const SafeShim* GetBaseShim()
				{
					AddRef();
					return &m_base_shim;
				}

				void Pin()
				{
					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(m_pincount.m_debug_value > 0);

					if (m_pincount.Release() && m_refcount.IsZero() && ListEmpty())
						delete this;
				}

				inline void RemoveBase(Safe_Stub_Base* pStub);

			private:
				Safe_Stub_Owner(const Safe_Stub_Owner&);
				Safe_Stub_Owner& operator =(const Safe_Stub_Owner&);

				Threading::Mutex                 m_lock;
				std::map<guid_t,Safe_Stub_Base*> m_iid_map;
				IObject*                         m_pI;
				Threading::AtomicRefCount        m_refcount;
				Threading::AtomicRefCount        m_pincount;
				SafeShim                         m_base_shim;

				inline virtual ~Safe_Stub_Owner();

				inline auto_iface_ptr<Safe_Stub_Base> GetStubBase(const guid_t& iid, IObject* pObj);

				bool ListEmpty()
				{
					try
					{
						Threading::Guard<Threading::Mutex> guard(m_lock);
						return m_iid_map.empty();
					}
					catch (std::exception& e)
					{
						OMEGA_THROW(e);
					}
				}

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
						*retval = static_cast<Safe_Stub_Owner*>(shim->m_stub)->QueryInterface(*iid,0);
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

			inline auto_iface_ptr<Safe_Stub_Owner> create_safe_stub_owner(IObject* pObject);

			struct OMEGA_PRIVATE_TYPE(safe_module)
			{
				int unused;
			};

			class safe_proxy_holder
			{
			public:
				inline void remove(const SafeShim* shim);
				inline auto_iface_ptr<Safe_Proxy_Owner> find(const SafeShim* shim);
				inline auto_iface_ptr<Safe_Proxy_Owner> add(const SafeShim* shim, Safe_Proxy_Owner* pOwner);

			private:
				Threading::Mutex                            m_lock;
				std::map<const SafeShim*,Safe_Proxy_Owner*> m_map;
			};
			typedef Threading::Singleton<safe_proxy_holder,Threading::InitialiseDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > SAFE_PROXY_HOLDER;

			class safe_stub_holder
			{
			public:
				inline void remove(IObject* pObject);
				inline auto_iface_ptr<Safe_Stub_Owner> find(IObject* pObject);
				inline auto_iface_ptr<Safe_Stub_Owner> add(IObject* pObject, Safe_Stub_Owner* pOwner);

			private:
				Threading::Mutex                    m_lock;
				std::map<IObject*,Safe_Stub_Owner*> m_map;
			};
			typedef Threading::Singleton<safe_stub_holder,Threading::InitialiseDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > SAFE_STUB_HOLDER;

			struct qi_rtti
			{
				Safe_Proxy_Base* (*pfnCreateSafeProxy)(const SafeShim* shim, Safe_Proxy_Owner* pOwner);
				Safe_Stub_Base* (*pfnCreateSafeStub)(IObject* pI, Safe_Stub_Owner* pOwner);
				const wchar_t* pszName;
			};

			typedef Threading::Singleton<std::map<guid_t,const qi_rtti*>,Threading::ModuleDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > RTTI_HOLDER;

			inline static const qi_rtti* get_qi_rtti_info(const guid_t& iid)
			{
				try
				{
					std::map<guid_t,const qi_rtti*>* iid_map = RTTI_HOLDER::instance();
					std::map<guid_t,const qi_rtti*>::const_iterator i=iid_map->find(iid);
					if (i != iid_map->end())
						return i->second;
				}
				catch (...)
				{}
				
				return 0;
			}

			inline static void register_rtti_info(const guid_t& iid, const qi_rtti* pRtti)
			{
				try
				{
					RTTI_HOLDER::instance()->insert(std::map<guid_t,const qi_rtti*>::value_type(iid,pRtti));
				}
				catch (std::exception& e)
				{
					OMEGA_THROW(e);
				}
			}

			OMEGA_DECLARE_FORWARDS(Omega,IException)
			OMEGA_DECLARE_FORWARDS(Omega::TypeInfo,ITypeInfo)

			template <typename I>
			inline const SafeShim* create_wire_stub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, I* pI);

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

			OMEGA_QI_MAGIC(Omega,IObject)
			OMEGA_QI_MAGIC(Omega,IException)
			OMEGA_QI_MAGIC(Omega::TypeInfo,ITypeInfo)			
		}
	}
}

// ISafeProxy has no rtti associated with it...
OMEGA_SET_GUIDOF(Omega::System::MetaInfo,ISafeProxy,"{ADFB60D2-3125-4046-9EEB-0CC898E989E8}")

#endif // OOCORE_SAFE_H_INCLUDED_

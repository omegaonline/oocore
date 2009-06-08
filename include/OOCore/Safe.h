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

					void update(T& dest, const guid_t*)
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

			template <class T>
			class std_safe_type<T&>
			{
			public:
				class ref_holder
				{
				public:
					ref_holder(typename marshal_info<T>::safe_type::type* val, const guid_t* piid) :
						m_val(*val), m_dest(val), m_piid(piid)
					{}

					~ref_holder()
					{
						m_val.update(*m_dest,m_piid);
					}

					operator T&()
					{
						return m_val;
					}

				private:
					typename marshal_info<T>::safe_type::ref_type m_val;
					typename marshal_info<T>::safe_type::type* m_dest;
					const guid_t* m_piid;
				};

				class ref_holder_safe
				{
				public:
					ref_holder_safe(T& val, const guid_t* piid = 0) : m_val(val), m_dest(&val), m_piid(piid)
					{}

					~ref_holder_safe()
					{
						m_val.update(*m_dest,m_piid);
					}

					operator typename marshal_info<T>::safe_type::type*()
					{
						return &m_val;
					}

				private:
					typename marshal_info<T>::safe_type::ref_safe_type m_val;
					T* m_dest;
					const guid_t* m_piid;
				};
				typedef typename marshal_info<T>::safe_type::type* type;

				static ref_holder_safe coerce(T& val)
				{
					return ref_holder_safe(val);
				}

				static ref_holder_safe coerce(T& val, const guid_t& iid)
				{
					return ref_holder_safe(val,&iid);
				}

				static ref_holder coerce(type val, const guid_t* piid = 0)
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

				static const T& coerce(type val)
				{
					return marshal_info<T>::safe_type::coerce(*const_cast<typename marshal_info<T>::safe_type::type*>(val));
				}
			};

			struct SafeShim
			{
				const void* m_vtable;
				void* m_stub;
				const guid_t* m_iid;
			};

			struct ISafeProxy : public IObject
			{
				virtual void Pin() = 0;
				virtual void Unpin() = 0;
				virtual SafeShim* GetStub(const Omega::guid_t& iid) = 0;
			};

			class Safe_Proxy_Owner;
			
			inline IObject* create_proxy(SafeShim* shim);
			
			class Safe_Stub_Owner;

			inline SafeShim* create_stub(IObject* proxy, const guid_t& iid);

			inline void throw_correct_exception(SafeShim* except);
			inline SafeShim* return_safe_exception(IException* pE);

			struct IObject_Safe_VTable
			{
				SafeShim* (OMEGA_CALL* pfnAddRef_Safe)(SafeShim* shim);
				SafeShim* (OMEGA_CALL* pfnRelease_Safe)(SafeShim* shim);
				SafeShim* (OMEGA_CALL* pfnQueryInterface_Safe)(SafeShim* shim, SafeShim** retval, const guid_t* iid);
				SafeShim* (OMEGA_CALL* pfnPin_Safe)(SafeShim* shim);
				SafeShim* (OMEGA_CALL* pfnUnpin_Safe)(SafeShim* shim);
			};

			class auto_safe_shim
			{
			public:
				auto_safe_shim(SafeShim* pS = 0) : m_pS(pS)
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

				operator SafeShim*&()
				{
					return m_pS;
				}

				SafeShim** operator &()
				{
					return &m_pS;
				}

				SafeShim* operator ->()
				{
					return m_pS;
				}

				SafeShim* detach()
				{
					SafeShim* ret = m_pS;
					m_pS = 0;
					return ret;
				}

			private:
				SafeShim* m_pS;

				void AddRef()
				{
					if (m_pS)
					{
						SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnAddRef_Safe(m_pS);
						if (except)
							throw_correct_exception(except);
					}
				}

				void Release()
				{
					if (m_pS)
					{
						SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_pS->m_vtable)->pfnRelease_Safe(m_pS);
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

				operator SafeShim* ()
				{
					return m_pS;
				}

				SafeShim* operator -> ()
				{
					return m_pS;
				}

			protected:
				SafeShim* m_pS;

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

				void update(I*& pI, const guid_t*)
				{
					if (pI)
						pI->Release();

					pI = static_cast<I*>(create_proxy(this->m_pS));
				}

				SafeShim** operator & ()
				{
					return &this->m_pS;
				}
			};

			template <class I>
			class iface_stub_functor
			{
			public:
				iface_stub_functor(SafeShim* pS, const guid_t*) :
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
				iface_stub_functor_ref(SafeShim* pS) :
				  iface_stub_functor<I>(pS,0)
				{
				}

				void update(SafeShim*& pS, const guid_t* piid)
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
				typedef SafeShim* type;

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
				virtual SafeShim* GetShim() = 0;
				virtual void Throw() = 0;
				virtual void Throw(const guid_t& iid) = 0;
			};

			inline Safe_Proxy_Base* create_proxy_base(SafeShim* shim);

			struct Safe_Stub_Base
			{
				virtual void AddRef() = 0;
				virtual bool IsDerived(const guid_t& iid) const = 0;
				virtual SafeShim* ShimQI(const guid_t& iid) = 0;
			};

			class Safe_Proxy_Owner : public ISafeProxy
			{
			public:
				Safe_Proxy_Owner(SafeShim* shim, const guid_t& iid, Safe_Proxy_Base* pB) : 
					m_shim(shim)
				{
					m_refcount.AddRef();
					m_iid_map[iid] = pB;
				}

				virtual void AddRef()
				{
					m_refcount.AddRef();
				}

				virtual void Release()
				{
					if (m_refcount.Release())
						delete this;
				}

				inline IObject* QueryInterface(const guid_t& iid);
				inline void Throw(const guid_t& iid);

				void Pin()
				{
					SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnPin_Safe(m_shim);
					if (except)
						throw_correct_exception(except);
				}

				void Unpin()
				{
					SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnUnpin_Safe(m_shim);
					if (except)
						throw_correct_exception(except);
				}

			private:
				Threading::Mutex                  m_lock;
				std::map<guid_t,Safe_Proxy_Base*> m_iid_map;
				SafeShim*                         m_shim;
				Threading::AtomicRefCount         m_refcount;

				virtual ~Safe_Proxy_Owner()
				{
					for (std::map<guid_t,Safe_Proxy_Base*>::iterator i=m_iid_map.begin();i!=m_iid_map.end();++i)
					{
						i->second->DecRef();
					}
				}

				inline Safe_Proxy_Base* GetBase(const Omega::guid_t& iid);

				SafeShim* GetStub(const Omega::guid_t& iid)
				{
					SafeShim* shim = GetBase(iid)->GetShim();
					if (!shim)
						OMEGA_THROW(L"No shim for stub");

					SafeShim* except = static_cast<const IObject_Safe_VTable*>(shim->m_vtable)->pfnAddRef_Safe(shim);
					if (except)
						throw_correct_exception(except);

					return shim;
				}
			};

			class Safe_Stub_Owner
			{
			public:
				Safe_Stub_Owner(const guid_t* iid, SafeShim* shim)
				{
					m_iid_map[*iid] = shim;
				}

				inline SafeShim* CachedQI(Safe_Stub_Base* pStub, const guid_t& iid);
				inline void RemoveShim(SafeShim* shim);

			private:
				Threading::Mutex           m_lock;
				std::map<guid_t,SafeShim*> m_iid_map;
			};

			struct qi_rtti
			{
				Safe_Proxy_Base* (*pfnCreateSafeProxy)(SafeShim* shim, Safe_Proxy_Owner* pOwner);
				SafeShim* (*pfnCreateSafeStub)(IObject* pI, Safe_Stub_Owner* pOwner, SafeShim** ret);
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
				static Safe_Proxy_Base* bind(SafeShim* shim, Safe_Proxy_Owner* pOwner)
				{
					Safe_Proxy* pThis;
					OMEGA_NEW(pThis,Safe_Proxy(shim,pOwner));
					return pThis;
				}

			protected:
				Safe_Proxy(SafeShim* shim, Safe_Proxy_Owner* pOwner) : m_shim(shim), m_pOwner(pOwner)
				{
					SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnAddRef_Safe(m_shim);
					if (except)
						throw_correct_exception(except);

					if (!m_pOwner)
						OMEGA_NEW(m_pOwner,Safe_Proxy_Owner(m_shim,OMEGA_GUIDOF(D),this));

					m_refcount.AddRef();
				}

				virtual ~Safe_Proxy()
				{
					SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnRelease_Safe(m_shim);
					if (except)
						throw_correct_exception(except);
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

				SafeShim* m_shim;

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
					if (m_refcount.Release())
						delete this;
				}

				IObject* QIReturn()
				{
					return static_cast<D*>(this);
				}

				SafeShim* GetShim()
				{
					return m_shim;
				}

				void Throw()
				{
					throw static_cast<D*>(this);
				}

				void Throw(const guid_t& iid)
				{
					return m_pOwner->Throw(iid);
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
				static SafeShim* create(IObject* pI, Safe_Stub_Owner* pOwner, SafeShim** ret)
				{
					try
					{
						Safe_Stub* pThis;
						OMEGA_NEW(pThis,Safe_Stub(pI,&OMEGA_GUIDOF(IObject),pOwner));
						*ret = &pThis->m_shim;
						return 0;
					}
					catch (IException* pE)
					{
						return return_safe_exception(pE);
					}
				}

			protected:
				Safe_Stub(IObject* pI, const guid_t* iid, Safe_Stub_Owner* pOwner) : 
					 m_pI(pI), m_pOwner(pOwner)
				{
					m_refcount.AddRef(); 
					m_pI->AddRef();

					if (!m_pOwner)
						OMEGA_NEW(m_pOwner,Safe_Stub_Owner(iid,&m_shim));

					m_shim.m_vtable = get_vt();
					m_shim.m_stub = this;
					m_shim.m_iid = iid;
				}

				virtual ~Safe_Stub()
				{
					m_pOwner->RemoveShim(&m_shim);
					m_pI->Release();
				}
				
				static const IObject_Safe_VTable* get_vt()
				{
					static const IObject_Safe_VTable vt = 
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&Pin_Safe,
						&Unpin_Safe
					};
					return &vt;
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}
				
				virtual void Pin()
				{
					m_pincount.AddRef();
				}

				virtual void Unpin()
				{
					if (m_pincount.Release() && m_refcount.IsZero())
						delete this;
				}

				SafeShim         m_shim;
				IObject*         m_pI;
				Safe_Stub_Owner* m_pOwner;

			private:
				Safe_Stub(const Safe_Stub&) {};
				Safe_Stub& operator =(const Safe_Stub&) { return *this; };

				Threading::AtomicRefCount m_refcount;
				Threading::AtomicRefCount m_pincount;
								
				void AddRef()
				{
					m_refcount.AddRef();
				}

				void Release()
				{
					if (m_refcount.Release() && m_pincount.IsZero())
						delete this;
				}

				SafeShim* QueryInterface(const guid_t& iid)
				{
					if (IsDerived(iid))
					{
						AddRef();
						return &m_shim;
					}

					return m_pOwner->CachedQI(this,iid);
				}

				virtual SafeShim* ShimQI(const guid_t& iid)
				{
					IObject* pI = m_pI->QueryInterface(iid);
					if (!pI)
						return 0;
					auto_iface_ptr<IObject> ptrI(pI);

					// Wrap it in a shim and add it...
					const qi_rtti* rtti = get_qi_rtti_info(iid);
					if (!rtti)
						OMEGA_THROW(L"Failed to create stub for interface - missing rtti");
					
					SafeShim* shim;
					SafeShim* except = (*rtti->pfnCreateSafeStub)(pI,m_pOwner,&shim);
					if (except)
						throw_correct_exception(except);
					
					return shim;
				}

				static SafeShim* OMEGA_CALL AddRef_Safe(SafeShim* shim)
				{
					SafeShim* except = 0;
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

				static SafeShim* OMEGA_CALL Release_Safe(SafeShim* shim)
				{
					SafeShim* except = 0;
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

				static SafeShim* OMEGA_CALL QueryInterface_Safe(SafeShim* shim, SafeShim** retval, const guid_t* iid)
				{
					SafeShim* except = 0;
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

				static SafeShim* OMEGA_CALL Pin_Safe(SafeShim* shim)
				{
					SafeShim* except = 0;
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

				static SafeShim* OMEGA_CALL Unpin_Safe(SafeShim* shim)
				{
					SafeShim* except = 0;
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

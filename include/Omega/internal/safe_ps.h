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

#ifndef OOCORE_SAFE_PS_H_INCLUDED_
#define OOCORE_SAFE_PS_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		bool PinObjectPointer(IObject* pObject);
		void UnpinObjectPointer(IObject* pObject);

		namespace Internal
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
					assert(m_pI != 0);

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
					assert(m_pS != 0);

					return m_pS;
				}

			private:
				const SafeShim* m_pS;

				void AddRef()
				{
					if (m_pS)
						addref_safe(m_pS);
				}

				void Release()
				{
					if (m_pS)
						release_safe(m_pS);
				}
			};

			struct OMEGA_PRIVATE_TYPE(safe_module)
			{
				int unused;
			};

			class safe_holder
			{
			public:
				safe_holder();
				~safe_holder();

				IObject* add(const SafeShim* shim, IObject* pObject);
				const SafeShim* add(IObject* pObject, const SafeShim* shim);

				const SafeShim* find(IObject* pObject);

				void remove(IObject* pObject);
				void remove(const SafeShim* shim);

			private:
				void* m_handle;
			};
			typedef Threading::Singleton<safe_holder,Threading::InitialiseDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > SAFE_HOLDER;

			class Safe_Proxy_Base : public System::Internal::ThrowingNew
			{
			protected:
				Safe_Proxy_Base(const SafeShim* shim) :
						m_shim(shim)
				{
					m_internal.m_pProxy = this;
					AddRef();
				}

				virtual ~Safe_Proxy_Base()
				{ }

				const SafeShim* m_shim;

				virtual bool IsDerived__proxy__(const guid_t& iid) const = 0;
				virtual IObject* QIReturn__proxy__() = 0;
				virtual void Destruct__proxy__() = 0;

				virtual void AddRef()
				{
					m_refcount.AddRef();
					addref_safe(m_shim);
				}

				virtual void Release()
				{
					assert(!m_refcount.IsZero());

					if (m_refcount.Release())
					{
						m_intcount.AddRef();

						// This can result in a circular dependancy
						release_safe(m_shim);

						if (m_intcount.Release() && m_pincount.IsZero())
							Destruct__proxy__();
					}
				}

				virtual IObject* QueryInterface(const guid_t& iid);

			private:
				Safe_Proxy_Base(const Safe_Proxy_Base&);
				Safe_Proxy_Base& operator =(const Safe_Proxy_Base&);

				struct Internal : public ISafeProxy
				{
					void AddRef()
					{
						m_pProxy->Internal_AddRef();
					}

					void Release()
					{
						m_pProxy->Internal_Release();
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						if (iid == OMEGA_GUIDOF(IObject))
						{
							AddRef();
							return this;
						}

						return m_pProxy->QueryInterface(iid);
					}

					void Pin()
					{
						m_pProxy->Pin();
					}

					void Unpin()
					{
						m_pProxy->Unpin();
					}

					const SafeShim* GetShim(const Omega::guid_t& iid)
					{
						return m_pProxy->GetShim(iid);
					}

					const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid)
					{
						return m_pProxy->CreateWireStub(shim_Controller,shim_Marshaller,iid);
					}

					Safe_Proxy_Base* m_pProxy;
				};
				friend struct Internal;
				Internal m_internal;

				Threading::AtomicRefCount m_refcount;
				Threading::AtomicRefCount m_intcount;
				Threading::AtomicRefCount m_pincount;

				virtual void Internal_AddRef()
				{
					m_intcount.AddRef();
				}

				virtual void Internal_Release()
				{
					assert(!m_intcount.IsZero());

					if (m_intcount.Release() && m_refcount.IsZero() && m_pincount.IsZero())
						Destruct__proxy__();
				}

				void Pin()
				{
					m_pincount.AddRef();
					
					// Pin the shim
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnPin_Safe(m_shim);
					if (except)
						throw_correct_exception(except);
				}

				void Unpin()
				{
					assert(!m_pincount.IsZero());

					if (m_pincount.Release())
					{
						// Unpin the shim
						const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnUnpin_Safe(m_shim);
						if (except)
							throw_correct_exception(except);

						if (m_intcount.IsZero() && m_refcount.IsZero())
							Destruct__proxy__();
					}
				}

				const SafeShim* GetShim(const Omega::guid_t& iid)
				{
					if (IsDerived__proxy__(iid) || guid_t(*m_shim->m_iid) == iid)
					{
						addref_safe(m_shim);
						return m_shim;
					}

					return 0;
				}

				const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const Omega::guid_t& iid)
				{
					if (!static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnCreateWireStub_Safe)
						return 0;

					const SafeShim* ret = 0;
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnCreateWireStub_Safe(m_shim,shim_Controller,shim_Marshaller,&iid,&ret);
					if (except)
						throw_correct_exception(except);

					return ret;
				}
			};

			template <typename I, typename D>
			class Safe_Proxy;

			template <typename D>
			class Safe_Proxy<IObject,D> : public D, public Safe_Proxy_Base
			{
			public:
				static IObject* bind(const SafeShim* shim)
				{
					Safe_Proxy* pThis = new Safe_Proxy(shim);
					return pThis->QIReturn__proxy__();
				}

			protected:
				Safe_Proxy(const SafeShim* shim) :
						Safe_Proxy_Base(shim)
				{ }

				virtual bool IsDerived__proxy__(const guid_t& /*iid*/) const
				{
					// Don't return on OMEGA_GUIDOF(IObject)
					return false;
				}

				IObject* QIReturn__proxy__()
				{
					return static_cast<D*>(this);
				}

				void Destruct__proxy__()
				{
					delete this;
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
					return Safe_Proxy_Base::QueryInterface(iid);
				}
			};

			class Safe_Proxy_IObject : public Safe_Proxy<IObject,IObject>
			{
			public:
				static IObject* bind(const SafeShim* shim)
				{
					Safe_Proxy_IObject* pThis = new Safe_Proxy_IObject(shim);

					// Add to the map...
					IObject* pExisting = SAFE_HOLDER::instance()->add(shim,pThis);
					if (pExisting)
					{
						pThis->Release();
						return pExisting;
					}

					return static_cast<IObject*>(pThis);
				}

			private:
				Safe_Proxy_IObject(const SafeShim* shim) :
						Safe_Proxy<IObject,IObject>(shim)
				{ }

				virtual ~Safe_Proxy_IObject()
				{
					SAFE_HOLDER::instance()->remove(this);
				}

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					// We are derived from IObject
					return (iid == OMEGA_GUIDOF(IObject));
				}
			};

			class Safe_Stub_Base : public System::Internal::ThrowingNew
			{
			public:
				void AddRef()
				{
					m_refcount.AddRef();
					m_pI->AddRef();
				}

				void Release()
				{
					assert(!m_refcount.IsZero());

					if (m_refcount.Release())
					{
						m_pincount.AddRef();

						// This can result in a circular dependancy
						m_pI->Release();

						if (m_pincount.Release())
							delete this;
					}
				}

				virtual bool IsDerived(const guid_t& iid) const = 0;

			protected:
				IObject* m_pI;
				SafeShim m_shim;

				Safe_Stub_Base(IObject* pI) :
						m_pI(pI)
				{
					AddRef();
				}

				virtual ~Safe_Stub_Base()
				{ }

				void Pin()
				{
					m_pincount.AddRef();
				}

				void Unpin()
				{
					assert(!m_pincount.IsZero());

					if (m_pincount.Release() && m_refcount.IsZero())
						delete this;
				}

				const SafeShim* QueryInterface(const guid_t& iid)
				{
					if (IsDerived(iid))
					{
						AddRef();
						return &m_shim;
					}

					// QI underlying interface
					auto_iface_ptr<IObject> ptr = m_pI->QueryInterface(iid);
					if (!ptr)
						return 0;

					return create_safe_stub(ptr,iid);
				}

				const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid);

			private:
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
				static const SafeShim* create(IObject* pI)
				{
					Safe_Stub* pThis = new Safe_Stub(pI,OMEGA_GUIDOF(IObject));
					return &pThis->m_shim;
				}

			protected:
				Safe_Stub(IObject* pI, const guid_t& iid) :
						Safe_Stub_Base(pI)
				{
					m_shim.m_vtable = get_vt();
					m_shim.m_stub = this;
					m_shim.m_iid = &iid;
				}

				static const IObject_Safe_VTable* get_vt()
				{
					static const IObject_Safe_VTable vt =
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&Pin_Safe,
						&Unpin_Safe,
						&CreateWireStub_Safe,
						0
					};
					return &vt;
				}

				virtual bool IsDerived(const guid_t& /*iid*/) const
				{
					// Do not return (iid == OMEGA_GUIDOF(IObject))
					return false;
				}

			private:
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

				static const SafeShim* OMEGA_CALL QueryInterface_Safe(const SafeShim* shim, const SafeShim** retval, const guid_base_t* iid)
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

				static const SafeShim* OMEGA_CALL CreateWireStub_Safe(const SafeShim* shim, const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_base_t* piid, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub*>(shim->m_stub)->CreateWireStub(shim_Controller,shim_Marshaller,*piid);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
			};

			class Safe_Stub_IObject : public Safe_Stub<IObject>
			{
			public:
				static const SafeShim* create(IObject* pI)
				{
					Safe_Stub_IObject* pThis = new Safe_Stub_IObject(pI,OMEGA_GUIDOF(IObject));

					// Add to the map...
					const SafeShim* pExisting = SAFE_HOLDER::instance()->add(pI,&pThis->m_shim);
					if (pExisting)
					{
						pThis->Release();
						return pExisting;
					}

					return &pThis->m_shim;
				}

			private:
				Safe_Stub_IObject(IObject* pI, const guid_t& iid) :
						Safe_Stub<IObject>(pI,iid)
				{
				}

				virtual ~Safe_Stub_IObject()
				{
					SAFE_HOLDER::instance()->remove(&m_shim);
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					// We are derived from IObject
					return (iid == OMEGA_GUIDOF(IObject));
				}
			};

			struct qi_rtti
			{
				IObject* (*pfnCreateSafeProxy)(const SafeShim* shim);
				const SafeShim* (*pfnCreateSafeStub)(IObject* pI);
			};

			typedef Threading::Singleton<std::map<guid_t,const qi_rtti*>,Threading::ModuleDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > RTTI_HOLDER;

			static const qi_rtti* get_qi_rtti_info(const guid_t& iid)
			{
				try
				{
					std::map<guid_t,const qi_rtti*>* iid_map = RTTI_HOLDER::instance();
					std::map<guid_t,const qi_rtti*>::const_iterator i=iid_map->find(iid);
					if (i != iid_map->end())
						return i->second;
				}
				catch (std::exception&)
				{}
				
				return 0;
			}

			static void register_rtti_info(const guid_t& iid, const qi_rtti* pRtti)
			{
				RTTI_HOLDER::instance()->insert(std::map<guid_t,const qi_rtti*>::value_type(iid,pRtti));
			}

			template <typename I> struct vtable_info;

			template <> struct vtable_info<IObject>
			{
				typedef IObject_Safe_VTable type;
			};

			OMEGA_DECLARE_FORWARDS(Omega,IException)

			OMEGA_DEFINE_INTERNAL_INTERFACE_NOPROXY
			(
				Omega, IException,

				OMEGA_METHOD_VOID(Rethrow,0,())
				OMEGA_METHOD(guid_t,GetThrownIID,0,())
				OMEGA_METHOD(IException*,GetCause,0,())
				OMEGA_METHOD(string_t,GetDescription,0,())
			)

			template <typename D>
			class Safe_Proxy<IException,D> : public Safe_Proxy<IObject,D>
			{
				const vtable_info<IException>::type* deref_vt()
				{
					return static_cast<const vtable_info<IException>::type*>(this->m_shim->m_vtable);
				}

			public:
				static IObject* bind(const SafeShim* shim)
				{
					Safe_Proxy* pThis = new Safe_Proxy(shim);
					return pThis->QIReturn__proxy__();
				}

			protected:
				Safe_Proxy(const SafeShim* shim) :
						Safe_Proxy<IObject,D>(shim)
				{}

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					if (iid == OMEGA_GUIDOF(IException))
						return true;

					return Safe_Proxy<IObject,D>::IsDerived__proxy__(iid);
				}

			public:
				void Rethrow()
				{
					guid_t iid = GetThrownIID();
					if (IsDerived__proxy__(iid) || !get_qi_rtti_info(iid))
						throw static_cast<D*>(this);

					// QI m_shim
					auto_safe_shim retval;
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(this->m_shim->m_vtable)->pfnQueryInterface_Safe(this->m_shim,&retval,&iid);
					if (except)
					{
						this->Release();
						throw_correct_exception(except);
					}

					if (!retval)
						throw static_cast<D*>(this);

					this->Release();
					static_cast<IException*>(create_safe_proxy(retval,OMEGA_GUIDOF(IException)))->Rethrow();
				}

				OMEGA_DECLARE_SAFE_PROXY_METHODS
				(
					OMEGA_METHOD(guid_t,GetThrownIID,0,())
					OMEGA_METHOD(IException*,GetCause,0,())
					OMEGA_METHOD(string_t,GetDescription,0,())
				)
			};

			OMEGA_QI_MAGIC(Omega,IObject)
			OMEGA_QI_MAGIC(Omega,IException)
		}
	}
}

// ISafeProxy has no rtti associated with it...
OMEGA_SET_GUIDOF(Omega::System::Internal,ISafeProxy,"{ADFB60D2-3125-4046-9EEB-0CC898E989E8}")

#endif // OOCORE_SAFE_PS_H_INCLUDED_

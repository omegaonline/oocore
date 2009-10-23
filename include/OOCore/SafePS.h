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

			class Safe_Proxy_Owner;
			
			class Safe_Proxy_Base
			{
			public:
				virtual bool IsDerived__proxy__(const guid_t& iid) const = 0;
				virtual IObject* QIReturn__proxy__() = 0;
				virtual void Throw__proxy__() = 0;

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

						if (m_intcount.IsZero() && m_pincount.IsZero())
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

					if (m_pincount.Release() && m_intcount.IsZero() && m_refcount.IsZero())
						delete this;
				}

				inline const SafeShim* GetShim(const guid_t& iid);
				inline const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid);
				
				const SafeShim* GetShim()
				{
					const SafeShim* except = static_cast<const IObject_Safe_VTable*>(m_shim->m_vtable)->pfnAddRef_Safe(m_shim);
					if (except)
						throw_correct_exception(except);
				
					return m_shim;
				}

				void Internal_AddRef()
				{
					m_intcount.AddRef();
				}

				void Internal_Release()
				{
					assert(m_intcount.m_debug_value > 0);

					if (m_intcount.Release() && m_refcount.IsZero() && m_pincount.IsZero())
						delete this;
				}

			protected:
				Safe_Proxy_Base(const SafeShim* shim, Safe_Proxy_Owner* pOwner) : m_shim(shim), m_pOwner(pOwner)
				{
					m_internal.m_pOuter = this;
					AddRef();
				}

				inline virtual ~Safe_Proxy_Base();

				inline void Throw(const guid_t& iid);

				const SafeShim* m_shim;
				
			private:
				Safe_Proxy_Base(const Safe_Proxy_Base&);
				Safe_Proxy_Base& operator =(const Safe_Proxy_Base&);	

				Safe_Proxy_Owner*         m_pOwner;
				Threading::AtomicRefCount m_refcount;
				Threading::AtomicRefCount m_intcount;
				Threading::AtomicRefCount m_pincount;

				struct Internal : public ISafeProxy
				{
					void AddRef() 
					{
						m_pOuter->Internal_AddRef(); 
					}

					void Release() 
					{
						m_pOuter->Internal_Release(); 
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

					const SafeShim* GetShim(const guid_t& iid)
					{
						return m_pOuter->GetShim(iid);
					}

					const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid)
					{
						return m_pOuter->CreateWireStub(shim_Controller,shim_Marshaller,iid);
					}

					IProxy* GetWireProxy()
					{
						return 0;
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

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

			private:
				IObject* QIReturn__proxy__()
				{
					AddRef();
					return static_cast<D*>(this);
				}

				void Throw__proxy__()
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
					if (iid != OMEGA_GUIDOF(IObject) && IsDerived__proxy__(iid))
						return QIReturn__proxy__();
					
					return Safe_Proxy_Base::QueryInterface(iid);
				}
			};

			class Safe_Stub_Owner;

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
				SafeShim m_shim;
				IObject* m_pI;
				
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
					OMEGA_NEW(pThis,Safe_Stub(pI,OMEGA_GUIDOF(IObject),pOwner));
					return pThis;					
				}

			protected:
				Safe_Stub(IObject* pI, const guid_t& iid, Safe_Stub_Owner* pOwner) : 
					 Safe_Stub_Base(pI,pOwner)
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
						&GetBaseShim_Safe,
						0,
						0
					};
					return &vt;
				}

				virtual bool IsDerived(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
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
				inline void Throw(const guid_t& iid);
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

					const SafeShim* GetShim(const guid_t& iid)
					{
						return m_pOwner->GetShim(iid);
					}

					const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid)
					{
						return m_pOwner->CreateWireStub(shim_Controller,shim_Marshaller,iid);
					}

					IProxy* GetWireProxy()
					{
						return 0;
					}

					Safe_Proxy_Owner* m_pOwner;
				};
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

			inline System::MetaInfo::auto_iface_ptr<System::MetaInfo::Safe_Proxy_Owner> create_safe_proxy_owner(const SafeShim* shim, IObject* pOuter);
			
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
						&Pin_Safe,
						&Unpin_Safe,
						&GetBaseShim_Safe,
						&CreateWireStub_Safe,
						0
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
				inline const SafeShim* CreateWireStub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid);

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

				static const SafeShim* OMEGA_CALL QueryInterface_Safe(const SafeShim* shim, const SafeShim** retval, const guid_base_t* iid)
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

				static const SafeShim* OMEGA_CALL CreateWireStub_Safe(const SafeShim* shim, const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_base_t* piid, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Safe_Stub_Owner*>(shim->m_stub)->CreateWireStub(shim_Controller,shim_Marshaller,*piid);
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

			template <typename I> struct vtable_info;

			template <> struct vtable_info<IObject>
			{
				typedef IObject_Safe_VTable type;
			};

			OMEGA_DECLARE_FORWARDS(Omega,IException)
			OMEGA_DECLARE_FORWARDS(Omega::TypeInfo,ITypeInfo)

			OMEGA_DEFINE_INTERNAL_INTERFACE_NOPROXY
			(
				Omega, IException,

				OMEGA_METHOD_VOID(Throw,0,())
				OMEGA_METHOD(guid_t,GetThrownIID,0,())
				OMEGA_METHOD(IException*,GetCause,0,())
				OMEGA_METHOD(string_t,GetDescription,0,())
				OMEGA_METHOD(string_t,GetSource,0,())
			)

			template <typename D>
			class Safe_Proxy<IException,D> : public Safe_Proxy<IObject,D>
			{
				const vtable_info<IException>::type* deref_vt() 
				{ 
					return static_cast<const vtable_info<IException>::type*>(this->m_shim->m_vtable); 
				}

			public:
				static Safe_Proxy_Base* bind(const SafeShim* shim, Safe_Proxy_Owner* pOwner)
				{
					Safe_Proxy* pThis; 
					OMEGA_NEW(pThis,Safe_Proxy(shim,pOwner)); 
					return pThis;
				}

			protected:
				Safe_Proxy(const SafeShim* shim, Safe_Proxy_Owner* pOwner = 0) : 
					 Safe_Proxy<IObject,D>(shim,pOwner) 
				{}

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					if (iid == OMEGA_GUIDOF(IException)) 
						return true;
					return Safe_Proxy<IObject,D>::IsDerived__proxy__(iid);
				}

			public:
				void Throw()
				{
					// Make sure we release our refcount before throwing the correct interface
					auto_iface_ptr<Safe_Proxy<IException,D> > ptrThis(this);

					Safe_Proxy_Base::Throw(GetThrownIID());
				}

				OMEGA_DECLARE_SAFE_PROXY_METHODS
				(
					OMEGA_METHOD(guid_t,GetThrownIID,0,())
					OMEGA_METHOD(IException*,GetCause,0,())
					OMEGA_METHOD(string_t,GetDescription,0,())
					OMEGA_METHOD(string_t,GetSource,0,())
				)
			};

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

#endif // OOCORE_SAFE_PS_H_INCLUDED_

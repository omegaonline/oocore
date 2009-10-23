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

#ifndef OOCORE_WIRE_PS_H_INCLUDED_
#define OOCORE_WIRE_PS_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		namespace MetaInfo
		{
			class Wire_Proxy_Owner;

			class Wire_Proxy_Base
			{
			public:
				virtual bool IsDerived__proxy__(const guid_t& iid) const = 0;
				virtual IObject* QIReturn__proxy__() = 0;
				virtual void Throw__proxy__() = 0;

				void AddRef()
				{
					m_refcount.AddRef();
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release() && m_pincount.IsZero())
						delete this;
				}

				inline IObject* QueryInterface(const guid_t& iid);

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

				inline const SafeShim* GetShim(const guid_t& iid);

				const SafeShim* GetShim()
				{
					AddRef();
					return &m_shim;
				}

			protected:
				SafeShim m_shim;

				Wire_Proxy_Base(Wire_Proxy_Owner* pOwner) : m_pOwner(pOwner)
				{
					m_internal.m_pOuter = this;
					AddRef();
				}

				inline virtual ~Wire_Proxy_Base();

				inline void Throw(const guid_t& iid);
				inline const SafeShim* GetBaseShim();
				inline const SafeShim* GetWireProxy();
				inline auto_iface_ptr<IMarshaller> GetMarshaller();
				inline auto_iface_ptr<Remoting::IMessage> CreateMessage(IMarshaller* pMarshaller, const guid_t& iid, uint32_t method_id);
				inline void UnpackHeader(Remoting::IMessage* pMessage);

			private:
				Wire_Proxy_Base(const Wire_Proxy_Base&);
				Wire_Proxy_Base& operator =(const Wire_Proxy_Base&);

				Wire_Proxy_Owner*         m_pOwner;
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

					const SafeShim* GetShim(const guid_t& iid)
					{
						return m_pOuter->GetShim(iid);
					}

					const SafeShim* CreateWireStub(const SafeShim*, const SafeShim*, const guid_t&)
					{
						return 0;
					}

					IProxy* GetWireProxy()
					{
						auto_safe_shim ss = m_pOuter->GetWireProxy();
						return static_cast<IProxy*>(create_safe_proxy(ss));
					}

					Wire_Proxy_Base* m_pOuter;
				};
				Internal m_internal;
			};

			template <typename I, typename D>
			class Wire_Proxy;

			template <typename D>
			class Wire_Proxy<IObject,D> : public Wire_Proxy_Base, public D
			{
			public:
				static Wire_Proxy_Base* bind(Wire_Proxy_Owner* pOwner)
				{
					Wire_Proxy* pThis;
					OMEGA_NEW(pThis,Wire_Proxy(pOwner));
					return pThis;
				}

			protected:
				Wire_Proxy(Wire_Proxy_Owner* pOwner) :
					 Wire_Proxy_Base(pOwner)
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
						&GetWireProxy_Safe
					};
					m_shim.m_vtable = &vt;
					m_shim.m_stub = this;
					m_shim.m_iid = &OMEGA_GUIDOF(D);
				}

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

				auto_iface_ptr<Remoting::IMessage> CreateMessage(IMarshaller* pMarshaller, uint32_t method_id)
				{
					return Wire_Proxy_Base::CreateMessage(pMarshaller,OMEGA_GUIDOF(D),method_id);
				}

				static const uint32_t MethodCount = 3;

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

				static const SafeShim* OMEGA_CALL AddRef_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Wire_Proxy*>(shim->m_stub)->AddRef();
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
						static_cast<Wire_Proxy*>(shim->m_stub)->Release();
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
						if (guid_t(*iid) == OMEGA_GUIDOF(IObject))
							*retval = static_cast<Wire_Proxy*>(shim->m_stub)->GetShim();
						else
							*retval = 0;
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
						static_cast<Wire_Proxy*>(shim->m_stub)->Pin();
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
						static_cast<Wire_Proxy*>(shim->m_stub)->Unpin();
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
						*retval = static_cast<Wire_Proxy*>(shim->m_stub)->GetBaseShim();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL GetWireProxy_Safe(const SafeShim* shim, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Wire_Proxy*>(shim->m_stub)->GetWireProxy();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

			// IObject members
			public:
				virtual void AddRef()
				{
					Wire_Proxy_Base::AddRef();
				}

				virtual void Release()
				{
					Wire_Proxy_Base::Release();
				}

				virtual IObject* QueryInterface(const guid_t& iid)
				{
					if (iid != OMEGA_GUIDOF(IObject) && IsDerived__proxy__(iid))
						return QIReturn__proxy__();

					return Wire_Proxy_Base::QueryInterface(iid);
				}
			};

			template <typename D> 
			class Wire_Proxy<IException,D> : public Wire_Proxy<IObject,D> 
			{ 
				typedef Wire_Proxy<IObject,D> Base;

			public: 
				static Wire_Proxy_Base* bind(Wire_Proxy_Owner* pOwner) 
				{ 
					Wire_Proxy* pThis; 
					OMEGA_NEW(pThis,Wire_Proxy(pOwner));
					return pThis; 
				} 
			
			protected: 
				Wire_Proxy(Wire_Proxy_Owner* pOwner = 0) : Base(pOwner) {} 
				
				virtual bool IsDerived__proxy__(const guid_t& iid) const 
				{ 
					if (iid == System::MetaInfo::uid_traits<IException>::GetUID()) 
						return true; 
					return Base::IsDerived__proxy__(iid); 
				} 
				
				static const uint32_t MethodCount = Base::MethodCount + 5; 
			
			private: 
				void Throw( ) 
				{ 
					// Make sure we release our refcount before throwing the correct interface
					auto_iface_ptr<Wire_Proxy<IException,D> > ptrThis(this);

					Wire_Proxy_Base::Throw(GetThrownIID());
				} 

				OMEGA_DECLARE_WIRE_PROXY_METHODS
				(
					OMEGA_METHOD_VOID(Throw__dummy__,0,())
					OMEGA_METHOD(guid_t,GetThrownIID,0,())
					OMEGA_METHOD(IException*,GetCause,0,())
					OMEGA_METHOD(string_t,GetDescription,0,())
					OMEGA_METHOD(string_t,GetSource,0,())
				)
			};			
				
			class Wire_Proxy_Owner
			{
			public:
				Wire_Proxy_Owner(const SafeShim* proxy_shim, IObject* pOuter) : m_pOuter(pOuter)
				{
					// Wrap the proxy shim in a safe proxy
					m_ptrProxy = static_cast<IProxy*>(create_safe_proxy(proxy_shim));
					assert(m_ptrProxy);

					m_ptrMarshaller = m_ptrProxy->GetMarshaller();

					m_internal.m_pOwner = this;
					m_safe_proxy.m_pOwner = this;

					static const IObject_Safe_VTable vt =
					{
						&AddRef_Safe,
						&Release_Safe,
						&QueryInterface_Safe,
						&Pin_Safe,
						&Unpin_Safe,
						&GetBaseShim_Safe,
						0,
						&GetWireProxy_Safe
					};
					m_base_shim.m_vtable = &vt;
					m_base_shim.m_stub = this;
					m_base_shim.m_iid = &OMEGA_GUIDOF(IObject);

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

				const SafeShim* GetBaseShim()
				{
					AddRef();
					return &m_base_shim;
				}

				auto_iface_ptr<IMarshaller> GetMarshaller()
				{
					return m_ptrMarshaller;
				}

				inline void Throw(const guid_t& iid);
				inline void RemoveBase(Wire_Proxy_Base* pProxy);
				inline IObject* QueryInterface(const guid_t& iid);
				inline const SafeShim* GetShim(const guid_t& iid);
				inline IObject* CreateProxy(const guid_t& iid);
				inline const SafeShim* GetWireProxy();
				inline auto_iface_ptr<Remoting::IMessage> CreateMessage(IMarshaller* pMarshaller, const guid_t& iid, uint32_t method_id);
				inline void UnpackHeader(Remoting::IMessage* pMessage);

			private:
				Wire_Proxy_Owner(const Wire_Proxy_Owner&);
				Wire_Proxy_Owner& operator =(const Wire_Proxy_Owner&);

				Threading::Mutex                  m_lock;
				std::map<guid_t,Wire_Proxy_Base*> m_iid_map;
				auto_iface_ptr<IProxy>            m_ptrProxy;
				auto_iface_ptr<IMarshaller>       m_ptrMarshaller;
				SafeShim                          m_base_shim;
				IObject*                          m_pOuter;
				Threading::AtomicRefCount         m_refcount;
				Threading::AtomicRefCount         m_pincount;

				struct Internal : public IObject
				{
					void AddRef()
					{
						if (m_refcount.AddRef())
							m_pOwner->AddRef();
					}

					void Release()
					{
						assert(m_refcount.m_debug_value > 0);

						if (m_refcount.Release())
							m_pOwner->Release();
					}

					IObject* QueryInterface(const guid_t& iid)
					{
						return m_pOwner->QueryInterface(iid);
					}

					Wire_Proxy_Owner* m_pOwner;

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

					const SafeShim* CreateWireStub(const SafeShim*, const SafeShim*, const guid_t&)
					{
						return 0;
					}

					IProxy* GetWireProxy()
					{
						return static_cast<IProxy*>(create_safe_proxy(m_pOwner->GetWireProxy()));
					}

					Wire_Proxy_Owner* m_pOwner;
				};
				SafeProxy m_safe_proxy;

				inline virtual ~Wire_Proxy_Owner();

				inline auto_iface_ptr<Wire_Proxy_Base> GetProxyBase(const guid_t& iid, bool bAllowPartial, bool bQI);

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

				static const SafeShim* OMEGA_CALL AddRef_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Wire_Proxy_Owner*>(shim->m_stub)->AddRef();
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
						static_cast<Wire_Proxy_Owner*>(shim->m_stub)->Release();
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
						if (guid_t(*iid) == OMEGA_GUIDOF(IObject))
							*retval = static_cast<Wire_Proxy_Owner*>(shim->m_stub)->GetBaseShim();
						else
							*retval = 0;
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
						static_cast<Wire_Proxy_Owner*>(shim->m_stub)->Pin();
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
						static_cast<Wire_Proxy_Owner*>(shim->m_stub)->Unpin();
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
						*retval = static_cast<Wire_Proxy_Owner*>(shim->m_stub)->GetBaseShim();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL GetWireProxy_Safe(const SafeShim* shim, const SafeShim** retval)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Wire_Proxy_Owner*>(shim->m_stub)->GetWireProxy();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
			};

			inline System::MetaInfo::auto_iface_ptr<System::MetaInfo::Wire_Proxy_Owner> create_wire_proxy_owner(const SafeShim* shim, IObject* pOuter);

			class Wire_Stub_Base
			{
			public:
				template <typename I>
				I* get_iface()
				{
					assert(SupportsInterface(OMEGA_GUIDOF(I)));
					return static_cast<I*>(static_cast<IObject*>(m_ptrI));
				}

				auto_iface_ptr<IMarshaller> GetMarshaller()
				{
					return m_ptrMarshaller;
				}

			protected:
				Wire_Stub_Base(IStubController* pController, IMarshaller* pMarshaller, IObject* pI) :
					m_ptrMarshaller(pMarshaller), m_ptrI(pI), m_pController(pController)
				{
					PinObjectPointer(m_pController);
					m_ptrMarshaller->AddRef();
					m_ptrI->AddRef();

					m_shim.m_vtable = get_vt();
					m_shim.m_stub = this;
					m_shim.m_iid = &OMEGA_GUIDOF(IStub);
				}

				virtual ~Wire_Stub_Base()
				{
					UnpinObjectPointer(m_pController);
				}

				virtual bool_t SupportsInterface(const guid_t& iid)
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

				typedef void (*MethodTableEntry)(Wire_Stub_Base* pThis, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut);

				virtual void Invoke(uint32_t method_id, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					static const MethodTableEntry MethodTable[] =
					{
						&Release_Wire,
						&QueryInterface_Wire,
						&MarshalStub_Wire
					};

					if (method_id < MethodCount)
						return MethodTable[method_id](this,pParamsIn,pParamsOut);

					OMEGA_THROW(L"Invoke called with invalid method index");
				}

				const SafeShim* GetShim()
				{
					AddRef();
					return &m_shim;
				}

				static const uint32_t MethodCount = 3;	// This must match the proxy

			private:
				auto_iface_ptr<IMarshaller> m_ptrMarshaller;
				auto_iface_ptr<IObject>     m_ptrI;
				IStubController*            m_pController;
				Threading::AtomicRefCount   m_refcount;
				Threading::AtomicRefCount   m_pincount;
				SafeShim                    m_shim;

				Wire_Stub_Base(const Wire_Stub_Base&);
				Wire_Stub_Base& operator = (const Wire_Stub_Base&);

				static const vtable_info<IStub>::type* get_vt()
				{
					static const vtable_info<IStub>::type vt =
					{
						{
							&AddRef_Safe,
							&Release_Safe,
							&QueryInterface_Safe,
							&Pin_Safe,
							&Unpin_Safe,
							&GetBaseShim_Safe,
							0,
							0
						},
						&Invoke_Safe,
						&SupportsInterface_Safe,
					};
					return &vt;
				}

				void AddRef()
				{
					m_refcount.AddRef();
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release() && m_pincount.IsZero())
						delete this;
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

				const SafeShim* GetBaseShim()
				{
					return GetShim();
				}

				void Invoke_Internal(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					// Read the method id
					Invoke(pParamsIn->ReadUInt32(L"$method_id"),pParamsIn,pParamsOut);
				}

				static const SafeShim* OMEGA_CALL AddRef_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Wire_Stub_Base*>(shim->m_stub)->AddRef();
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
						static_cast<Wire_Stub_Base*>(shim->m_stub)->Release();
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
						if (guid_t(*iid) == OMEGA_GUIDOF(IObject) ||
							guid_t(*iid) == OMEGA_GUIDOF(IStub))
						{
							static_cast<Wire_Stub_Base*>(shim->m_stub)->AddRef();
							*retval = shim;
						}
						else
							*retval = 0;
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
						static_cast<Wire_Stub_Base*>(shim->m_stub)->Pin();
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
						static_cast<Wire_Stub_Base*>(shim->m_stub)->Unpin();
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
						*retval = static_cast<Wire_Stub_Base*>(shim->m_stub)->GetBaseShim();
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL Invoke_Safe(const SafeShim* shim, const SafeShim* shim_ParamsIn, const SafeShim* shim_ParamsOut)
				{
					const SafeShim* except = 0;
					try
					{
						auto_iface_ptr<Remoting::IMessage> ptrParamsIn = static_cast<Remoting::IMessage*>(create_safe_proxy(shim_ParamsIn));
						auto_iface_ptr<Remoting::IMessage> ptrParamsOut = static_cast<Remoting::IMessage*>(create_safe_proxy(shim_ParamsOut));

						static_cast<Wire_Stub_Base*>(shim->m_stub)->Invoke_Internal(ptrParamsIn,ptrParamsOut);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static const SafeShim* OMEGA_CALL SupportsInterface_Safe(const SafeShim* shim, int* retval, const guid_base_t* piid)
				{
					const SafeShim* except = 0;
					try
					{
						*retval = static_cast<Wire_Stub_Base*>(shim->m_stub)->SupportsInterface(*piid);
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}

				static void Release_Wire(Wire_Stub_Base* pThis, Remoting::IMessage* pParamsIn, Remoting::IMessage*)
				{
					pThis->m_pController->RemoteRelease(pParamsIn->ReadUInt32(L"release_count"));
				}

				static void QueryInterface_Wire(Wire_Stub_Base* pThis, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					pParamsOut->WriteBoolean(L"bQI",pThis->m_pController->RemoteQueryInterface(pParamsIn->ReadGuid(L"iid")));
				}

				static void MarshalStub_Wire(Wire_Stub_Base* pThis, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					pThis->m_pController->MarshalStub(pParamsIn,pParamsOut);
				}
			};

			template <typename I>
			class Wire_Stub;

			template <>
			class Wire_Stub<IObject> : public Wire_Stub_Base
			{
			public:
				static const SafeShim* create(IStubController* pController, IMarshaller* pMarshaller, IObject* pI)
				{
					Wire_Stub* pThis;
					OMEGA_NEW(pThis,Wire_Stub(pController,pMarshaller,pI));
					return pThis->GetShim();
				}

			protected:
				Wire_Stub(IStubController* pController, IMarshaller* pMarshaller, IObject* pI) :
					Wire_Stub_Base(pController,pMarshaller,pI)
				{
				}

				virtual void Invoke(uint32_t method_id, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					return Wire_Stub_Base::Invoke(method_id,pParamsIn,pParamsOut);
				}

				static const uint32_t MethodCount = Wire_Stub_Base::MethodCount;
			};

			inline const SafeShim* create_wire_stub(const SafeShim* shim_Controller, const SafeShim* shim_Marshaller, const guid_t& iid, IObject* pObj);

			class wire_proxy_holder
			{
			public:
				inline void remove(const SafeShim* shim);
				inline auto_iface_ptr<Wire_Proxy_Owner> find(const SafeShim* shim);
				inline auto_iface_ptr<Wire_Proxy_Owner> add(const SafeShim* shim, Wire_Proxy_Owner* pOwner);

			private:
				Threading::Mutex                            m_lock;
				std::map<const SafeShim*,Wire_Proxy_Owner*> m_map;
			};
			typedef Threading::Singleton<wire_proxy_holder,Threading::InitialiseDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > WIRE_PROXY_HOLDER;

			struct wire_rtti
			{
				Wire_Proxy_Base* (*pfnCreateWireProxy)(Wire_Proxy_Owner* pOwner);
				const SafeShim* (*pfnCreateWireStub)(IStubController* pController, IMarshaller* pMarshaller, IObject* pI);
			};

			typedef Threading::Singleton<std::map<guid_t,const wire_rtti*>,Threading::ModuleDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > WIRE_HOLDER;

			inline static const wire_rtti* get_wire_rtti_info(const guid_t& iid)
			{
				try
				{
					std::map<guid_t,const wire_rtti*>* iid_map = WIRE_HOLDER::instance();
					std::map<guid_t,const wire_rtti*>::const_iterator i=iid_map->find(iid);
					if (i != iid_map->end())
						return i->second;
				}
				catch (...)
				{}

				return 0;
			}

			inline static void register_wire_rtti_info(const guid_t& iid, const wire_rtti* pRtti)
			{
				try
				{
					WIRE_HOLDER::instance()->insert(std::map<guid_t,const wire_rtti*>::value_type(iid,pRtti));
				}
				catch (std::exception& e)
				{
					OMEGA_THROW(e);
				}
			}

			OMEGA_WIRE_MAGIC(Omega,IObject)

			// These are the remoteable interfaces
			OMEGA_DEFINE_INTERNAL_INTERFACE_PART2_NOPROXY
			(
				Omega, IException,

				OMEGA_METHOD_VOID(Throw,0,())
				OMEGA_METHOD(guid_t,GetThrownIID,0,())
				OMEGA_METHOD(IException*,GetCause,0,())
				OMEGA_METHOD(string_t,GetDescription,0,())
				OMEGA_METHOD(string_t,GetSource,0,())
			)

			OMEGA_DEFINE_INTERNAL_INTERFACE_PART2
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

			OMEGA_QI_MAGIC(Omega::Remoting,IMessage)
			OMEGA_QI_MAGIC(Omega::System,IStub)
			OMEGA_QI_MAGIC(Omega::System,IStubController)
			OMEGA_QI_MAGIC(Omega::System,IProxy)
			OMEGA_QI_MAGIC(Omega::System,IMarshaller)

		}
	}
}

#endif // OOCORE_WIRE_PS_H_INCLUDED_

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

			protected:
				Wire_Proxy_Base(Wire_Proxy_Owner* pOwner) : m_pOwner(pOwner)
				{
					m_internal.m_pOuter = this;
					AddRef();
				}

				inline virtual ~Wire_Proxy_Base();

				inline void Throw(const guid_t& iid);
				inline auto_iface_ptr<Remoting::IMarshaller> GetMarshaller();
				inline auto_iface_ptr<Remoting::IMessage> CreateMessage(Remoting::IMarshaller* pMarshaller, const guid_t& iid, uint32_t method_id);
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

					Wire_Proxy_Base* m_pOuter;
				};
				Internal m_internal;
				friend struct Internal;

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
				{ }

				virtual bool IsDerived__proxy__(const guid_t& /*iid*/) const
				{
					// Don't return OMEGA_GUIDOF(IObject) - Should be passed on to m_pOwner
					return false;
				}

				auto_iface_ptr<Remoting::IMessage> CreateMessage(Remoting::IMarshaller* pMarshaller, uint32_t method_id)
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
					if (IsDerived__proxy__(iid))
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
					if (iid == OMEGA_GUIDOF(IException)) 
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
					m_ptrProxy = create_safe_proxy<Remoting::IProxy>(proxy_shim);
					assert(m_ptrProxy);

					m_proxy_shim = proxy_shim;

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

				inline IObject* QueryInterface(const guid_t& iid);

				auto_iface_ptr<Remoting::IMarshaller> GetMarshaller()
				{
					return m_ptrMarshaller;
				}

				IObject* GetInternal() { return 0; }

				inline void RemoveBase(Wire_Proxy_Base* pProxy);
				inline const SafeShim* GetShim(const guid_t& iid);
				inline IObject* CreateProxy(const guid_t& wire_iid, const guid_t& iid = OMEGA_GUIDOF(IObject));
				inline void Throw(const guid_t& iid);
				inline auto_iface_ptr<Remoting::IMessage> CreateMessage(Remoting::IMarshaller* pMarshaller, const guid_t& iid, uint32_t method_id);
				inline void UnpackHeader(Remoting::IMessage* pMessage);

			private:
				Wire_Proxy_Owner(const Wire_Proxy_Owner&);
				Wire_Proxy_Owner& operator =(const Wire_Proxy_Owner&);

				Threading::Mutex                      m_lock;
				std::map<guid_t,Wire_Proxy_Base*>     m_iid_map;
				auto_iface_ptr<Remoting::IProxy>      m_ptrProxy;
				const SafeShim*                       m_proxy_shim;
				auto_iface_ptr<Remoting::IMarshaller> m_ptrMarshaller;
				SafeShim                              m_base_shim;
				IObject*                              m_pOuter;
				Threading::AtomicRefCount             m_refcount;
				Threading::AtomicRefCount             m_pincount;

				struct Internal : public IObject
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

					Wire_Proxy_Owner* m_pOwner;
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

					Wire_Proxy_Owner* m_pOwner;
				};
				SafeProxy m_safe_proxy;
				friend struct SafeProxy;

				inline virtual ~Wire_Proxy_Owner();

				inline const SafeShim* GetWireProxy();
				inline auto_iface_ptr<Wire_Proxy_Base> GetProxyBase(const guid_t& iid, const guid_t& fallback_iid, bool bQI);

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
					*retval = 0;
					if (*iid == OMEGA_GUIDOF(IObject))
					{
						static_cast<Wire_Proxy_Owner*>(shim->m_stub)->AddRef();
						*retval = &static_cast<Wire_Proxy_Owner*>(shim->m_stub)->m_base_shim;
					}

					return 0;
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

			inline System::MetaInfo::auto_iface_ptr<System::MetaInfo::Wire_Proxy_Owner> create_wire_proxy_owner(const SafeShim* proxy, IObject* pOuter);

			class Wire_Stub_Base : public Remoting::IStub
			{
			public:
				template <typename I>
				I* get_iface()
				{
					assert(SupportsInterface(OMEGA_GUIDOF(I)));
					return static_cast<I*>(static_cast<IObject*>(m_ptrI));
				}

				auto_iface_ptr<Remoting::IMarshaller>& GetMarshaller()
				{
					return m_ptrMarshaller;
				}

			protected:
				Wire_Stub_Base(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, IObject* pI) :
					m_ptrMarshaller(pMarshaller), m_ptrI(pI), m_pController(pController)
				{
					PinObjectPointer(m_pController);

					m_ptrMarshaller->AddRef();
					m_ptrI->AddRef();

					AddRef();
				}

				virtual ~Wire_Stub_Base()
				{
					UnpinObjectPointer(m_pController);
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

				static const uint32_t MethodCount = 3;	// This must match the proxy

			private:
				auto_iface_ptr<Remoting::IMarshaller> m_ptrMarshaller;
				auto_iface_ptr<IObject>               m_ptrI;
				Remoting::IStubController*            m_pController;
				Threading::AtomicRefCount             m_refcount;
				
				Wire_Stub_Base(const Wire_Stub_Base&);
				Wire_Stub_Base& operator = (const Wire_Stub_Base&);

				void AddRef()
				{
					m_refcount.AddRef();
				}

				void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release())
						delete this;
				}

				IObject* QueryInterface(const guid_t& iid)
				{
					if (iid == OMEGA_GUIDOF(IObject) ||
						iid == OMEGA_GUIDOF(Remoting::IStub))
					{
						AddRef();
						return this;
					}
					return 0;
				}

				void Invoke(Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					// Read the method id
					Invoke(pParamsIn->ReadUInt32(L"$method_id"),pParamsIn,pParamsOut);
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
				static IStub* create(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, IObject* pI)
				{
					Wire_Stub* pThis;
					OMEGA_NEW(pThis,Wire_Stub(pController,pMarshaller,pI));
					return pThis;
				}

			protected:
				Wire_Stub(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, IObject* pI) :
					Wire_Stub_Base(pController,pMarshaller,pI)
				{
				}

				virtual bool_t SupportsInterface(const guid_t& iid)
				{
					return (iid == OMEGA_GUIDOF(IObject));
				}

				virtual void Invoke(uint32_t method_id, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					return Wire_Stub_Base::Invoke(method_id,pParamsIn,pParamsOut);
				}

				static const uint32_t MethodCount = Wire_Stub_Base::MethodCount;
			};

			inline Remoting::IStub* create_wire_stub(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, const guid_t& iid, IObject* pObj);

			class wire_proxy_holder
			{
			public:
				inline void remove(const SafeShim* shim);
				inline Wire_Proxy_Owner* find(const SafeShim* shim);
				inline Wire_Proxy_Owner* add(const SafeShim* shim, Wire_Proxy_Owner* pOwner);

			private:
				Threading::Mutex                            m_lock;
				std::map<const SafeShim*,Wire_Proxy_Owner*> m_map;
			};
			typedef Threading::Singleton<wire_proxy_holder,Threading::InitialiseDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > WIRE_PROXY_HOLDER;

			struct wire_rtti
			{
				Wire_Proxy_Base* (*pfnCreateWireProxy)(Wire_Proxy_Owner* pOwner);
				Remoting::IStub* (*pfnCreateWireStub)(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, IObject* pI);
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
			OMEGA_QI_MAGIC(Omega::Remoting,IStub)
			OMEGA_QI_MAGIC(Omega::Remoting,IStubController)
			OMEGA_QI_MAGIC(Omega::Remoting,IProxy)
			OMEGA_QI_MAGIC(Omega::Remoting,IMarshaller)

		}
	}
}

#endif // OOCORE_WIRE_PS_H_INCLUDED_

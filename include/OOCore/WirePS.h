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
			inline IObject* create_wire_proxy(Remoting::IProxy* pProxy, const guid_t& iid, const guid_t& fallback_iid = OMEGA_GUIDOF(IObject));

			class wire_holder
			{
			public:
				inline IObject* add(IObject* pProxy, IObject* pObject);
				
				inline IObject* find(IObject* pProxy);

				inline void remove(IObject* pProxy);
				
			private:
				Threading::Mutex            m_lock;
				std::map<IObject*,IObject*> m_map;
			};
			typedef Threading::Singleton<wire_holder,Threading::InitialiseDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > WIRE_HOLDER;

			class Wire_Proxy_Base
			{
			protected:
				Wire_Proxy_Base(Remoting::IProxy* pProxy) : 
					 m_ptrProxy(pProxy)
				{
					m_ptrProxy->AddRef();
					m_internal.m_pProxy = this;

					m_ptrMarshaller = m_ptrProxy->GetMarshaller();
					
					static const IObject_Safe_VTable vt =
					{
						&AddRef_Safe,
						&Release_Safe,
						0,
						0,
						0,
						0,
						&GetWireProxy_Safe
					};
					m_shim.m_vtable = &vt;
					m_shim.m_stub = this;
					m_shim.m_iid = &OMEGA_GUIDOF(IObject);

					m_refcount.AddRef();
				}

				virtual ~Wire_Proxy_Base() 
				{ }

				virtual bool IsDerived__proxy__(const guid_t& iid) const = 0;
				virtual IObject* QIReturn__proxy__() = 0;

				virtual void AddRef()
				{
					m_refcount.AddRef();
				}

				virtual void Release()
				{
					assert(m_refcount.m_debug_value > 0);

					if (m_refcount.Release() && m_pincount.IsZero())
						delete this;
				}

				inline virtual IObject* QueryInterface(const guid_t& iid);

				auto_iface_ptr<Remoting::IMarshaller> GetMarshaller()
				{
					return m_ptrMarshaller;
				}

				inline auto_iface_ptr<Remoting::IMessage> CreateMessage(const guid_t& iid, uint32_t method_id);
				inline void UnpackHeader(Remoting::IMessage* pMessage);

				inline IException* Throw(const guid_t& iid);

			private:
				friend class Wire_Proxy_IObject;

				Wire_Proxy_Base(const Wire_Proxy_Base&);
				Wire_Proxy_Base& operator =(const Wire_Proxy_Base&);

				auto_iface_ptr<Remoting::IProxy>      m_ptrProxy;
				auto_iface_ptr<Remoting::IMarshaller> m_ptrMarshaller;
				SafeShim                              m_shim;

				struct Internal : public ISafeProxy
				{
					void AddRef()
					{
						m_pProxy->AddRef();
					}

					void Release()
					{
						m_pProxy->Release();
					}

					IObject* QueryInterface(const guid_t& iid)
					{
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

					Wire_Proxy_Base* m_pProxy;
				};
				friend struct Internal;
				Internal m_internal;

				Threading::AtomicRefCount m_refcount;
				Threading::AtomicRefCount m_pincount;

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

				const SafeShim* GetShim(const Omega::guid_t& /*iid*/)
				{
					AddRef();
					return &m_shim;
				}

				const SafeShim* CreateWireStub(const SafeShim* /*shim_Controller*/, const SafeShim* /*shim_Marshaller*/, const Omega::guid_t& /*iid*/)
				{
					return 0;
				}

				static const SafeShim* OMEGA_CALL AddRef_Safe(const SafeShim* shim)
				{
					const SafeShim* except = 0;
					try
					{
						static_cast<Wire_Proxy_Base*>(shim->m_stub)->AddRef();
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
						static_cast<Wire_Proxy_Base*>(shim->m_stub)->Release();
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
						*retval = create_safe_stub(static_cast<Wire_Proxy_Base*>(shim->m_stub)->m_ptrProxy,OMEGA_GUIDOF(Remoting::IProxy));
					}
					catch (IException* pE)
					{
						except = return_safe_exception(pE);
					}
					return except;
				}
			};
			
			template <typename I, typename D>
			class Wire_Proxy;

			template <typename D>
			class Wire_Proxy<IObject,D> : public D, public Wire_Proxy_Base
			{
			public:
				static IObject* bind(Remoting::IProxy* pProxy)
				{
					Wire_Proxy* pThis;
					OMEGA_NEW(pThis,Wire_Proxy(pProxy));
					return pThis->QIReturn__proxy__();
				}

			protected:
				Wire_Proxy(Remoting::IProxy* pProxy) : 
					 Wire_Proxy_Base(pProxy)
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

				auto_iface_ptr<Remoting::IMessage> CreateMessage(uint32_t method_id)
				{
					return Wire_Proxy_Base::CreateMessage(OMEGA_GUIDOF(D),method_id);
				}

				static const uint32_t MethodCount = 4;

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
					return Wire_Proxy_Base::QueryInterface(iid);
				}
			};

			class Wire_Proxy_IObject : public Wire_Proxy<IObject,IObject>
			{
			public:
				static IObject* bind(Remoting::IProxy* pProxy)
				{
					Wire_Proxy_IObject* pThis;
					OMEGA_NEW(pThis,Wire_Proxy_IObject(pProxy));

					// Add to the map...
					IObject* pExisting = WIRE_HOLDER::instance()->add(pThis->m_ptrProxyObj,pThis);
					if (pExisting)
					{
						pThis->Release();
						return pExisting;
					}

					return pThis;
				}

			private:
				auto_iface_ptr<IObject> m_ptrProxyObj;

				Wire_Proxy_IObject(Remoting::IProxy* pProxy) : 
					 Wire_Proxy<IObject,IObject>(pProxy)
				{ 
					// QI for base object of pProxy
					m_ptrProxyObj = pProxy->QueryInterface(OMEGA_GUIDOF(IObject));
				}

				virtual ~Wire_Proxy_IObject() 
				{
					WIRE_HOLDER::instance()->remove(m_ptrProxyObj);
				}

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					// We are derived from IObject
					return (iid == OMEGA_GUIDOF(IObject));
				}
			};

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
					m_ptrMarshaller(pMarshaller), m_ptrI(pI), m_pController(pController), m_bPinned(false)
				{
					m_bPinned = PinObjectPointer(m_pController);

					m_ptrMarshaller->AddRef();
					m_ptrI->AddRef();

					AddRef();
				}

				virtual ~Wire_Stub_Base()
				{
					if (m_bPinned)
						UnpinObjectPointer(m_pController);
				}

				typedef void (*MethodTableEntry)(Wire_Stub_Base* pThis, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut);

				virtual void Invoke(uint32_t method_id, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					static const MethodTableEntry MethodTable[] =
					{
						&Release_Wire,
						&QueryInterface_Wire,
						&QueryIObject_Wire,
						&MarshalStub_Wire
					};

					if (method_id < MethodCount)
						return MethodTable[method_id](this,pParamsIn,pParamsOut);

					OMEGA_THROW(L"Invoke called with invalid method index");
				}

				static const uint32_t MethodCount = 4;	// This must match the proxy

			private:
				auto_iface_ptr<Remoting::IMarshaller> m_ptrMarshaller;
				auto_iface_ptr<IObject>               m_ptrI;
				Remoting::IStubController*            m_pController;
				bool                                  m_bPinned;
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
				
				static void Release_Wire(Wire_Stub_Base* pThis, Remoting::IMessage*, Remoting::IMessage*)
				{
					pThis->m_pController->RemoteRelease();
				}

				static void QueryInterface_Wire(Wire_Stub_Base* pThis, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut)
				{
					pParamsOut->WriteBoolean(L"$retval",pThis->m_pController->RemoteQueryInterface(pParamsIn->ReadGuid(L"iid")));
				}

				static void QueryIObject_Wire(Wire_Stub_Base* pThis, Remoting::IMessage*, Remoting::IMessage* pParamsOut)
				{
					auto_iface_ptr<IObject> ptrObj = pThis->m_ptrI->QueryInterface(OMEGA_GUIDOF(IObject));
					pThis->m_ptrMarshaller->MarshalInterface(L"$retval",pParamsOut,OMEGA_GUIDOF(IObject),ptrObj);
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

			struct wire_rtti
			{
				IObject* (*pfnCreateWireProxy)(Remoting::IProxy* pProxy);
				Remoting::IStub* (*pfnCreateWireStub)(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, IObject* pI);
			};

			typedef Threading::Singleton<std::map<guid_t,const wire_rtti*>,Threading::ModuleDestructor<OMEGA_PRIVATE_TYPE(safe_module)> > WIRE_RTTI_HOLDER;

			inline static const wire_rtti* get_wire_rtti_info(const guid_t& iid)
			{
				try
				{
					std::map<guid_t,const wire_rtti*>* iid_map = WIRE_RTTI_HOLDER::instance();
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
					WIRE_RTTI_HOLDER::instance()->insert(std::map<guid_t,const wire_rtti*>::value_type(iid,pRtti));
				}
				catch (std::exception& e)
				{
					OMEGA_THROW(e);
				}
			}

			OMEGA_WIRE_MAGIC(Omega,IObject)

			template <typename D>
			class Wire_Proxy<IException,D> : public Wire_Proxy<IObject,D>
			{
				typedef Wire_Proxy<IObject,D> Base;

			public:
				static IObject* bind(Remoting::IProxy* pProxy)
				{
					Wire_Proxy* pThis; 
					OMEGA_NEW(pThis,Wire_Proxy(pProxy));
					return pThis->QIReturn__proxy__();
				}

			protected:
				Wire_Proxy(Remoting::IProxy* pProxy) : 
					 Base(pProxy) 
				{ } 

				virtual bool IsDerived__proxy__(const guid_t& iid) const
				{
					if (iid == OMEGA_GUIDOF(IException)) 
						return true;

					return Base::IsDerived__proxy__(iid);
				}

				static const uint32_t MethodCount = Base::MethodCount + 5;

			private:
				void Throw()
				{
					guid_t iid = GetThrownIID();
					if (IsDerived__proxy__(iid))
						throw static_cast<D*>(this);
					
					IException* pE = Wire_Proxy_Base::Throw(iid);
					if (!pE)
						throw static_cast<D*>(this);

					this->Release();
					pE->Throw();
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

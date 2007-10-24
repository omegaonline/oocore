#ifndef OOCORE_REMOTING_H_INCLUDED_
#define OOCORE_REMOTING_H_INCLUDED_

#include <OOCore/OOCore.h>

namespace Omega
{
	namespace Remoting
	{
		//interface IMarshal : public IObject
		//{
		//	enum Flags
		//	{
		//		apartment = 0,
		//		process = 1,
		//		machine = 2
		//	};
		//	typedef uint16_t Flags_t;

		//	virtual guid_t GetUnmarshalOID(const guid_t& iid, Flags_t flags) = 0;
		//	virtual void Marshal(Serialize::IFormattedStream* pOutput, const guid_t& iid, Flags_t flags) = 0;
		//	virtual IObject* Unmarshal(Serialize::IFormattedStream* pInput, const guid_t& iid) = 0;
		//	//virtual void ReleaseMarshalData(Serialize::IFormattedStream* pInput) = 0;
		//	//virtual void DisconnectObject() = 0;
		//};

		interface ICallContext : public IObject
		{
		};

		interface IChannel : public IObject
		{
			virtual Serialize::IFormattedStream* CreateOutputStream(IObject* pOuter = 0) = 0;
			virtual Serialize::IFormattedStream* SendAndReceive(MethodAttributes_t attribs, Serialize::IFormattedStream* pStream, uint16_t timeout) = 0;
		};

		interface IObjectManager : public IObject
		{
			virtual void Connect(IChannel* pChannel) = 0;
			virtual void Invoke(Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut) = 0;
			virtual void Disconnect() = 0;
			virtual void CreateRemoteInstance(const guid_t& oid, const guid_t& iid, IObject* pOuter, IObject*& pObject) = 0;
		};

		interface IInterProcessService : public IObject
		{
			virtual Registry::IRegistryKey* GetRegistry() = 0;
			virtual Activation::IRunningObjectTable* GetRunningObjectTable() = 0;
		};

		// {63EB243E-6AE3-43bd-B073-764E096775F8}
		OOCORE_DECLARE_OID(OID_StdObjectManager);

		// {7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}
		OOCORE_DECLARE_OID(OID_InterProcess);
	}
}

//OMEGA_DEFINE_INTERFACE
//(
//	Omega::Remoting, IMarshal, "{5EE81A3F-88AA-47ee-9CAA-CECC8BE8F4C4}",
//
//	OMEGA_METHOD(guid_t,GetUnmarshalOID,2,((in),const guid_t&,iid,(in),Omega::Remoting::IMarshal::Flags_t,flags))
//    OMEGA_METHOD_VOID(Marshal,3,((in),Serialize::IFormattedStream*,pOutput,(in),const guid_t&,iid,(in),Omega::Remoting::IMarshal::Flags_t,flags))
//    OMEGA_METHOD(IObject*,Unmarshal,2,((in),Serialize::IFormattedStream*,pInput,(in),const guid_t&,iid))
//    //OMEGA_METHOD_VOID(ReleaseMarshalData,1,((in),Serialize::IFormattedStream*,pInput))
//    //OMEGA_METHOD_VOID(DisconnectObject,0,())
//)

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IChannel, "{F18430B0-8AC5-4b57-9B66-56B3BE867C24}",

	OMEGA_METHOD(Serialize::IFormattedStream*,CreateOutputStream,1,((in),IObject*,pOuter))
	OMEGA_METHOD(Serialize::IFormattedStream*,SendAndReceive,3,((in),Remoting::MethodAttributes_t,attribs,(in),Serialize::IFormattedStream*,pStream,(in),uint16_t,timeout))
)

OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IObjectManager, "{0A6F7B1B-26A0-403c-AC80-ADFADA83615D}",

	OMEGA_METHOD_VOID(Connect,1,((in),Remoting::IChannel*,pChannel))
	OMEGA_METHOD_VOID(Invoke,2,((in),Serialize::IFormattedStream*,pParamsIn,(in),Serialize::IFormattedStream*,pParamsOut))
	OMEGA_METHOD_VOID(Disconnect,0,())
	OMEGA_METHOD_VOID(CreateRemoteInstance,4,((in),const guid_t&,oid,(in),const guid_t&,iid,(in),IObject*,pOuter,(out)(iid_is(iid)),IObject*&,pObject))
)

/*OMEGA_DEFINE_INTERFACE
(
	Omega::Remoting, IInterProcessService, "{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}",

	OMEGA_METHOD(Registry::IRegistryKey*,GetRegistry,0,())
	OMEGA_METHOD(Activation::IRunningObjectTable*,GetRunningObjectTable,0,())
)*/

struct __declspec(uuid("{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}")) 
Omega::Remoting::IInterProcessService; 

namespace Omega 
{ 
	namespace System 
	{ 
		namespace MetaInfo 
		{ 
			template <class Base> class IInterProcessService95_Impl_Safe; 
			template <class I, class Base> class IInterProcessService95_SafeStub; 
			template <class I, class Base> class IInterProcessService95_SafeProxy; 
			template <class I, class Base> class IInterProcessService95_WireStub; 
			template <class Base> class IInterProcessService95_WireProxy; 
			template <> struct interface_info<Omega::Remoting::IInterProcessService> 
			{ 
				typedef IInterProcessService95_Impl_Safe<interface_info<Omega::IObject>::safe_class> safe_class; 
				template <class I> struct safe_stub_factory 
				{ 
					typedef IInterProcessService95_SafeStub<I,typename interface_info<Omega::IObject>::safe_stub_factory<I>::type> type; 
				}; 
				template <class I> struct safe_proxy_factory 
				{
					typedef IInterProcessService95_SafeProxy<I,typename interface_info<Omega::IObject>::safe_proxy_factory<I>::type> type;
				}; 
				template <class I> struct wire_stub_factory 
				{ 
					typedef IInterProcessService95_WireStub<I,typename interface_info<Omega::IObject>::wire_stub_factory<I>::type> type;
				}; 
				template <class I> struct wire_proxy_factory 
				{ 
					typedef IInterProcessService95_WireProxy<typename interface_info<Omega::IObject>::wire_proxy_factory<I>::type> type;
				}; 
			}; 
			template <> struct marshal_info<Omega::Remoting::IInterProcessService*> 
			{ 
				typedef iface_safe_type<Omega::Remoting::IInterProcessService> safe_type;
				typedef iface_wire_type<Omega::Remoting::IInterProcessService> wire_type;
			};
			template <class Base> class IInterProcessService95_Impl_Safe : public Base 
			{ 
			public:
				virtual IException_Safe* __cdecl GetRegistry_Safe (marshal_info<Registry::IRegistryKey*&>::safe_type::type GetRegistry_RetVal  ) = 0; 
				virtual IException_Safe* __cdecl GetRunningObjectTable_Safe (marshal_info<Activation::IRunningObjectTable*&>::safe_type::type GetRunningObjectTable_RetVal  ) = 0; 
			}; 
			template <class I, class Base> class IInterProcessService95_SafeStub : public Base 
			{ 
			public:
				IInterProcessService95_SafeStub(SafeStub* pStub, I* pI) : Base(pStub,pI) { } 

				virtual IException_Safe* Internal_QueryInterface_Safe(bool bRecurse, const guid_t* piid, IObject_Safe** ppObjS) 
				{ 
					if (*piid == (*reinterpret_cast<const Omega::guid_t*>(&__uuidof(Omega::Remoting::IInterProcessService)))) 
					{ 
						*ppObjS = static_cast<interface_info<Omega::Remoting::IInterProcessService>::safe_class*>(this); 
						(*ppObjS)->AddRef_Safe(); 
						return 0; 
					} 
					return Base::Internal_QueryInterface_Safe(bRecurse,piid,ppObjS); 
				} 
				virtual IException_Safe* __cdecl GetRegistry_Safe (marshal_info<Registry::IRegistryKey*&>::safe_type::type GetRegistry_RetVal  ) 
				{ 
					try 
					{ 
						static_cast<Registry::IRegistryKey*&>(marshal_info<Registry::IRegistryKey*&>::safe_type::coerce(GetRegistry_RetVal)) = this->m_pI->GetRegistry(  ); 
						return 0; 
					} 
					catch (IException* GetRegistry_Exception) 
					{ 
						return return_safe_exception(GetRegistry_Exception); 
					} 
				} 
				virtual IException_Safe* __cdecl GetRunningObjectTable_Safe (marshal_info<Activation::IRunningObjectTable*&>::safe_type::type GetRunningObjectTable_RetVal  ) 
				{ 
					try 
					{ 
						static_cast<Activation::IRunningObjectTable*&>(marshal_info<Activation::IRunningObjectTable*&>::safe_type::coerce(GetRunningObjectTable_RetVal)) = this->m_pI->GetRunningObjectTable(  ); 
						return 0;
					} 
					catch (IException* GetRunningObjectTable_Exception) 
					{ 
						return return_safe_exception(GetRunningObjectTable_Exception); 
					} 
				} 
			}; 
			template <class I, class Base> class IInterProcessService95_SafeProxy : public Base 
			{ 
			public:
				IInterProcessService95_SafeProxy(SafeProxy* pProxy, typename interface_info<I>::safe_class* pS) : Base(pProxy,pS) { } 
				virtual IObject* Internal_QueryInterface(bool bRecurse, const guid_t& iid) 
				{ 
					if (iid == (*reinterpret_cast<const Omega::guid_t*>(&__uuidof(Omega::Remoting::IInterProcessService)))) 
					{ 
						this->AddRef(); 
						return static_cast<Omega::Remoting::IInterProcessService*>(this); 
					} 
					return Base::Internal_QueryInterface(bRecurse,iid); 
				} 
				Registry::IRegistryKey* GetRegistry( ) 
				{ 
					Registry::IRegistryKey* GetRegistry_RetVal = Omega::System::MetaInfo::default_value<Registry::IRegistryKey*>::value(); 
					IException_Safe* GetRegistry_Exception = this->m_pS->GetRegistry_Safe( marshal_info<Registry::IRegistryKey*&>::safe_type::coerce(GetRegistry_RetVal)  ); 
					if (GetRegistry_Exception) 
						throw_correct_exception(GetRegistry_Exception); 
					return GetRegistry_RetVal; 
				} 
				Activation::IRunningObjectTable* GetRunningObjectTable( ) 
				{ 
					Activation::IRunningObjectTable* GetRunningObjectTable_RetVal = Omega::System::MetaInfo::default_value<Activation::IRunningObjectTable*>::value(); 
					IException_Safe* GetRunningObjectTable_Exception = this->m_pS->GetRunningObjectTable_Safe( marshal_info<Activation::IRunningObjectTable*&>::safe_type::coerce(GetRunningObjectTable_RetVal)  ); 
					if (GetRunningObjectTable_Exception) 
						throw_correct_exception(GetRunningObjectTable_Exception); 
					return GetRunningObjectTable_RetVal; 
				} 
			private: 
				IInterProcessService95_SafeProxy(const IInterProcessService95_SafeProxy&) {}; 
				IInterProcessService95_SafeProxy& operator = (const IInterProcessService95_SafeProxy&) {}; 
			}; 
			namespace 
			{ 
				class name95_RttiInit 
				{ 
				public:
					name95_RttiInit() 
					{ 
						static const qi_rtti s_rtti = 
						{ 
							SafeStubImpl<interface_info<Omega::Remoting::IInterProcessService>::safe_stub_factory<Omega::Remoting::IInterProcessService>::type,Omega::Remoting::IInterProcessService>::Create, 
							SafeProxyImpl<interface_info<Omega::Remoting::IInterProcessService>::safe_proxy_factory<Omega::Remoting::IInterProcessService>::type,Omega::Remoting::IInterProcessService>::Create, 
							SafeThrow<Omega::Remoting::IInterProcessService>, 
							L"Omega::Remoting::IInterProcessService" 
						}; 
						register_rtti_info((*reinterpret_cast<const Omega::guid_t*>(&__uuidof(Omega::Remoting::IInterProcessService))),&s_rtti); 
					} 
				}; 
				static const name95_RttiInit name95_RttiInit_i; 
			} 
			template <class I, class Base> class IInterProcessService95_WireStub : public Base 
			{ 
			public:
				IInterProcessService95_WireStub(IWireManager_Safe* pManager, IObject_Safe* pObj) : Base(pManager,pObj) 
				{} 
				virtual IException_Safe* __cdecl Invoke_Safe(uint32_t method_id, IFormattedStream_Safe* pParamsIn, IFormattedStream_Safe* pParamsOut) 
				{ 
					static const typename Base::MethodTableEntry MethodTable[] = 
					{ 
						GetRegistry_Wire, 
						GetRunningObjectTable_Wire
					}; 
					if (method_id < Base::MethodCount) 
						return Base::Invoke_Safe(method_id,pParamsIn,pParamsOut); 
					else if (method_id < MethodCount) 
						return MethodTable[method_id - Base::MethodCount](this,this->m_pS,pParamsIn,pParamsOut); 
					else 
						return return_safe_exception(IException::Create(L"Invalid method index")); 
				} 
				static const uint32_t MethodCount = Base::MethodCount + 2; 
				static IException_Safe* GetRegistry_Wire(void* __wire__pParam, I* __wire__pI, IFormattedStream_Safe* __wire__pParamsIn, IFormattedStream_Safe* __wire__pParamsOut) 
				{ 
					(__wire__pParam); (__wire__pParamsIn); (__wire__pParamsOut);  
					IException_Safe* __wire__pException = 0;  
					marshal_info<Registry::IRegistryKey*>::wire_type::type GetRegistry_RetVal = Omega::System::MetaInfo::default_value<marshal_info<Registry::IRegistryKey*>::wire_type::type>::value();  
					__wire__pException = __wire__pI->GetRegistry_Safe ( &GetRegistry_RetVal  );  
					if (__wire__pException) return __wire__pException;
					__wire__pException = marshal_info<Registry::IRegistryKey*>::wire_type::write(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut,GetRegistry_RetVal); 
					return __wire__pException; 
				} 
				static IException_Safe* GetRunningObjectTable_Wire(void* __wire__pParam, I* __wire__pI, IFormattedStream_Safe* __wire__pParamsIn, IFormattedStream_Safe* __wire__pParamsOut) 
				{ 
					(__wire__pParam); (__wire__pParamsIn); (__wire__pParamsOut);  
					IException_Safe* __wire__pException = 0;  
					marshal_info<Activation::IRunningObjectTable*>::wire_type::type GetRunningObjectTable_RetVal = Omega::System::MetaInfo::default_value<marshal_info<Activation::IRunningObjectTable*>::wire_type::type>::value();  
					__wire__pException = __wire__pI->GetRunningObjectTable_Safe ( &GetRunningObjectTable_RetVal  ); 
					if (__wire__pException) return __wire__pException;
					__wire__pException = marshal_info<Activation::IRunningObjectTable*>::wire_type::write(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut,GetRunningObjectTable_RetVal); 
					return __wire__pException; 
				} 
			private: 
				IInterProcessService95_WireStub() {}; 
				IInterProcessService95_WireStub(const IInterProcessService95_WireStub&) {}; 
				IInterProcessService95_WireStub& operator =(const IInterProcessService95_WireStub&) {}; 
			}; 
			template <class Base> class IInterProcessService95_WireProxy : public Base 
			{ 
			public:
				IInterProcessService95_WireProxy(IWireProxy_Safe* pProxy, IWireManager_Safe* pManager) : Base(pProxy,pManager) { }
				virtual IException_Safe* Internal_QueryInterface_Safe(bool bRecurse, const guid_t* piid, IObject_Safe** ppS) 
				{ 
					if (*piid == (*reinterpret_cast<const Omega::guid_t*>(&__uuidof(Omega::Remoting::IInterProcessService)))) 
					{ 
						*ppS = static_cast<interface_info<Omega::Remoting::IInterProcessService>::safe_class*>(this); 
						(*ppS)->AddRef_Safe(); 
						return 0; 
					} 
					return Base::Internal_QueryInterface_Safe(bRecurse,piid,ppS); 
				} 
				IException_Safe* __cdecl GetRegistry_Safe(marshal_info<Registry::IRegistryKey*&>::safe_type::type GetRegistry_RetVal ) 
				{ 
					auto_iface_safe_ptr<IFormattedStream_Safe> __wire__pParamsOut; 
					IException_Safe* GetRegistry_Exception_Safe = this->CreateOutputStream(__wire__pParamsOut); 
					if (GetRegistry_Exception_Safe) 
						return GetRegistry_Exception_Safe; 
					GetRegistry_Exception_Safe = this->WriteKey(__wire__pParamsOut,this_iid()); 
					if (GetRegistry_Exception_Safe) 
						return GetRegistry_Exception_Safe; 
					GetRegistry_Exception_Safe = wire_write(__wire__pParamsOut,GetRegistry_MethodId); 
					if (GetRegistry_Exception_Safe) 
						return GetRegistry_Exception_Safe;  
					auto_iface_safe_ptr<IFormattedStream_Safe> __wire__pParamsIn; 
					GetRegistry_Exception_Safe = this->SendAndReceive(0,__wire__pParamsOut,__wire__pParamsIn); 
					if (GetRegistry_Exception_Safe) 
						return GetRegistry_Exception_Safe;  
					GetRegistry_Exception_Safe = marshal_info<Registry::IRegistryKey*>::wire_type::read(this->m_pManager,__wire__pParamsIn,*GetRegistry_RetVal); 
					return GetRegistry_Exception_Safe; 
				} 
				static const uint32_t GetRegistry_MethodId = Base::MethodCount + 0; 
				IException_Safe* __cdecl GetRunningObjectTable_Safe(marshal_info<Activation::IRunningObjectTable*&>::safe_type::type GetRunningObjectTable_RetVal ) 
				{ 
					auto_iface_safe_ptr<IFormattedStream_Safe> __wire__pParamsOut; 
					IException_Safe* GetRunningObjectTable_Exception_Safe = this->CreateOutputStream(__wire__pParamsOut); 
					if (GetRunningObjectTable_Exception_Safe) 
						return GetRunningObjectTable_Exception_Safe; 
					GetRunningObjectTable_Exception_Safe = this->WriteKey(__wire__pParamsOut,this_iid()); 
					if (GetRunningObjectTable_Exception_Safe) 
						return GetRunningObjectTable_Exception_Safe; 
					GetRunningObjectTable_Exception_Safe = wire_write(__wire__pParamsOut,GetRunningObjectTable_MethodId); 
					if (GetRunningObjectTable_Exception_Safe) 
						return GetRunningObjectTable_Exception_Safe;  
					auto_iface_safe_ptr<IFormattedStream_Safe> __wire__pParamsIn; 
					GetRunningObjectTable_Exception_Safe = this->SendAndReceive(0,__wire__pParamsOut,__wire__pParamsIn); 
					if (GetRunningObjectTable_Exception_Safe) 
						return GetRunningObjectTable_Exception_Safe;  
					GetRunningObjectTable_Exception_Safe = marshal_info<Activation::IRunningObjectTable*>::wire_type::read(this->m_pManager,__wire__pParamsIn,*GetRunningObjectTable_RetVal); 
					return GetRunningObjectTable_Exception_Safe; 
				} 
				static const uint32_t GetRunningObjectTable_MethodId = Base::MethodCount + 1; 
				static const uint32_t MethodCount = Base::MethodCount + 2; 
			private: 
				IInterProcessService95_WireProxy(const IInterProcessService95_WireProxy&) {}; 
				IInterProcessService95_WireProxy& operator = (const IInterProcessService95_WireProxy&) {}; 
				const guid_t& this_iid() const { return OMEGA_UUIDOF(Omega::Remoting::IInterProcessService); }
			}; 
			namespace 
			{ 
				class name95_WireInit 
				{ 
				public:
					static __declspec(dllexport) Omega::System::MetaInfo::IException_Safe* __cdecl create_wire_stub(Omega::System::MetaInfo::IWireStub_Safe** ppStub , Omega::System::MetaInfo::marshal_info<IWireManager*>::safe_type::type pManager , Omega::System::MetaInfo::marshal_info<Omega::IObject*>::safe_type::type pObject) 
					{ 
						try 
						{ 
							*ppStub = CreateWireStub<interface_info<Omega::Remoting::IInterProcessService>::wire_stub_factory<interface_info<Omega::Remoting::IInterProcessService>::safe_class>::type>(pManager,pObject); 
							return 0; 
						} 
						catch (Omega::IException* name_Exception) 
						{ 
							return Omega::System::MetaInfo::return_safe_exception(name_Exception); 
						} 
					} 
					static __declspec(dllexport) Omega::System::MetaInfo::IException_Safe* __cdecl create_wire_proxy(Omega::System::MetaInfo::IObject_Safe** ppRet, Omega::System::MetaInfo::marshal_info<Omega::System::MetaInfo::IWireProxy*>::safe_type::type pProxy , Omega::System::MetaInfo::marshal_info<Omega::System::MetaInfo::IWireManager*>::safe_type::type pManager) 
					{ 
						try 
						{ 
							*ppRet = WireProxyImpl<interface_info<Omega::Remoting::IInterProcessService>::wire_proxy_factory<interface_info<Omega::Remoting::IInterProcessService>::safe_class>::type>::Create(pProxy,pManager); 
							return 0; 
						} 
						catch (Omega::IException* name_Exception) 
						{ 
							return Omega::System::MetaInfo::return_safe_exception(name_Exception); 
						} 
					} 
					name95_WireInit() 
					{ 
						RegisterWireFactories((*reinterpret_cast<const Omega::guid_t*>(&__uuidof(Omega::Remoting::IInterProcessService))),(void*)&create_wire_proxy,(void*)&create_wire_stub); 
					} 
				}; 
				static const name95_WireInit name95_WireInit_i; 
			} 
		} 
	} 
}

#endif // OOCORE_REMOTING_H_INCLUDED_

///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OOCORE_SERVER_H_INCLUDED_
#define OOCORE_SERVER_H_INCLUDED_

#include <OOCore/Remoting.h>

namespace Omega
{
	namespace System
	{
		interface IInterProcessService : public IObject
		{
			virtual Registry::IKey* GetRegistry() = 0;
			virtual Activation::IRunningObjectTable* GetRunningObjectTable() = 0;
			virtual void LaunchObjectApp(const guid_t& oid, const guid_t& iid, IObject*& pObject) = 0;
			virtual bool_t HandleRequest(uint32_t timeout) = 0;
			virtual Remoting::IChannel* OpenRemoteChannel(const string_t& strEndpoint) = 0;
			virtual Remoting::IChannelSink* OpenServerSink(const guid_t& message_oid, Remoting::IChannelSink* pSink) = 0;
		};

		// {7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}
		extern const Omega::guid_t OID_InterProcessService;
	}
}

/*OMEGA_DEFINE_INTERFACE
(
	Omega::System, IInterProcessService, "{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}",

	OMEGA_METHOD(Registry::IKey*,GetRegistry,0,())
	OMEGA_METHOD(Activation::IRunningObjectTable*,GetRunningObjectTable,0,())
	OMEGA_METHOD_VOID(LaunchObjectApp,3,((in),const guid_t&,oid,(in),const guid_t&,iid,(out)(iid_is(iid)),IObject*&,pObject))
	OMEGA_METHOD(bool_t,HandleRequest,1,((in),uint32_t,timeout))
	OMEGA_METHOD(Remoting::IChannel*,OpenRemoteChannel,1,((in),const string_t&,strEndpoint))
	OMEGA_METHOD(Remoting::IChannelSink*,OpenServerSink,2,((in),const guid_t&,message_oid,(in),Remoting::IChannelSink*,pSink))
)*/

struct __declspec(uuid("{70F6D098-6E53-4e8d-BF21-9EA359DC4FF8}")) Omega::System::IInterProcessService; 
namespace Omega 
{ 
	namespace System 
	{
		namespace MetaInfo 
		{ 
			template <> struct marshal_info<Omega::System::IInterProcessService*> { typedef iface_safe_type<Omega::System::IInterProcessService> safe_type; typedef iface_wire_type<Omega::System::IInterProcessService> wire_type; };
			template <> struct type_kind<Omega::System::IInterProcessService*> { static const TypeInfo::Types_t type = TypeInfo::typeObject; };
			struct IInterProcessService56_Safe_VTable { vtable_info<Omega::IObject>::type base_vtable; SafeShim* (__cdecl* pfnGetRegistry_Safe)(SafeShim*, marshal_info<Registry::IKey*&>::safe_type::type  ); SafeShim* (__cdecl* pfnGetRunningObjectTable_Safe)(SafeShim*, marshal_info<Activation::IRunningObjectTable*&>::safe_type::type  ); SafeShim* (__cdecl* pfnLaunchObjectApp_Safe)(SafeShim* , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type oid , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type iid , Omega::System::MetaInfo::marshal_info<IObject*&>::safe_type::type pObject ); SafeShim* (__cdecl* pfnHandleRequest_Safe)(SafeShim*, marshal_info<bool_t&>::safe_type::type , Omega::System::MetaInfo::marshal_info<uint32_t>::safe_type::type timeout ); SafeShim* (__cdecl* pfnOpenRemoteChannel_Safe)(SafeShim*, marshal_info<Remoting::IChannel*&>::safe_type::type , Omega::System::MetaInfo::marshal_info<const string_t&>::safe_type::type strEndpoint ); SafeShim* (__cdecl* pfnOpenServerSink_Safe)(SafeShim*, marshal_info<Remoting::IChannelSink*&>::safe_type::type , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type message_oid , Omega::System::MetaInfo::marshal_info<Remoting::IChannelSink*>::safe_type::type pSink ); };
			template <> struct vtable_info<Omega::System::IInterProcessService> { typedef IInterProcessService56_Safe_VTable type; };
			template <> class TypeInfo_Holder<Omega::System::IInterProcessService> { public: static const typeinfo_rtti* get_type_info() { static const typeinfo_rtti ti = { &method_info, method_count, &Omega::guid_t::FromUuidof(__uuidof(Omega::IObject)) }; return &ti; }; static const uint32_t method_count = TypeInfo_Holder<Omega::IObject>::method_count + 6; private: static const typeinfo_rtti::MethodInfo* method_info() { static const typeinfo_rtti::ParamInfo GetRegistry_params[] = {  { 0, 0, 0, L"", 0 } }; static const typeinfo_rtti::ParamInfo GetRunningObjectTable_params[] = {  { 0, 0, 0, L"", 0 } }; static const typeinfo_rtti::ParamInfo LaunchObjectApp_params[] = { { L"oid",type_kind<const guid_t&>::type, TypeInfo::attrIn, L"", typeinfo_rtti::has_guid_t<type_kind<const guid_t&>::type==TypeInfo::typeObject,const guid_t&>::guid() }, { L"iid",type_kind<const guid_t&>::type, TypeInfo::attrIn, L"", typeinfo_rtti::has_guid_t<type_kind<const guid_t&>::type==TypeInfo::typeObject,const guid_t&>::guid() }, { L"pObject",type_kind<IObject*&>::type, TypeInfo::attrOut | TypeInfo::attrIid_is , L"" L"iid" , typeinfo_rtti::has_guid_t<type_kind<IObject*&>::type==TypeInfo::typeObject,IObject*&>::guid() }, { 0, 0, 0, L"", 0 } }; static const typeinfo_rtti::ParamInfo HandleRequest_params[] = { { L"timeout",type_kind<uint32_t>::type, TypeInfo::attrIn, L"", typeinfo_rtti::has_guid_t<type_kind<uint32_t>::type==TypeInfo::typeObject,uint32_t>::guid() }, { 0, 0, 0, L"", 0 } }; static const typeinfo_rtti::ParamInfo OpenRemoteChannel_params[] = { { L"strEndpoint",type_kind<const string_t&>::type, TypeInfo::attrIn, L"", typeinfo_rtti::has_guid_t<type_kind<const string_t&>::type==TypeInfo::typeObject,const string_t&>::guid() }, { 0, 0, 0, L"", 0 } }; static const typeinfo_rtti::ParamInfo OpenServerSink_params[] = { { L"message_oid",type_kind<const guid_t&>::type, TypeInfo::attrIn, L"", typeinfo_rtti::has_guid_t<type_kind<const guid_t&>::type==TypeInfo::typeObject,const guid_t&>::guid() }, { L"pSink",type_kind<Remoting::IChannelSink*>::type, TypeInfo::attrIn, L"", typeinfo_rtti::has_guid_t<type_kind<Remoting::IChannelSink*>::type==TypeInfo::typeObject,Remoting::IChannelSink*>::guid() }, { 0, 0, 0, L"", 0 } }; static const typeinfo_rtti::MethodInfo method_infos[] = { { L"GetRegistry",TypeInfo::Synchronous,0,0,type_kind<Registry::IKey*>::type,GetRegistry_params }, { L"GetRunningObjectTable",TypeInfo::Synchronous,0,0,type_kind<Activation::IRunningObjectTable*>::type,GetRunningObjectTable_params }, { L"LaunchObjectApp",TypeInfo::Synchronous,0,3,TypeInfo::typeVoid,LaunchObjectApp_params }, { L"HandleRequest",TypeInfo::Synchronous,0,1,type_kind<bool_t>::type,HandleRequest_params }, { L"OpenRemoteChannel",TypeInfo::Synchronous,0,1,type_kind<Remoting::IChannel*>::type,OpenRemoteChannel_params }, { L"OpenServerSink",TypeInfo::Synchronous,0,2,type_kind<Remoting::IChannelSink*>::type,OpenServerSink_params }, { 0, 0, 0, 0, 0, 0 } }; return method_infos; } };
			
			template <class D> class Safe_Proxy<Omega::System::IInterProcessService,D> : 
				public Safe_Proxy<Omega::IObject,D> 
			{ 
				const vtable_info<Omega::System::IInterProcessService>::type* deref_vt() 
				{ 
					return static_cast<const vtable_info<Omega::System::IInterProcessService>::type*>(this->m_shim->m_vtable); 
				} 
			public: 
				static Safe_Proxy_Base* bind(SafeShim* shim, Safe_Proxy_Owner* pOwner) 
				{ 
					Safe_Proxy* pThis; do { pThis = new (std::nothrow) Safe_Proxy(shim,pOwner); if (!pThis) throw Omega::ISystemException::Create(14L,(Omega::string_t::Format(L"%hs(%u): %ls","c:\\work\\omegaonline\\src\\oocore\\../Common/Server.h",56,Omega::string_t( __FUNCSIG__  ,false).c_str()))); } while (!pThis); 
					return pThis; 
				} 
			protected: 
				Safe_Proxy(SafeShim* shim, Safe_Proxy_Owner* pOwner = 0) : 
					Safe_Proxy<Omega::IObject,D>(shim,pOwner) 
				{} 
				virtual bool IsDerived(const guid_t& iid) const { if (iid == Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService))) return true; return Safe_Proxy<Omega::IObject,D>::IsDerived(iid); } 
			public: 
				Registry::IKey* GetRegistry( ) 
				{ 
					Registry::IKey* GetRegistry_RetVal = Omega::System::MetaInfo::default_value<Registry::IKey*>::value(); 
					SafeShim* GetRegistry_except = deref_vt()->pfnGetRegistry_Safe( this->m_shim,marshal_info<Registry::IKey*&>::safe_type::coerce(GetRegistry_RetVal) ); 
					if (GetRegistry_except) 
						throw_correct_exception(GetRegistry_except); 
					return GetRegistry_RetVal; 
				} 
											
				Activation::IRunningObjectTable* GetRunningObjectTable( ) 
				{ 
					Activation::IRunningObjectTable* GetRunningObjectTable_RetVal = Omega::System::MetaInfo::default_value<Activation::IRunningObjectTable*>::value(); 
					SafeShim* GetRunningObjectTable_except = deref_vt()->pfnGetRunningObjectTable_Safe( this->m_shim,marshal_info<Activation::IRunningObjectTable*&>::safe_type::coerce(GetRunningObjectTable_RetVal) ); 
					if (GetRunningObjectTable_except) 
						throw_correct_exception(GetRunningObjectTable_except); 
					return GetRunningObjectTable_RetVal; 
				}
				
				void LaunchObjectApp( const guid_t& oid , const guid_t& iid , IObject*& pObject ) 
				{ SafeShim* LaunchObjectApp_except = deref_vt()->pfnLaunchObjectApp_Safe(this->m_shim , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(oid  ) , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(iid  ) , Omega::System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(pObject  ,iid  )); if (LaunchObjectApp_except) throw_correct_exception(LaunchObjectApp_except); }
				bool_t HandleRequest( uint32_t timeout ) 
				{ bool_t HandleRequest_RetVal = Omega::System::MetaInfo::default_value<bool_t>::value(); SafeShim* HandleRequest_except = deref_vt()->pfnHandleRequest_Safe( this->m_shim,marshal_info<bool_t&>::safe_type::coerce(HandleRequest_RetVal) , Omega::System::MetaInfo::marshal_info<uint32_t>::safe_type::coerce(timeout  )); if (HandleRequest_except) throw_correct_exception(HandleRequest_except); return HandleRequest_RetVal; }
				Remoting::IChannel* OpenRemoteChannel( const string_t& strEndpoint ) 
				{ Remoting::IChannel* OpenRemoteChannel_RetVal = Omega::System::MetaInfo::default_value<Remoting::IChannel*>::value(); SafeShim* OpenRemoteChannel_except = deref_vt()->pfnOpenRemoteChannel_Safe( this->m_shim,marshal_info<Remoting::IChannel*&>::safe_type::coerce(OpenRemoteChannel_RetVal) , Omega::System::MetaInfo::marshal_info<const string_t&>::safe_type::coerce(strEndpoint  )); if (OpenRemoteChannel_except) throw_correct_exception(OpenRemoteChannel_except); return OpenRemoteChannel_RetVal; }
				Remoting::IChannelSink* OpenServerSink( const guid_t& message_oid , Remoting::IChannelSink* pSink ) { Remoting::IChannelSink* OpenServerSink_RetVal = Omega::System::MetaInfo::default_value<Remoting::IChannelSink*>::value(); SafeShim* OpenServerSink_except = deref_vt()->pfnOpenServerSink_Safe( this->m_shim,marshal_info<Remoting::IChannelSink*&>::safe_type::coerce(OpenServerSink_RetVal) , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(message_oid  ) , Omega::System::MetaInfo::marshal_info<Remoting::IChannelSink*>::safe_type::coerce(pSink  )); if (OpenServerSink_except) throw_correct_exception(OpenServerSink_except); return OpenServerSink_RetVal; } };
			
			template <> class Safe_Stub<Omega::System::IInterProcessService> : public Safe_Stub<Omega::IObject> { public: static SafeShim* create(IObject* pI, Safe_Stub_Owner* pOwner, SafeShim** ret) { try { Safe_Stub* pThis; do { pThis = new (std::nothrow) Safe_Stub(static_cast<Omega::System::IInterProcessService*>(pI),&Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService)),pOwner); if (!pThis) throw Omega::ISystemException::Create(14L,(Omega::string_t::Format(L"%hs(%u): %ls","c:\\work\\omegaonline\\src\\oocore\\../Common/Server.h",56,Omega::string_t( __FUNCSIG__  ,false).c_str()))); } while (!pThis); *ret = &pThis->m_shim; return 0; } catch (IException* pE) { return return_safe_exception(pE); } } protected: Safe_Stub(Omega::System::IInterProcessService* pI, const guid_t* iid, Safe_Stub_Owner* pOwner) : Safe_Stub<Omega::IObject>(pI,iid,pOwner) { m_shim.m_vtable = get_vt(); } static const vtable_info<Omega::System::IInterProcessService>::type* get_vt() { static const vtable_info<Omega::System::IInterProcessService>::type vt = { *Safe_Stub<Omega::IObject>::get_vt() , &GetRegistry_Safe , &GetRunningObjectTable_Safe , &LaunchObjectApp_Safe , &HandleRequest_Safe , &OpenRemoteChannel_Safe , &OpenServerSink_Safe }; return &vt; } virtual bool IsDerived(const guid_t& iid) const { if (iid == Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService))) return true; return Safe_Stub<Omega::IObject>::IsDerived(iid); } private: static Omega::System::IInterProcessService* deref_shim(SafeShim* shim) { return static_cast<Omega::System::IInterProcessService*>(static_cast<Safe_Stub*>(shim->m_stub)->m_pI); } static SafeShim* __cdecl GetRegistry_Safe(SafeShim* GetRegistry_shim, marshal_info<Registry::IKey*&>::safe_type::type GetRegistry_RetVal  ) { SafeShim* GetRegistry_except = 0; try { static_cast<Registry::IKey*&>(marshal_info<Registry::IKey*&>::safe_type::coerce(GetRegistry_RetVal)) = deref_shim(GetRegistry_shim)->GetRegistry(  ); } catch (IException* GetRegistry_exception) { GetRegistry_except = return_safe_exception(GetRegistry_exception); } return GetRegistry_except; } static SafeShim* __cdecl GetRunningObjectTable_Safe(SafeShim* GetRunningObjectTable_shim, marshal_info<Activation::IRunningObjectTable*&>::safe_type::type GetRunningObjectTable_RetVal  ) { SafeShim* GetRunningObjectTable_except = 0; try { static_cast<Activation::IRunningObjectTable*&>(marshal_info<Activation::IRunningObjectTable*&>::safe_type::coerce(GetRunningObjectTable_RetVal)) = deref_shim(GetRunningObjectTable_shim)->GetRunningObjectTable(  ); } catch (IException* GetRunningObjectTable_exception) { GetRunningObjectTable_except = return_safe_exception(GetRunningObjectTable_exception); } return GetRunningObjectTable_except; } static SafeShim* __cdecl LaunchObjectApp_Safe(SafeShim* LaunchObjectApp_shim , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type oid , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type iid , Omega::System::MetaInfo::marshal_info<IObject*&>::safe_type::type pObject ) { SafeShim* LaunchObjectApp_except = 0; try { deref_shim(LaunchObjectApp_shim)->LaunchObjectApp(  Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(oid  ) , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(iid  ) , Omega::System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(pObject  ,iid  ) ); } catch (IException* LaunchObjectApp_exception) { LaunchObjectApp_except = return_safe_exception(LaunchObjectApp_exception); } return LaunchObjectApp_except; } static SafeShim* __cdecl HandleRequest_Safe(SafeShim* HandleRequest_shim, marshal_info<bool_t&>::safe_type::type HandleRequest_RetVal , Omega::System::MetaInfo::marshal_info<uint32_t>::safe_type::type timeout ) { SafeShim* HandleRequest_except = 0; try { static_cast<bool_t&>(marshal_info<bool_t&>::safe_type::coerce(HandleRequest_RetVal)) = deref_shim(HandleRequest_shim)->HandleRequest(  Omega::System::MetaInfo::marshal_info<uint32_t>::safe_type::coerce(timeout  ) ); } catch (IException* HandleRequest_exception) { HandleRequest_except = return_safe_exception(HandleRequest_exception); } return HandleRequest_except; } static SafeShim* __cdecl OpenRemoteChannel_Safe(SafeShim* OpenRemoteChannel_shim, marshal_info<Remoting::IChannel*&>::safe_type::type OpenRemoteChannel_RetVal , Omega::System::MetaInfo::marshal_info<const string_t&>::safe_type::type strEndpoint ) { SafeShim* OpenRemoteChannel_except = 0; try { static_cast<Remoting::IChannel*&>(marshal_info<Remoting::IChannel*&>::safe_type::coerce(OpenRemoteChannel_RetVal)) = deref_shim(OpenRemoteChannel_shim)->OpenRemoteChannel(  Omega::System::MetaInfo::marshal_info<const string_t&>::safe_type::coerce(strEndpoint  ) ); } catch (IException* OpenRemoteChannel_exception) { OpenRemoteChannel_except = return_safe_exception(OpenRemoteChannel_exception); } return OpenRemoteChannel_except; } static SafeShim* __cdecl OpenServerSink_Safe(SafeShim* OpenServerSink_shim, marshal_info<Remoting::IChannelSink*&>::safe_type::type OpenServerSink_RetVal , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type message_oid , Omega::System::MetaInfo::marshal_info<Remoting::IChannelSink*>::safe_type::type pSink ) { SafeShim* OpenServerSink_except = 0; try { static_cast<Remoting::IChannelSink*&>(marshal_info<Remoting::IChannelSink*&>::safe_type::coerce(OpenServerSink_RetVal)) = deref_shim(OpenServerSink_shim)->OpenServerSink(  Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(message_oid  ) , Omega::System::MetaInfo::marshal_info<Remoting::IChannelSink*>::safe_type::coerce(pSink  ) ); } catch (IException* OpenServerSink_exception) { OpenServerSink_except = return_safe_exception(OpenServerSink_exception); } return OpenServerSink_except; } };
			namespace { class IInterProcessService56_RttiInit { public: static const wchar_t* get_name() { return L"Omega::System::IInterProcessService"; } IInterProcessService56_RttiInit() { static const qi_rtti s_rtti = { &Safe_Proxy<Omega::System::IInterProcessService,Omega::System::IInterProcessService>::bind, &Safe_Stub<Omega::System::IInterProcessService>::create, get_name() }; register_rtti_info(Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService)),&s_rtti); RegisterAutoTypeInfo(Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService)),get_name(),TypeInfo_Holder<Omega::System::IInterProcessService>::get_type_info()); } ~IInterProcessService56_RttiInit() { UnregisterAutoTypeInfo(Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService)),TypeInfo_Holder<Omega::System::IInterProcessService>::get_type_info()); } }; static const IInterProcessService56_RttiInit IInterProcessService56_RttiInit_i; }
			
			template <> class Wire_Proxy<Omega::System::IInterProcessService> : 
				public Wire_Proxy<Omega::IObject> 
			{ 
			public: 
				static SafeShim* create(IProxy* pProxy) 
				{ 
					Wire_Proxy* pThis; 
					do { pThis = new (std::nothrow) Wire_Proxy(pProxy,&Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService))); if (!pThis) throw Omega::ISystemException::Create(14L,(Omega::string_t::Format(L"%hs(%u): %ls","c:\\work\\omegaonline\\src\\oocore\\../Common/Server.h",56,Omega::string_t( __FUNCSIG__  ,false).c_str()))); } while (!pThis); 
					return &pThis->m_shim; 
				}
			protected: 
				Wire_Proxy(IProxy* pProxy, const guid_t* iid) : 
					Wire_Proxy<Omega::IObject>(pProxy,iid) 
				{ 
					m_shim.m_vtable = get_vt(); 
				} 
				virtual bool IsDerived(const guid_t& iid) const 
				{ 
					if (iid == Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService))) 
						return true; 
					return Wire_Proxy<Omega::IObject>::IsDerived(iid); 
				} 
				static const vtable_info<Omega::System::IInterProcessService>::type* get_vt() { static const vtable_info<Omega::System::IInterProcessService>::type vt = { *Wire_Proxy<Omega::IObject>::get_vt() , &GetRegistry_Safe , &GetRunningObjectTable_Safe , &LaunchObjectApp_Safe , &HandleRequest_Safe , &OpenRemoteChannel_Safe , &OpenServerSink_Safe }; return &vt; } 
				static const uint32_t MethodCount = Wire_Proxy<Omega::IObject>::MethodCount + 6; 
			private: 
				typedef Wire_Proxy<Omega::IObject> Base; 
				Registry::IKey* GetRegistry() 
				{ 
					auto_iface_ptr<Remoting::IMessage> pParamsOut__wire__ = this->CreateMessage(GetRegistry_MethodId); 
					auto_iface_ptr<Remoting::IMessage> pParamsIn__wire__; 
					size_t unpack_count__wire__ = 0; (unpack_count__wire__); 
					IException* GetRegistry_Exception = 0; 
					try 
					{  
						pParamsOut__wire__->WriteStructEnd(L"ipc_request"); 
						GetRegistry_Exception = m_ptrMarshaller->SendAndReceive(TypeInfo::Synchronous,pParamsOut__wire__,pParamsIn__wire__,0); 
					} 
					catch (...) 
					{ 
						this->UnpackHeader(pParamsOut__wire__);  
						throw; 
					} 
					if (GetRegistry_Exception) 
						throw GetRegistry_Exception;  
					marshal_info<Registry::IKey*>::wire_type::type GetRegistry_RetVal; 
					marshal_info<Registry::IKey*>::wire_type::read(L"$retval",m_ptrMarshaller,pParamsIn__wire__,GetRegistry_RetVal); 
					return GetRegistry_RetVal; 
				}
				static SafeShim* __cdecl GetRegistry_Safe(SafeShim* GetRegistry_shim, marshal_info<Registry::IKey*&>::safe_type::type GetRegistry_RetVal  ) 
				{ 
					SafeShim* GetRegistry_except = 0; 
					try 
					{ 
						static_cast<Registry::IKey*&>(marshal_info<Registry::IKey*&>::safe_type::coerce(GetRegistry_RetVal)) = static_cast<Wire_Proxy*>(GetRegistry_shim->m_stub)->GetRegistry(  ); 
					} 
					catch (IException* GetRegistry_exception) 
					{ 
						GetRegistry_except = return_safe_exception(GetRegistry_exception); 
					} 
					return GetRegistry_except; 
				} 
				static const uint32_t GetRegistry_MethodId = Base::MethodCount + 0; 
				
				Activation::IRunningObjectTable* GetRunningObjectTable() 
				{ 
					auto_iface_ptr<Remoting::IMessage> pParamsOut__wire__ = this->CreateMessage(GetRunningObjectTable_MethodId); 
					auto_iface_ptr<Remoting::IMessage> pParamsIn__wire__; 
					size_t unpack_count__wire__ = 0; (unpack_count__wire__); 
					IException* GetRunningObjectTable_Exception = 0; 
					try 
					{  
						pParamsOut__wire__->WriteStructEnd(L"ipc_request"); 
						GetRunningObjectTable_Exception = m_ptrMarshaller->SendAndReceive(TypeInfo::Synchronous,pParamsOut__wire__,pParamsIn__wire__,0); 
					} 
					catch (...) 
					{ 
						this->UnpackHeader(pParamsOut__wire__);  
						throw; 
					} 
					if (GetRunningObjectTable_Exception) 
						throw GetRunningObjectTable_Exception;  
					marshal_info<Activation::IRunningObjectTable*>::wire_type::type GetRunningObjectTable_RetVal; 
					marshal_info<Activation::IRunningObjectTable*>::wire_type::read(L"$retval",m_ptrMarshaller,pParamsIn__wire__,GetRunningObjectTable_RetVal); 
					return GetRunningObjectTable_RetVal; 
				} 
				static SafeShim* __cdecl GetRunningObjectTable_Safe(SafeShim* GetRunningObjectTable_shim, marshal_info<Activation::IRunningObjectTable*&>::safe_type::type GetRunningObjectTable_RetVal  ) 
				{ 
					SafeShim* GetRunningObjectTable_except = 0; 
					try 
					{ 
						static_cast<Activation::IRunningObjectTable*&>(marshal_info<Activation::IRunningObjectTable*&>::safe_type::coerce(GetRunningObjectTable_RetVal)) = static_cast<Wire_Proxy*>(GetRunningObjectTable_shim->m_stub)->GetRunningObjectTable(  ); 
					} 
					catch (IException* GetRunningObjectTable_exception) 
					{ 
						GetRunningObjectTable_except = return_safe_exception(GetRunningObjectTable_exception);
					} 
					return GetRunningObjectTable_except; 
				} 
				static const uint32_t GetRunningObjectTable_MethodId = Base::MethodCount + 1; 
				
				void LaunchObjectApp( const guid_t& oid , const guid_t& iid , IObject*& pObject) { auto_iface_ptr<Remoting::IMessage> pParamsOut__wire__ = this->CreateMessage(LaunchObjectApp_MethodId); auto_iface_ptr<Remoting::IMessage> pParamsIn__wire__; size_t unpack_count__wire__ = 0; (unpack_count__wire__); IException* LaunchObjectApp_Exception = 0; try { marshal_info<const guid_t&>::wire_type:: write(L"oid",this->m_ptrMarshaller,pParamsOut__wire__,oid ); unpack_count__wire__ = 0 + 1; marshal_info<const guid_t&>::wire_type:: write(L"iid",this->m_ptrMarshaller,pParamsOut__wire__,iid ); unpack_count__wire__ = 1 + 1; marshal_info<IObject*&>::wire_type:: no_op(false  ); unpack_count__wire__ = 2 + 1; pParamsOut__wire__->WriteStructEnd(L"ipc_request"); LaunchObjectApp_Exception = m_ptrMarshaller->SendAndReceive(TypeInfo::Synchronous,pParamsOut__wire__,pParamsIn__wire__,0); } catch (...) { this->UnpackHeader(pParamsOut__wire__); if (unpack_count__wire__ > 0) { marshal_info<const guid_t&>::wire_type:: no_op(false ); } if (unpack_count__wire__ > 1) { marshal_info<const guid_t&>::wire_type:: no_op(false ); } if (unpack_count__wire__ > 2) { marshal_info<IObject*&>::wire_type:: unpack(L"pObject",this->m_ptrMarshaller,pParamsOut__wire__,pObject  ); } throw; } if (LaunchObjectApp_Exception) throw LaunchObjectApp_Exception; marshal_info<const guid_t&>::wire_type:: no_op(false ); marshal_info<const guid_t&>::wire_type:: no_op(false ); marshal_info<IObject*&>::wire_type:: read(L"pObject",this->m_ptrMarshaller,pParamsIn__wire__,pObject  ); } static SafeShim* __cdecl LaunchObjectApp_Safe(SafeShim* LaunchObjectApp_shim , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type oid , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type iid , Omega::System::MetaInfo::marshal_info<IObject*&>::safe_type::type pObject ) { SafeShim* LaunchObjectApp_except = 0; try { static_cast<Wire_Proxy*>(LaunchObjectApp_shim->m_stub)->LaunchObjectApp(  Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(oid  ) , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(iid  ) , Omega::System::MetaInfo::marshal_info<IObject*&>::safe_type::coerce(pObject  ,iid  ) ); } catch (IException* LaunchObjectApp_exception) { LaunchObjectApp_except = return_safe_exception(LaunchObjectApp_exception); } return LaunchObjectApp_except; } static const uint32_t LaunchObjectApp_MethodId = Base::MethodCount + 2; bool_t HandleRequest( uint32_t timeout) { auto_iface_ptr<Remoting::IMessage> pParamsOut__wire__ = this->CreateMessage(HandleRequest_MethodId); auto_iface_ptr<Remoting::IMessage> pParamsIn__wire__; size_t unpack_count__wire__ = 0; (unpack_count__wire__); IException* HandleRequest_Exception = 0; try { marshal_info<uint32_t>::wire_type:: write(L"timeout",this->m_ptrMarshaller,pParamsOut__wire__,timeout ); unpack_count__wire__ = 0 + 1; pParamsOut__wire__->WriteStructEnd(L"ipc_request"); HandleRequest_Exception = m_ptrMarshaller->SendAndReceive(TypeInfo::Synchronous,pParamsOut__wire__,pParamsIn__wire__,0); } catch (...) { this->UnpackHeader(pParamsOut__wire__); if (unpack_count__wire__ > 0) { marshal_info<uint32_t>::wire_type:: no_op(false ); } throw; } if (HandleRequest_Exception) throw HandleRequest_Exception; marshal_info<uint32_t>::wire_type:: no_op(false ); marshal_info<bool_t>::wire_type::type HandleRequest_RetVal; marshal_info<bool_t>::wire_type::read(L"$retval",m_ptrMarshaller,pParamsIn__wire__,HandleRequest_RetVal); return HandleRequest_RetVal; } static SafeShim* __cdecl HandleRequest_Safe(SafeShim* HandleRequest_shim, marshal_info<bool_t&>::safe_type::type HandleRequest_RetVal , Omega::System::MetaInfo::marshal_info<uint32_t>::safe_type::type timeout ) { SafeShim* HandleRequest_except = 0; try { static_cast<bool_t&>(marshal_info<bool_t&>::safe_type::coerce(HandleRequest_RetVal)) = static_cast<Wire_Proxy*>(HandleRequest_shim->m_stub)->HandleRequest(  Omega::System::MetaInfo::marshal_info<uint32_t>::safe_type::coerce(timeout  ) ); } catch (IException* HandleRequest_exception) { HandleRequest_except = return_safe_exception(HandleRequest_exception); } return HandleRequest_except; } static const uint32_t HandleRequest_MethodId = Base::MethodCount + 3; Remoting::IChannel* OpenRemoteChannel( const string_t& strEndpoint) { auto_iface_ptr<Remoting::IMessage> pParamsOut__wire__ = this->CreateMessage(OpenRemoteChannel_MethodId); auto_iface_ptr<Remoting::IMessage> pParamsIn__wire__; size_t unpack_count__wire__ = 0; (unpack_count__wire__); IException* OpenRemoteChannel_Exception = 0; try { marshal_info<const string_t&>::wire_type:: write(L"strEndpoint",this->m_ptrMarshaller,pParamsOut__wire__,strEndpoint ); unpack_count__wire__ = 0 + 1; pParamsOut__wire__->WriteStructEnd(L"ipc_request"); OpenRemoteChannel_Exception = m_ptrMarshaller->SendAndReceive(TypeInfo::Synchronous,pParamsOut__wire__,pParamsIn__wire__,0); } catch (...) { this->UnpackHeader(pParamsOut__wire__); if (unpack_count__wire__ > 0) { marshal_info<const string_t&>::wire_type:: no_op(false ); } throw; } if (OpenRemoteChannel_Exception) throw OpenRemoteChannel_Exception; marshal_info<const string_t&>::wire_type:: no_op(false ); marshal_info<Remoting::IChannel*>::wire_type::type OpenRemoteChannel_RetVal; marshal_info<Remoting::IChannel*>::wire_type::read(L"$retval",m_ptrMarshaller,pParamsIn__wire__,OpenRemoteChannel_RetVal); return OpenRemoteChannel_RetVal; } static SafeShim* __cdecl OpenRemoteChannel_Safe(SafeShim* OpenRemoteChannel_shim, marshal_info<Remoting::IChannel*&>::safe_type::type OpenRemoteChannel_RetVal , Omega::System::MetaInfo::marshal_info<const string_t&>::safe_type::type strEndpoint ) { SafeShim* OpenRemoteChannel_except = 0; try { static_cast<Remoting::IChannel*&>(marshal_info<Remoting::IChannel*&>::safe_type::coerce(OpenRemoteChannel_RetVal)) = static_cast<Wire_Proxy*>(OpenRemoteChannel_shim->m_stub)->OpenRemoteChannel(  Omega::System::MetaInfo::marshal_info<const string_t&>::safe_type::coerce(strEndpoint  ) ); } catch (IException* OpenRemoteChannel_exception) { OpenRemoteChannel_except = return_safe_exception(OpenRemoteChannel_exception); } return OpenRemoteChannel_except; } static const uint32_t OpenRemoteChannel_MethodId = Base::MethodCount + 4; Remoting::IChannelSink* OpenServerSink( const guid_t& message_oid , Remoting::IChannelSink* pSink) { auto_iface_ptr<Remoting::IMessage> pParamsOut__wire__ = this->CreateMessage(OpenServerSink_MethodId); auto_iface_ptr<Remoting::IMessage> pParamsIn__wire__; size_t unpack_count__wire__ = 0; (unpack_count__wire__); IException* OpenServerSink_Exception = 0; try { marshal_info<const guid_t&>::wire_type:: write(L"message_oid",this->m_ptrMarshaller,pParamsOut__wire__,message_oid ); unpack_count__wire__ = 0 + 1; marshal_info<Remoting::IChannelSink*>::wire_type:: write(L"pSink",this->m_ptrMarshaller,pParamsOut__wire__,pSink ); unpack_count__wire__ = 1 + 1; pParamsOut__wire__->WriteStructEnd(L"ipc_request"); OpenServerSink_Exception = m_ptrMarshaller->SendAndReceive(TypeInfo::Synchronous,pParamsOut__wire__,pParamsIn__wire__,0); } catch (...) { this->UnpackHeader(pParamsOut__wire__); if (unpack_count__wire__ > 0) { marshal_info<const guid_t&>::wire_type:: no_op(false ); } if (unpack_count__wire__ > 1) { marshal_info<Remoting::IChannelSink*>::wire_type:: no_op(false ); } throw; } if (OpenServerSink_Exception) throw OpenServerSink_Exception; marshal_info<const guid_t&>::wire_type:: no_op(false ); marshal_info<Remoting::IChannelSink*>::wire_type:: no_op(false ); marshal_info<Remoting::IChannelSink*>::wire_type::type OpenServerSink_RetVal; marshal_info<Remoting::IChannelSink*>::wire_type::read(L"$retval",m_ptrMarshaller,pParamsIn__wire__,OpenServerSink_RetVal); return OpenServerSink_RetVal; } static SafeShim* __cdecl OpenServerSink_Safe(SafeShim* OpenServerSink_shim, marshal_info<Remoting::IChannelSink*&>::safe_type::type OpenServerSink_RetVal , Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::type message_oid , Omega::System::MetaInfo::marshal_info<Remoting::IChannelSink*>::safe_type::type pSink ) { SafeShim* OpenServerSink_except = 0; try { static_cast<Remoting::IChannelSink*&>(marshal_info<Remoting::IChannelSink*&>::safe_type::coerce(OpenServerSink_RetVal)) = static_cast<Wire_Proxy*>(OpenServerSink_shim->m_stub)->OpenServerSink(  Omega::System::MetaInfo::marshal_info<const guid_t&>::safe_type::coerce(message_oid  ) , Omega::System::MetaInfo::marshal_info<Remoting::IChannelSink*>::safe_type::coerce(pSink  ) ); } catch (IException* OpenServerSink_exception) { OpenServerSink_except = return_safe_exception(OpenServerSink_exception); } return OpenServerSink_except; } static const uint32_t OpenServerSink_MethodId = Base::MethodCount + 5; }; 
			
			template <> class Wire_Stub<Omega::System::IInterProcessService> : 
				public Wire_Stub<Omega::IObject> 
			{ 
			public: 
				static SafeShim* create(IStubController* pController, IMarshaller* pMarshaller, IObject* pI) 
				{ Wire_Stub* pThis; do { pThis = new (std::nothrow) Wire_Stub(pController,pMarshaller,pI); if (!pThis) throw Omega::ISystemException::Create(14L,(Omega::string_t::Format(L"%hs(%u): %ls","c:\\work\\omegaonline\\src\\oocore\\../Common/Server.h",56,Omega::string_t( __FUNCSIG__  ,false).c_str()))); } while (!pThis); return &pThis->m_shim; } 
			protected: 
				Wire_Stub(IStubController* pController, IMarshaller* pMarshaller, IObject* pI) : Wire_Stub<Omega::IObject>(pController,pMarshaller,pI) { } 
				virtual bool_t SupportsInterface(const guid_t& iid) { if (iid == Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService))) return true; return Wire_Stub<Omega::IObject>::SupportsInterface(iid); } 
				
				virtual void Internal_Invoke(uint32_t method_id, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut) 
				{ 
					static const MethodTableEntry MethodTable[] = 
					{  &GetRegistry_Wire , &GetRunningObjectTable_Wire , &LaunchObjectApp_Wire , &HandleRequest_Wire , &OpenRemoteChannel_Wire , &OpenServerSink_Wire }; 
					if (method_id >= Wire_Stub<Omega::IObject>::MethodCount && method_id < MethodCount) 
						return MethodTable[method_id - Wire_Stub<Omega::IObject>::MethodCount](this,pParamsIn,pParamsOut); 
					return Wire_Stub<Omega::IObject>::Internal_Invoke(method_id,pParamsIn,pParamsOut); 
				} 
				static const uint32_t MethodCount = Wire_Stub<Omega::IObject>::MethodCount + 6; 
			private: 
				typedef Omega::System::IInterProcessService iface; 
				static void GetRegistry_Wire(void* pParam__wire__, Remoting::IMessage* pParamsIn__wire__, Remoting::IMessage* pParamsOut__wire__) 
				{ 
					(pParam__wire__); (pParamsIn__wire__); (pParamsOut__wire__);   
					size_t unpack_count__wire__ = 0; (unpack_count__wire__); 
					try 
					{ 
						marshal_info<Registry::IKey*>::wire_type::type GetRegistry_RetVal = static_cast<Wire_Stub*>(pParam__wire__)->get_iface<iface>()->GetRegistry( );  
						marshal_info<Registry::IKey*>::wire_type::write(L"$retval",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsOut__wire__,GetRegistry_RetVal); 
					} 
					catch (...) 
					{  
						throw; 
					} 
				} 
				static void GetRunningObjectTable_Wire(void* pParam__wire__, Remoting::IMessage* pParamsIn__wire__, Remoting::IMessage* pParamsOut__wire__) { (pParam__wire__); (pParamsIn__wire__); (pParamsOut__wire__);   size_t unpack_count__wire__ = 0; (unpack_count__wire__); try { marshal_info<Activation::IRunningObjectTable*>::wire_type::type GetRunningObjectTable_RetVal = static_cast<Wire_Stub*>(pParam__wire__)->get_iface<iface>()->GetRunningObjectTable( );  marshal_info<Activation::IRunningObjectTable*>::wire_type::write(L"$retval",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsOut__wire__,GetRunningObjectTable_RetVal); } catch (...) {  throw; } } static void LaunchObjectApp_Wire(void* pParam__wire__, Remoting::IMessage* pParamsIn__wire__, Remoting::IMessage* pParamsOut__wire__) { (pParam__wire__); (pParamsIn__wire__); (pParamsOut__wire__); marshal_info<const guid_t&>::wire_type::type oid; marshal_info<const guid_t&>::wire_type::type iid; marshal_info<IObject*&>::wire_type::type pObject; marshal_info<const guid_t&>::wire_type::read(L"oid",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsIn__wire__,oid ); marshal_info<const guid_t&>::wire_type::read(L"iid",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsIn__wire__,iid ); marshal_info<IObject*&>::wire_type::init(pObject  ); size_t unpack_count__wire__ = 0; (unpack_count__wire__); try { static_cast<Wire_Stub*>(pParam__wire__)->get_iface<iface>()->LaunchObjectApp(  oid , iid , pObject ); marshal_info<const guid_t&>::wire_type::no_op(false ); unpack_count__wire__ = 0 + 1; marshal_info<const guid_t&>::wire_type::no_op(false ); unpack_count__wire__ = 1 + 1; marshal_info<IObject*&>::wire_type::write(L"pObject",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsOut__wire__,pObject  ); unpack_count__wire__ = 2 + 1; } catch (...) { if (unpack_count__wire__ > 0) { marshal_info<const guid_t&>::wire_type::no_op(false ); } if (unpack_count__wire__ > 1) { marshal_info<const guid_t&>::wire_type::no_op(false ); } if (unpack_count__wire__ > 2) { marshal_info<IObject*&>::wire_type::unpack(L"pObject",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsOut__wire__,pObject  ); } throw; } } static void HandleRequest_Wire(void* pParam__wire__, Remoting::IMessage* pParamsIn__wire__, Remoting::IMessage* pParamsOut__wire__) { (pParam__wire__); (pParamsIn__wire__); (pParamsOut__wire__); marshal_info<uint32_t>::wire_type::type timeout; marshal_info<uint32_t>::wire_type::read(L"timeout",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsIn__wire__,timeout ); size_t unpack_count__wire__ = 0; (unpack_count__wire__); try { marshal_info<bool_t>::wire_type::type HandleRequest_RetVal = static_cast<Wire_Stub*>(pParam__wire__)->get_iface<iface>()->HandleRequest( timeout ); marshal_info<uint32_t>::wire_type::no_op(false ); unpack_count__wire__ = 0 + 1; marshal_info<bool_t>::wire_type::write(L"$retval",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsOut__wire__,HandleRequest_RetVal); } catch (...) { if (unpack_count__wire__ > 0) { marshal_info<uint32_t>::wire_type::no_op(false ); } throw; } } static void OpenRemoteChannel_Wire(void* pParam__wire__, Remoting::IMessage* pParamsIn__wire__, Remoting::IMessage* pParamsOut__wire__) { (pParam__wire__); (pParamsIn__wire__); (pParamsOut__wire__); marshal_info<const string_t&>::wire_type::type strEndpoint; marshal_info<const string_t&>::wire_type::read(L"strEndpoint",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsIn__wire__,strEndpoint ); size_t unpack_count__wire__ = 0; (unpack_count__wire__); try { marshal_info<Remoting::IChannel*>::wire_type::type OpenRemoteChannel_RetVal = static_cast<Wire_Stub*>(pParam__wire__)->get_iface<iface>()->OpenRemoteChannel( strEndpoint ); marshal_info<const string_t&>::wire_type::no_op(false ); unpack_count__wire__ = 0 + 1; marshal_info<Remoting::IChannel*>::wire_type::write(L"$retval",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsOut__wire__,OpenRemoteChannel_RetVal); } catch (...) { if (unpack_count__wire__ > 0) { marshal_info<const string_t&>::wire_type::no_op(false ); } throw; } } static void OpenServerSink_Wire(void* pParam__wire__, Remoting::IMessage* pParamsIn__wire__, Remoting::IMessage* pParamsOut__wire__) { (pParam__wire__); (pParamsIn__wire__); (pParamsOut__wire__); marshal_info<const guid_t&>::wire_type::type message_oid; marshal_info<Remoting::IChannelSink*>::wire_type::type pSink; marshal_info<const guid_t&>::wire_type::read(L"message_oid",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsIn__wire__,message_oid ); marshal_info<Remoting::IChannelSink*>::wire_type::read(L"pSink",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsIn__wire__,pSink ); size_t unpack_count__wire__ = 0; (unpack_count__wire__); try { marshal_info<Remoting::IChannelSink*>::wire_type::type OpenServerSink_RetVal = static_cast<Wire_Stub*>(pParam__wire__)->get_iface<iface>()->OpenServerSink( message_oid , pSink ); marshal_info<const guid_t&>::wire_type::no_op(false ); unpack_count__wire__ = 0 + 1; marshal_info<Remoting::IChannelSink*>::wire_type::no_op(false ); unpack_count__wire__ = 1 + 1; marshal_info<Remoting::IChannelSink*>::wire_type::write(L"$retval",static_cast<Wire_Stub*>(pParam__wire__)->m_ptrMarshaller,pParamsOut__wire__,OpenServerSink_RetVal); } catch (...) { if (unpack_count__wire__ > 0) { marshal_info<const guid_t&>::wire_type::no_op(false ); } if (unpack_count__wire__ > 1) { marshal_info<Remoting::IChannelSink*>::wire_type::no_op(false ); } throw; } } };
			
			namespace { class IInterProcessService56_WireInit { public: IInterProcessService56_WireInit() { RegisterAutoProxyStubCreators(Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService)),&create_wire_proxy<Omega::System::IInterProcessService>,&create_wire_stub<Omega::System::IInterProcessService>); } ~IInterProcessService56_WireInit() { UnregisterAutoProxyStubCreators(Omega::guid_t::FromUuidof(__uuidof(Omega::System::IInterProcessService)),&create_wire_proxy<Omega::System::IInterProcessService>,&create_wire_stub<Omega::System::IInterProcessService>); } }; static const IInterProcessService56_WireInit IInterProcessService56_WireInit_i; }
		}
	}
}

#endif // OOCORE_SERVER_H_INCLUDED_

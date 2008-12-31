///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOCORE_MACROS_H_INCLUDED_
#define OOCORE_MACROS_H_INCLUDED_

#if !defined(DOXYGEN)

#include <OOCore/Preprocessor/sequence.h>
#include <OOCore/Preprocessor/repeat.h>
#include <OOCore/Preprocessor/comma.h>
#include <OOCore/Preprocessor/split.h>

#define OMEGA_UNIQUE_NAME(name) \
	OMEGA_CONCAT(name,__LINE__)

#define OMEGA_QI_MAGIC(n_space,iface) \
	namespace \
	{ \
		class OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_RttiInit) \
		{ \
		public: \
			OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_RttiInit)() \
			{ \
				static const qi_rtti s_rtti = \
				{ \
					SafeStubImpl<interface_info<n_space::iface>::safe_stub_factory<n_space::iface>::type,n_space::iface>::Create, \
					SafeProxyImpl<interface_info<n_space::iface>::safe_proxy_factory<n_space::iface>::type,n_space::iface>::Create, \
					SafeThrow<n_space::iface>, \
					OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(n_space::iface)) \
				}; \
				register_rtti_info(OMEGA_GUIDOF(n_space::iface),&s_rtti); \
			} \
		}; \
		static const OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_RttiInit) OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_RttiInit_i); \
	}

#define OMEGA_WIRE_MAGIC(n_space,iface) \
	namespace \
	{ \
		class OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_WireInit) \
		{ \
		public: \
			static Omega::System::MetaInfo::IException_Safe* OMEGA_CALL create_wire_proxy(Omega::System::MetaInfo::IProxy_Safe* pProxy, Omega::System::MetaInfo::IMarshaller_Safe* pManager, Omega::System::MetaInfo::IObject_Safe** ppProxy) \
			{ \
				try \
				{ \
					*ppProxy = ProxyImpl<interface_info<n_space::iface>::wire_proxy_factory<interface_info<n_space::iface>::safe_class>::type>::Create(pProxy,pManager); \
					return 0; \
				} \
				catch (Omega::IException* pE) \
				{ \
					return Omega::System::MetaInfo::return_safe_exception(pE); \
				} \
			} \
			static Omega::System::MetaInfo::IException_Safe* OMEGA_CALL create_wire_stub(IStubController_Safe* pController, IMarshaller_Safe* pManager, IObject_Safe* pObject, IStub_Safe** ppStub) \
			{ \
				try \
				{ \
					*ppStub = CreateStub<interface_info<n_space::iface>::wire_stub_factory<interface_info<n_space::iface>::safe_class>::type>(pController,pManager,pObject); \
					return 0; \
				} \
				catch (Omega::IException* pE) \
				{ \
					return Omega::System::MetaInfo::return_safe_exception(pE); \
				} \
			} \
			static Omega::System::MetaInfo::IException_Safe* OMEGA_CALL create_type_info(ITypeInfo_Safe** ppTypeInfo) \
			{ \
				try \
				{ \
					static_cast<TypeInfo::ITypeInfo*&>(marshal_info<TypeInfo::ITypeInfo*&>::safe_type::coerce(ppTypeInfo)) = TypeInfo_Impl<n_space::iface>::Create(); \
					return 0; \
				} \
				catch (Omega::IException* pE) \
				{ \
					return Omega::System::MetaInfo::return_safe_exception(pE); \
				} \
			} \
			OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_WireInit)() \
			{ \
				RegisterAutoProxyStubCreators(OMEGA_GUIDOF(n_space::iface),&create_wire_proxy,&create_wire_stub); \
				RegisterAutoTypeInfo(OMEGA_GUIDOF(n_space::iface),&create_type_info); \
			} \
		}; \
		static const OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_WireInit) OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_WireInit_i); \
	}

#define OMEGA_DECLARE_FORWARDS(unique,n_space,name,d_space,derived) \
	template <class Base> interface OMEGA_CONCAT_R(unique,_Impl_Safe); \
	template <class I, class Base> class OMEGA_CONCAT_R(unique,_SafeStub); \
	template <class I, class Base> class OMEGA_CONCAT_R(unique,_SafeProxy); \
	template <class I, class Base> class OMEGA_CONCAT_R(unique,_Stub); \
	template <class Base> class OMEGA_CONCAT_R(unique,_Proxy); \
	template <> \
	struct interface_info<n_space::name> \
	{ \
		typedef OMEGA_CONCAT_R(unique,_Impl_Safe)<interface_info<d_space::derived>::safe_class> safe_class; \
		template <class I> struct safe_stub_factory \
		{ \
			typedef OMEGA_CONCAT_R(unique,_SafeStub)<I,typename interface_info<d_space::derived>::safe_stub_factory<I>::type> type; \
		}; \
		template <class I> struct safe_proxy_factory \
		{ \
			typedef OMEGA_CONCAT_R(unique,_SafeProxy)<I,typename interface_info<d_space::derived>::safe_proxy_factory<I>::type> type; \
		}; \
		template <class I> struct wire_stub_factory \
		{ \
			typedef OMEGA_CONCAT_R(unique,_Stub)<I,typename interface_info<d_space::derived>::wire_stub_factory<I>::type> type; \
		}; \
		template <class I> struct wire_proxy_factory \
		{ \
			typedef OMEGA_CONCAT_R(unique,_Proxy)<typename interface_info<d_space::derived>::wire_proxy_factory<I>::type> type; \
		}; \
	}; \
	template <> struct marshal_info<n_space::name*> \
	{ \
		typedef iface_safe_type<n_space::name> safe_type; \
		typedef iface_wire_type<n_space::name> wire_type; \
	};

#define OMEGA_DECLARE_PARAM_I(meta,type,name) \
	type name

#define OMEGA_DECLARE_PARAM(index,params,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_I params

#define OMEGA_DECLARE_PARAMS(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_EMIT_PARAM_I(meta,type,name) \
	name

#define OMEGA_EMIT_PARAM_VOID(index,params,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_EMIT_PARAM_I params

#define OMEGA_EMIT_PARAMS_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_EMIT_PARAM_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_EMIT_PARAM(index,params,d) \
	, OMEGA_EMIT_PARAM_I params

#define OMEGA_EMIT_PARAMS(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_EMIT_PARAM,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_SAFE_I(meta,t,name) \
	Omega::System::MetaInfo::marshal_info<t>::safe_type::type name

#define OMEGA_DECLARE_PARAM_SAFE_VOID(index,params,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_SAFE_I params

#define OMEGA_DECLARE_PARAMS_SAFE_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_SAFE(index,params,d) \
	, OMEGA_DECLARE_PARAM_SAFE_I params

#define OMEGA_DECLARE_PARAMS_SAFE(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_PS_PARAM_in(t,name)
#define OMEGA_PS_PARAM_in_out(t,name)
#define OMEGA_PS_PARAM_out(t,name)
#define OMEGA_PS_PARAM_iid_is(iid)       ,iid OMEGA_PS_PARAM_II
#define OMEGA_PS_PARAM_size_is(size)     OMEGA_PS_PARAM_II
#define OMEGA_PS_PARAM_II(t,name)

#define OMEGA_PS_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_PS_PARAM_,meta) d

#define OMEGA_PS_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_PS_PARAM_I,meta,(type,name))

#define OMEGA_DECLARE_SAFE_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params) ) = 0;

#define OMEGA_DECLARE_SAFE_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (marshal_info<ret_type&>::safe_type::type OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params) ) = 0;

#define OMEGA_DECLARE_SAFE_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_,method)

#define OMEGA_DECLARE_SAFE_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_METHOD,methods,0)

#define OMEGA_DECLARE_SAFE(unique,methods,name) \
	template <class Base> \
	interface OMEGA_CONCAT_R(unique,_Impl_Safe) : public Base \
	{ \
		OMEGA_DECLARE_SAFE_METHODS(methods) \
	};

// Add extra meta info types here
#define OMEGA_DECLARE_TYPE_in(t,name)        TypeInfo::attrIn
#define OMEGA_DECLARE_TYPE_in_out(t,name)    TypeInfo::attrInOut
#define OMEGA_DECLARE_TYPE_out(t,name)       TypeInfo::attrOut
#define OMEGA_DECLARE_TYPE_iid_is(iid)       | TypeInfo::attrIid_is OMEGA_DECLARE_TYPE_II
#define OMEGA_DECLARE_TYPE_size_is(size)     | TypeInfo::attrSize_is OMEGA_DECLARE_TYPE_II
#define OMEGA_DECLARE_TYPE_II(t,name)

#define OMEGA_DECLARE_TYPE_PARAM_II(index,meta,d) \
	OMEGA_CONCAT(OMEGA_DECLARE_TYPE_,meta) d

#define OMEGA_DECLARE_TYPE_PARAM_I(meta,t,name) \
	{ OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),type_kind<t>::type, OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_DECLARE_TYPE_PARAM_II,meta,(t,name)) },

#define OMEGA_DECLARE_TYPE_PARAM(index,params,d) \
	OMEGA_DECLARE_TYPE_PARAM_I params

#define OMEGA_DECLARE_TYPE_PARAMS(name,param_count,params) \
	static const ParamInfo OMEGA_CONCAT(name,_params)[] = { \
		OMEGA_TUPLE_FOR_EACH(param_count,OMEGA_DECLARE_TYPE_PARAM,OMEGA_SPLIT_3(param_count,params),0) \
		{ 0, 0, 0 } };

#define OMEGA_DECLARE_TYPE_PARAM_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	OMEGA_DECLARE_TYPE_PARAMS(name,param_count,params)

#define OMEGA_DECLARE_TYPE_PARAM_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	OMEGA_DECLARE_TYPE_PARAMS(name,param_count,params)

#define OMEGA_DECLARE_TYPE_METHOD_PARAM(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_TYPE_PARAM_,method)

#define OMEGA_DECLARE_TYPE_METHOD_PARAMS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_TYPE_METHOD_PARAM,methods,0)

#define OMEGA_DECLARE_TYPE_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	{ OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),attribs,timeout,param_count,TypeInfo::typeVoid,OMEGA_CONCAT(name,_params) },

#define OMEGA_DECLARE_TYPE_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	{ OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),attribs,timeout,param_count,type_kind<ret_type>::type,OMEGA_CONCAT(name,_params) },

#define OMEGA_DECLARE_TYPE_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_TYPE_,method)

#define OMEGA_DECLARE_TYPE_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_TYPE_METHOD,methods,0)

#define OMEGA_DECLARE_TYPE_PART1(name,n_space,methods,d_space,derived) \
	template <> \
	interface TypeInfo_Impl<n_space::name> : public TypeInfoBase \
	{ \
		static TypeInfo::ITypeInfo* Create() \
		{ \
			static TypeInfo_Impl<n_space::name> instance; \
			return &instance; \
		} \
		uint32_t GetMethodCount() { return method_count; } \
		TypeInfo::ITypeInfo* GetBaseType() { return TypeInfo_Impl<d_space::derived>::Create(); } \
		guid_t GetIID() { return OMEGA_GUIDOF(n_space::name); } \
		string_t GetName() { return OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(n_space::name)); } \
		void GetMethodInfo(uint32_t method_idx, string_t& strName, TypeInfo::MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, TypeInfo::Types_t& return_type) \
			{ return get_method_info(method_idx,strName,attribs,timeout,param_count,return_type); } \
		void GetParamInfo(uint32_t method_idx, byte_t param_idx, string_t& strName, TypeInfo::Types_t& type, TypeInfo::ParamAttributes_t& attribs) \
			{ get_param_info(method_idx,param_idx,strName,type,attribs); } \
		byte_t GetAttributeRef(uint32_t method_idx, byte_t param_idx, TypeInfo::ParamAttributes_t attrib) \
			{ OMEGA_UNUSED_ARG(method_idx); OMEGA_UNUSED_ARG(param_idx); OMEGA_UNUSED_ARG(attrib); OMEGA_THROW(EINVAL); } \
		static const uint32_t method_count = TypeInfo_Impl<d_space::derived>::method_count + OMEGA_SEQUENCE_SIZEOF(methods); \
		static void get_method_info(uint32_t method_idx, string_t& strName, TypeInfo::MethodAttributes_t& attribs, uint32_t& timeout, byte_t& param_count, TypeInfo::Types_t& return_type) \
		{ \
			if (method_idx >= method_count) \
				OMEGA_THROW(EINVAL); \
			if (method_idx < TypeInfo_Impl<d_space::derived>::method_count) \
				return TypeInfo_Impl<d_space::derived>::get_method_info(method_idx,strName,attribs,timeout,param_count,return_type); \
			const MethodInfo& info = method_info()[method_idx - TypeInfo_Impl<d_space::derived>::method_count]; \
			strName = info.pszName; \
			attribs = info.attribs; \
			timeout = info.timeout; \
			param_count = info.param_count; \
			return_type = info.return_type; \
		}

#define OMEGA_DECLARE_TYPE_PART2(name,n_space,methods,d_space,derived) \
		static void get_param_info(uint32_t method_idx, byte_t param_idx, string_t& strName, TypeInfo::Types_t& type, TypeInfo::ParamAttributes_t& attribs) \
		{ \
			if (method_idx >= method_count) \
				OMEGA_THROW(EINVAL); \
			if (method_idx < TypeInfo_Impl<d_space::derived>::method_count) \
				return TypeInfo_Impl<d_space::derived>::get_param_info(method_idx,param_idx,strName,type,attribs); \
			method_idx -= TypeInfo_Impl<d_space::derived>::method_count; \
			if (param_idx >= method_info()[method_idx].param_count) \
				OMEGA_THROW(EINVAL); \
			const ParamInfo& info = method_info()[method_idx].params[param_idx]; \
			strName = info.pszName; \
			type = info.type; \
			attribs = info.attribs; \
		} \
	private: \
		static const MethodInfo* method_info() \
		{ \
			OMEGA_DECLARE_TYPE_METHOD_PARAMS(methods) \
			static const MethodInfo method_infos[] = \
			{ \
				OMEGA_DECLARE_TYPE_METHODS(methods) \
				{ 0, 0, 0, 0 } \
			}; \
			return method_infos; \
		} \
	};

#define OMEGA_DECLARE_TYPE(name,n_space,methods,d_space,derived) \
	OMEGA_DECLARE_TYPE_PART1(name,n_space,methods,d_space,derived) \
	OMEGA_DECLARE_TYPE_PART2(name,n_space,methods,d_space,derived)

#define OMEGA_DECLARE_TYPEKIND(name,n_space) \
	template <> interface type_kind<n_space::name*> \
	{ \
		static const TypeInfo::Types_t type = TypeInfo::typeObject; \
	};

#define OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params) ) \
	{ \
		try \
		{ \
			this->m_pI->name( OMEGA_DECLARE_PARAMS_SAFE_STUB(param_count,params) ); \
			return 0; \
		} \
		catch (IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	}

#define OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (marshal_info<ret_type&>::safe_type::type OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params) ) \
	{ \
		try \
		{ \
			static_cast<ret_type&>(marshal_info<ret_type&>::safe_type::coerce(OMEGA_CONCAT(name,_RetVal))) = this->m_pI->name( OMEGA_DECLARE_PARAMS_SAFE_STUB(param_count,params) ); \
			return 0; \
		} \
		catch (IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	}

#define OMEGA_DECLARE_PARAM_SAFE_STUB_I(meta,t,name) \
	Omega::System::MetaInfo::marshal_info<t>::safe_type::coerce(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DECLARE_PARAM_SAFE_STUB(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_SAFE_STUB_I param

#define OMEGA_DECLARE_PARAMS_SAFE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_STUB,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_SAFE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_STUB_,method)

#define OMEGA_DECLARE_SAFE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_STUB_METHOD,methods,0)

// Add extra meta info types here
#define OMEGA_DECLARE_PARAM_WIRE_STUB_I(meta,t,name) \
	marshal_info<t>::wire_type::type name;

#define OMEGA_DECLARE_PARAM_WIRE_STUB(index,param,d) \
	OMEGA_DECLARE_PARAM_WIRE_STUB_I param

#define OMEGA_DECLARE_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_READ_STUB_PARAM_in(t,name)        read(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsIn,name
#define OMEGA_WIRE_READ_STUB_PARAM_in_out(t,name)    read(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsIn,name
#define OMEGA_WIRE_READ_STUB_PARAM_out(t,name)       init(name
#define OMEGA_WIRE_READ_STUB_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_READ_STUB_PARAM_II
#define OMEGA_WIRE_READ_STUB_PARAM_size_is(size)     ,size OMEGA_WIRE_READ_STUB_PARAM_II
#define OMEGA_WIRE_READ_STUB_PARAM_II(t,name)

#define OMEGA_WIRE_READ_STUB_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_READ_STUB_PARAM_,meta) d

#define OMEGA_WIRE_READ_STUB_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_READ_STUB_PARAM_I,meta,(type,name))

#define OMEGA_READ_PARAM_WIRE_STUB_I(meta,t,name) \
	__wire__pException = marshal_info<t>::wire_type:: OMEGA_WIRE_READ_STUB_PARAM(meta,t,name) ); if (__wire__pException) return __wire__pException;

#define OMEGA_READ_PARAM_WIRE_STUB(index,param,d) \
	OMEGA_READ_PARAM_WIRE_STUB_I param

#define OMEGA_READ_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_READ_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_WRITE_STUB_PARAM_in(t,name)        no_op(false
#define OMEGA_WIRE_WRITE_STUB_PARAM_in_out(t,name)    write(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut,name
#define OMEGA_WIRE_WRITE_STUB_PARAM_out(t,name)       write(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut,name
#define OMEGA_WIRE_WRITE_STUB_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_WRITE_STUB_PARAM_II
#define OMEGA_WIRE_WRITE_STUB_PARAM_size_is(size)     ,size OMEGA_WIRE_WRITE_STUB_PARAM_II
#define OMEGA_WIRE_WRITE_STUB_PARAM_II(t,name)

#define OMEGA_WIRE_WRITE_STUB_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_WRITE_STUB_PARAM_,meta) d

#define OMEGA_WIRE_WRITE_STUB_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_WRITE_STUB_PARAM_I,meta,(type,name))

#define OMEGA_WRITE_PARAM_WIRE_STUB_I(meta,t,name) \
	marshal_info<t>::wire_type:: OMEGA_WIRE_WRITE_STUB_PARAM(meta,t,name)

#define OMEGA_WRITE_PARAM_WIRE_STUB(index,param,d) \
	__wire__pException = OMEGA_WRITE_PARAM_WIRE_STUB_I param ); if (__wire__pException) goto __wire_Cleanup; __wire_unpack_count = index + 1;

#define OMEGA_WRITE_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_WRITE_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_UNPACK_STUB_PARAM_in(t,name)        no_op(false
#define OMEGA_WIRE_UNPACK_STUB_PARAM_in_out(t,name)    unpack(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut,name
#define OMEGA_WIRE_UNPACK_STUB_PARAM_out(t,name)       unpack(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut,name
#define OMEGA_WIRE_UNPACK_STUB_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_UNPACK_STUB_PARAM_II
#define OMEGA_WIRE_UNPACK_STUB_PARAM_size_is(size)     ,size OMEGA_WIRE_UNPACK_STUB_PARAM_II
#define OMEGA_WIRE_UNPACK_STUB_PARAM_II(t,name)

#define OMEGA_WIRE_UNPACK_STUB_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_UNPACK_STUB_PARAM_,meta) d

#define OMEGA_WIRE_UNPACK_STUB_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_UNPACK_STUB_PARAM_I,meta,(type,name))

#define OMEGA_UNPACK_PARAM_WIRE_STUB_I(meta,t,name) \
	marshal_info<t>::wire_type:: OMEGA_WIRE_UNPACK_STUB_PARAM(meta,t,name)

#define OMEGA_UNPACK_PARAM_WIRE_STUB(index,param,d) \
	if (__wire_unpack_count > index) { __wire__pException2 = OMEGA_UNPACK_PARAM_WIRE_STUB_I param ); if (__wire__pException2) { __wire__pException->Release_Safe(); return __wire__pException2; } }

#define OMEGA_UNPACK_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_UNPACK_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_WIRE_STUB_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	OMEGA_CONCAT(name,_Wire)

#define OMEGA_DECLARE_WIRE_STUB_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	OMEGA_CONCAT(name,_Wire)

#define OMEGA_DECLARE_WIRE_STUB_METHOD(index,method,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_CONCAT_R(OMEGA_DECLARE_WIRE_STUB_,method)

#define OMEGA_DECLARE_WIRE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_WIRE_STUB_METHOD,methods,0)

#define OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	static IException_Safe* OMEGA_CONCAT(name,_Wire)(void* __wire__pParam, IMessage_Safe* __wire__pParamsIn, IMessage_Safe* __wire__pParamsOut) \
	{ \
		OMEGA_UNUSED_ARG(__wire__pParam); OMEGA_UNUSED_ARG(__wire__pParamsIn); OMEGA_UNUSED_ARG(__wire__pParamsOut); \
		OMEGA_DECLARE_PARAMS_WIRE_STUB(param_count,params) \
		IException_Safe* __wire__pException = 0; \
		OMEGA_READ_PARAMS_WIRE_STUB(param_count,params) \
		__wire__pException = static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pS->OMEGA_CONCAT(name,_Safe)( OMEGA_EMIT_PARAMS_VOID(param_count,params) ); \
		if (__wire__pException) return __wire__pException; \
		size_t __wire_unpack_count = 0; OMEGA_UNUSED_ARG(__wire_unpack_count); \
		OMEGA_WRITE_PARAMS_WIRE_STUB(param_count,params) \
		if (__wire__pException) goto __wire_Cleanup; return 0; \
		__wire_Cleanup: \
		IException_Safe* __wire__pException2 = 0; OMEGA_UNUSED_ARG(__wire__pException2); \
		OMEGA_UNPACK_PARAMS_WIRE_STUB(param_count,params) \
		return __wire__pException; \
	}

#define OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	static IException_Safe* OMEGA_CONCAT(name,_Wire)(void* __wire__pParam, IMessage_Safe* __wire__pParamsIn, IMessage_Safe* __wire__pParamsOut) \
	{ \
		OMEGA_UNUSED_ARG(__wire__pParam); OMEGA_UNUSED_ARG(__wire__pParamsIn); OMEGA_UNUSED_ARG(__wire__pParamsOut); \
		OMEGA_DECLARE_PARAMS_WIRE_STUB(param_count,params) \
		IException_Safe* __wire__pException = 0; \
		OMEGA_READ_PARAMS_WIRE_STUB(param_count,params) \
		marshal_info<ret_type>::wire_type::type OMEGA_CONCAT(name,_RetVal) = default_value<marshal_info<ret_type>::wire_type::type>::value(); \
		__wire__pException = static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pS->OMEGA_CONCAT(name,_Safe) ( &OMEGA_CONCAT(name,_RetVal) OMEGA_EMIT_PARAMS(param_count,params) ); \
		if (__wire__pException) return __wire__pException; \
		size_t __wire_unpack_count = 0; OMEGA_UNUSED_ARG(__wire_unpack_count); \
		OMEGA_WRITE_PARAMS_WIRE_STUB(param_count,params) \
		__wire__pException = marshal_info<ret_type>::wire_type::write(L"$retval",static_cast<IObject_Stub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut,OMEGA_CONCAT(name,_RetVal)); \
		if (__wire__pException) goto __wire_Cleanup; return 0; \
		__wire_Cleanup: \
		IException_Safe* __wire__pException2 = 0; OMEGA_UNUSED_ARG(__wire__pException2); \
		OMEGA_UNPACK_PARAMS_WIRE_STUB(param_count,params) \
		return __wire__pException; \
	}

#define OMEGA_DEFINE_WIRE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DEFINE_WIRE_STUB_,method)

#define OMEGA_DEFINE_WIRE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DEFINE_WIRE_STUB_METHOD,methods,0)

#define OMEGA_DECLARE_STUB(unique,n_space,name,methods) \
	template <class I, class Base> \
	class OMEGA_CONCAT_R(unique,_SafeStub) : public Base \
	{ \
	public: \
		OMEGA_CONCAT_R(unique,_SafeStub)(SafeStub* pStub, I* pI) : Base(pStub,pI) \
		{ } \
		virtual IException_Safe* Internal_QueryInterface_Safe(bool bRecurse, const guid_t* piid, IObject_Safe** ppObjS) \
		{ \
			if (*piid == OMEGA_GUIDOF(n_space::name)) \
			{ \
				*ppObjS = static_cast<interface_info<n_space::name>::safe_class*>(this); \
				(*ppObjS)->AddRef_Safe(); \
				return 0; \
			} \
			return Base::Internal_QueryInterface_Safe(bRecurse,piid,ppObjS); \
		} \
		OMEGA_DECLARE_SAFE_STUB_METHODS(methods) \
	};

#define OMEGA_DECLARE_WIRE_STUB(unique,n_space,name,methods) \
	template <class I, class Base> \
	class OMEGA_CONCAT_R(unique,_Stub) : public Base \
	{ \
	public: \
		OMEGA_CONCAT_R(unique,_Stub)(IStubController_Safe* pController, IMarshaller_Safe* pManager, IObject_Safe* pObj) : Base(pController,pManager,pObj) \
		{} \
		virtual IException_Safe* OMEGA_CALL SupportsInterface_Safe(bool_t* pbSupports, const guid_t* piid) \
		{ \
			if (*piid == OMEGA_GUIDOF(n_space::name)) \
			{ \
				*pbSupports = true; \
				return 0; \
			} \
			return Base::SupportsInterface_Safe(pbSupports,piid); \
		} \
		virtual IException_Safe* Internal_Invoke_Safe(uint32_t method_id, IMessage_Safe* pParamsIn, IMessage_Safe* pParamsOut) \
		{ \
			static const typename Base::MethodTableEntry MethodTable[] = \
			{ \
				OMEGA_DECLARE_WIRE_STUB_METHODS(methods) \
			}; \
			if (method_id >= Base::MethodCount && method_id < MethodCount) \
				return MethodTable[method_id - Base::MethodCount](this,pParamsIn,pParamsOut); \
			else \
				return Base::Internal_Invoke_Safe(method_id,pParamsIn,pParamsOut); \
		} \
		static const uint32_t MethodCount = Base::MethodCount + OMEGA_SEQUENCE_SIZEOF(methods); \
		OMEGA_DEFINE_WIRE_STUB_METHODS(methods) \
	private: \
		OMEGA_CONCAT_R(unique,_Stub)() {}; \
		OMEGA_CONCAT_R(unique,_Stub)(const OMEGA_CONCAT_R(unique,_Stub)&) {}; \
		OMEGA_CONCAT_R(unique,_Stub)& operator =(const OMEGA_CONCAT_R(unique,_Stub)&) {}; \
	};

#define OMEGA_DECLARE_PARAM_SAFE_PROXY_I(meta,t,name) \
	Omega::System::MetaInfo::marshal_info<t>::safe_type::coerce(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DECLARE_PARAM_SAFE_PROXY_VOID(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_SAFE_PROXY_I param

#define OMEGA_DECLARE_PARAMS_SAFE_PROXY_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_PROXY_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_SAFE_PROXY(index,param,d) \
	, OMEGA_DECLARE_PARAM_SAFE_PROXY_I param

#define OMEGA_DECLARE_PARAMS_SAFE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_PROXY,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	void name(OMEGA_DECLARE_PARAMS(param_count,params) ) \
	{ \
		IException_Safe* OMEGA_CONCAT(name,_Exception) = this->m_pS->OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_PROXY_VOID(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
	}

#define OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	ret_type name(OMEGA_DECLARE_PARAMS(param_count,params) ) \
	{ \
		ret_type OMEGA_CONCAT(name,_RetVal) = Omega::System::MetaInfo::default_value<ret_type>::value(); \
		IException_Safe* OMEGA_CONCAT(name,_Exception) = this->m_pS->OMEGA_CONCAT(name,_Safe)( \
			marshal_info<ret_type&>::safe_type::coerce(OMEGA_CONCAT(name,_RetVal)) \
			OMEGA_DECLARE_PARAMS_SAFE_PROXY(param_count,params) ); \
		if (OMEGA_CONCAT(name,_Exception)) throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		return OMEGA_CONCAT(name,_RetVal); \
	}

#define OMEGA_DECLARE_SAFE_PROXY_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_PROXY_,method)

#define OMEGA_DECLARE_SAFE_PROXY_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_PROXY_METHOD,methods,0)

// Add extra meta info types here
#define OMEGA_WIRE_READ_PROXY_PARAM_in(t,name)        no_op(false
#define OMEGA_WIRE_READ_PROXY_PARAM_in_out(t,name)    read(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),this->m_pManager,__wire__pParamsIn,name
#define OMEGA_WIRE_READ_PROXY_PARAM_out(t,name)       read(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),this->m_pManager,__wire__pParamsIn,name
#define OMEGA_WIRE_READ_PROXY_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_READ_PROXY_PARAM_II
#define OMEGA_WIRE_READ_PROXY_PARAM_size_is(size)     ,size OMEGA_WIRE_READ_PROXY_PARAM_II
#define OMEGA_WIRE_READ_PROXY_PARAM_II(t,name)

#define OMEGA_WIRE_READ_PROXY_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_READ_PROXY_PARAM_,meta) d

#define OMEGA_WIRE_READ_PROXY_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_READ_PROXY_PARAM_I,meta,(type,name))

#define OMEGA_READ_PARAM_WIRE_PROXY_I(meta,t,name) \
	marshal_info<t>::wire_type:: OMEGA_WIRE_READ_PROXY_PARAM(meta,t,name)

#define OMEGA_READ_PARAM_WIRE_PROXY(index,param,d) \
	__wire__pException = OMEGA_READ_PARAM_WIRE_PROXY_I param ); if (__wire__pException) return __wire__pException;

#define OMEGA_READ_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_READ_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_WRITE_PROXY_PARAM_in(t,name)        write(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),this->m_pManager,__wire__pParamsOut,name
#define OMEGA_WIRE_WRITE_PROXY_PARAM_in_out(t,name)    write(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),this->m_pManager,__wire__pParamsOut,name
#define OMEGA_WIRE_WRITE_PROXY_PARAM_out(t,name)       no_op(false
#define OMEGA_WIRE_WRITE_PROXY_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_WRITE_PROXY_PARAM_II
#define OMEGA_WIRE_WRITE_PROXY_PARAM_size_is(size)     ,size OMEGA_WIRE_WRITE_PROXY_PARAM_II
#define OMEGA_WIRE_WRITE_PROXY_PARAM_II(t,name)

#define OMEGA_WIRE_WRITE_PROXY_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_WRITE_PROXY_PARAM_,meta) d

#define OMEGA_WIRE_WRITE_PROXY_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_WRITE_PROXY_PARAM_I,meta,(type,name))

#define OMEGA_WRITE_PARAM_WIRE_PROXY_I(meta,t,name) \
	marshal_info<t>::wire_type:: OMEGA_WIRE_WRITE_PROXY_PARAM(meta,t,name)

#define OMEGA_WRITE_PARAM_WIRE_PROXY(index,param,d) \
	__wire__pException = OMEGA_WRITE_PARAM_WIRE_PROXY_I param ); if (__wire__pException) goto __wire_Cleanup; __wire_unpack_count = index + 1;

#define OMEGA_WRITE_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_WRITE_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_in(t,name)        no_op(false
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_in_out(t,name)    unpack(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),this->m_pManager,__wire__pParamsOut,name
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_out(t,name)       unpack(OMEGA_WIDEN_STRING(OMEGA_STRINGIZE(name)),this->m_pManager,__wire__pParamsOut,name
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_UNPACK_PROXY_PARAM_II
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_size_is(size)     ,size OMEGA_WIRE_UNPACK_PROXY_PARAM_II
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_II(t,name)

#define OMEGA_WIRE_UNPACK_PROXY_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_UNPACK_PROXY_PARAM_,meta) d

#define OMEGA_WIRE_UNPACK_PROXY_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_UNPACK_PROXY_PARAM_I,meta,(type,name))

#define OMEGA_UNPACK_PARAM_WIRE_PROXY_I(meta,t,name) \
	marshal_info<t>::wire_type:: OMEGA_WIRE_UNPACK_PROXY_PARAM(meta,t,name)

#define OMEGA_UNPACK_PARAM_WIRE_PROXY(index,param,d) \
	if (__wire_unpack_count > index) { __wire__pException2 = OMEGA_UNPACK_PARAM_WIRE_PROXY_I param ); if (__wire__pException2) { __wire__pException->Release_Safe(); return __wire__pException2; } }

#define OMEGA_UNPACK_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_UNPACK_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params)) \
	{ \
		auto_iface_safe_ptr<IMessage_Safe> __wire__pParamsOut; \
		IException_Safe* __wire__pException = this->CreateMessage(__wire__pParamsOut); \
		if (__wire__pException) return __wire__pException; \
		__wire__pException = __wire__pParamsOut->WriteStructStart_Safe(L"ipc_request",L"$ipc_request_type"); \
		if (__wire__pException) return __wire__pException; \
		__wire__pException = this->WriteKey(__wire__pParamsOut,this_iid()); \
		if (__wire__pException) return __wire__pException; \
		__wire__pException = wire_write(L"$method_id",__wire__pParamsOut,OMEGA_CONCAT(name,_MethodId)); \
		if (__wire__pException) return __wire__pException; \
		auto_iface_safe_ptr<IMessage_Safe> __wire__pParamsIn; \
		IException_Safe* __wire__pException2 = 0; \
		size_t __wire_unpack_count = 0; OMEGA_UNUSED_ARG(__wire_unpack_count); \
		OMEGA_WRITE_PARAMS_WIRE_PROXY(param_count,params) \
		__wire__pException = __wire__pParamsOut->WriteStructEnd_Safe(L"ipc_request"); \
		if (__wire__pException) goto __wire_Cleanup; \
		__wire__pException = this->SendAndReceive(__wire__pException2,attribs,__wire__pParamsOut,__wire__pParamsIn,timeout); \
		if (__wire__pException) goto __wire_Cleanup; \
		if (__wire__pException2) return __wire__pException2; \
		OMEGA_READ_PARAMS_WIRE_PROXY(param_count,params) \
		return 0; \
		__wire_Cleanup: \
		uint32_t key1; guid_t key2; \
		__wire__pException2 = __wire__pParamsOut->ReadStructStart_Safe(L"ipc_request",L"$ipc_request_type"); if (__wire__pException2) return __wire__pException2; \
		__wire__pException2 = wire_read(L"$stub_id",__wire__pParamsOut,key1); if (__wire__pException2) { __wire__pException->Release_Safe(); return __wire__pException2; } \
		__wire__pException2 = wire_read(L"$iid",__wire__pParamsOut,key2); if (__wire__pException2) { __wire__pException->Release_Safe(); return __wire__pException2; } \
		__wire__pException2 = wire_read(L"$method_id",__wire__pParamsOut,key1); if (__wire__pException2) { __wire__pException->Release_Safe(); return __wire__pException2; } \
		OMEGA_UNPACK_PARAMS_WIRE_PROXY(param_count,params) \
		return __wire__pException; \
	} \
	static const uint32_t OMEGA_CONCAT(name,_MethodId) = Base::MethodCount +

#define OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(marshal_info<ret_type&>::safe_type::type OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params)) \
	{ \
		auto_iface_safe_ptr<IMessage_Safe> __wire__pParamsOut; \
		IException_Safe* __wire__pException = this->CreateMessage(__wire__pParamsOut); \
		if (__wire__pException) return __wire__pException; \
		__wire__pException = __wire__pParamsOut->WriteStructStart_Safe(L"ipc_request",L"$ipc_request_type"); \
		if (__wire__pException) return __wire__pException; \
		__wire__pException = this->WriteKey(__wire__pParamsOut,this_iid()); \
		if (__wire__pException) return __wire__pException; \
		__wire__pException = wire_write(L"$method_id",__wire__pParamsOut,OMEGA_CONCAT(name,_MethodId)); \
		if (__wire__pException) return __wire__pException; \
		auto_iface_safe_ptr<IMessage_Safe> __wire__pParamsIn; \
		IException_Safe* __wire__pException2 = 0; \
		size_t __wire_unpack_count = 0; OMEGA_UNUSED_ARG(__wire_unpack_count); \
		OMEGA_WRITE_PARAMS_WIRE_PROXY(param_count,params) \
		__wire__pException = __wire__pParamsOut->WriteStructEnd_Safe(L"ipc_request"); \
		if (__wire__pException) goto __wire_Cleanup; \
		__wire__pException = this->SendAndReceive(__wire__pException2,attribs,__wire__pParamsOut,__wire__pParamsIn,timeout); \
		if (__wire__pException) goto __wire_Cleanup; \
		if (__wire__pException2) return __wire__pException2; \
		OMEGA_READ_PARAMS_WIRE_PROXY(param_count,params) \
		return marshal_info<ret_type>::wire_type::read(L"$retval",this->m_pManager,__wire__pParamsIn,*OMEGA_CONCAT(name,_RetVal)); \
		__wire_Cleanup: \
		uint32_t key1; guid_t key2; \
		__wire__pException2 = __wire__pParamsOut->ReadStructStart_Safe(L"ipc_request",L"$ipc_request_type"); if (__wire__pException2) return __wire__pException2; \
		__wire__pException2 = wire_read(L"$stub_id",__wire__pParamsOut,key1); if (__wire__pException2) return __wire__pException2; \
		__wire__pException2 = wire_read(L"$iid",__wire__pParamsOut,key2); if (__wire__pException2) return __wire__pException2; \
		__wire__pException2 = wire_read(L"$method_id",__wire__pParamsOut,key1); if (__wire__pException2) return __wire__pException2; \
		OMEGA_UNPACK_PARAMS_WIRE_PROXY(param_count,params) \
		return __wire__pException; \
	} \
	static const uint32_t OMEGA_CONCAT(name,_MethodId) = Base::MethodCount +

#define OMEGA_DECLARE_WIRE_PROXY_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_WIRE_PROXY_,method) index;

#define OMEGA_DECLARE_WIRE_PROXY_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_WIRE_PROXY_METHOD,methods,0) \
	static const uint32_t MethodCount = Base::MethodCount + OMEGA_SEQUENCE_SIZEOF(methods);

#define OMEGA_DECLARE_PROXY(unique,n_space,name,methods) \
	template <class I, class Base> \
	class OMEGA_CONCAT_R(unique,_SafeProxy) : public Base \
	{ \
	public: \
		OMEGA_CONCAT_R(unique,_SafeProxy)(IObject* pOuter, typename interface_info<I>::safe_class* pS) : Base(pOuter,pS) \
		{ } \
		virtual IObject* Internal_QueryInterface(bool bRecurse, const guid_t& iid) \
		{ \
			if (iid == OMEGA_GUIDOF(n_space::name)) \
			{ \
				this->AddRef(); \
				return static_cast<n_space::name*>(this); \
			} \
			return Base::Internal_QueryInterface(bRecurse,iid); \
		} \
		OMEGA_DECLARE_SAFE_PROXY_METHODS(methods) \
	private: \
		OMEGA_CONCAT_R(unique,_SafeProxy)(const OMEGA_CONCAT_R(unique,_SafeProxy)&) {}; \
		OMEGA_CONCAT_R(unique,_SafeProxy)& operator = (const OMEGA_CONCAT_R(unique,_SafeProxy)&) {}; \
	};

#define OMEGA_DECLARE_WIRE_PROXY(unique,n_space,name,methods) \
	template <class Base> \
	class OMEGA_CONCAT_R(unique,_Proxy) : public Base \
	{ \
	public: \
		OMEGA_CONCAT_R(unique,_Proxy)(IProxy_Safe* pProxy, IMarshaller_Safe* pManager) : Base(pProxy,pManager) \
		{ } \
		virtual IException_Safe* Internal_QueryInterface_Safe(bool bRecurse, const guid_t* piid, IObject_Safe** ppS) \
		{ \
			if (*piid == OMEGA_GUIDOF(n_space::name)) \
			{ \
				*ppS = static_cast<interface_info<n_space::name>::safe_class*>(this); \
				(*ppS)->AddRef_Safe(); \
				return 0; \
			} \
			return Base::Internal_QueryInterface_Safe(bRecurse,piid,ppS); \
		} \
		OMEGA_DECLARE_WIRE_PROXY_METHODS(methods) \
	private: \
		OMEGA_CONCAT_R(unique,_Proxy)(const OMEGA_CONCAT_R(unique,_Proxy)&) {}; \
		OMEGA_CONCAT_R(unique,_Proxy)& operator = (const OMEGA_CONCAT_R(unique,_Proxy)&) {}; \
		const guid_t& this_iid() const { return OMEGA_GUIDOF(n_space::name); } \
	};

#define OMEGA_DEFINE_INTERNAL_INTERFACE(n_space,name,methods) \
	OMEGA_DECLARE_SAFE(name,methods,name) \
	OMEGA_DECLARE_TYPE(name,n_space,methods,Omega,IObject) \
	OMEGA_DECLARE_STUB(name,n_space,name,methods) \
	OMEGA_DECLARE_PROXY(name,n_space,name,methods)

#define OMEGA_DEFINE_INTERNAL_INTERFACE_PART2(n_space,name,methods) \
	OMEGA_DECLARE_WIRE_STUB(name,n_space,name,methods) \
	OMEGA_DECLARE_WIRE_PROXY(name,n_space,name,methods) \
	OMEGA_WIRE_MAGIC(n_space,name)

#define OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL(n_space,name,d_space,derived,guid,methods) \
	OMEGA_SET_GUIDOF(n_space,name,guid) \
	namespace Omega { namespace System { namespace MetaInfo { \
	OMEGA_DECLARE_FORWARDS(OMEGA_UNIQUE_NAME(name),n_space,name,d_space,derived) \
	OMEGA_DECLARE_SAFE(OMEGA_UNIQUE_NAME(name),methods,name) \
	OMEGA_DECLARE_TYPE(name,n_space,methods,d_space,derived) \
	OMEGA_DECLARE_STUB(OMEGA_UNIQUE_NAME(name),n_space,name,methods) \
	OMEGA_DECLARE_PROXY(OMEGA_UNIQUE_NAME(name),n_space,name,methods) \
	OMEGA_QI_MAGIC(n_space,name) \
	} } }

#define OMEGA_DEFINE_INTERFACE_LOCAL(n_space,name,guid,methods) \
	OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL(n_space,name,Omega,IObject,guid,methods)

#define OMEGA_DEFINE_INTERFACE_DERIVED(n_space,name,d_space,derived,guid,methods) \
	OMEGA_SET_GUIDOF(n_space,name,guid) \
	namespace Omega { namespace System { namespace MetaInfo { \
	OMEGA_DECLARE_FORWARDS(OMEGA_UNIQUE_NAME(name),n_space,name,d_space,derived) \
	OMEGA_DECLARE_SAFE(OMEGA_UNIQUE_NAME(name),methods,name) \
	OMEGA_DECLARE_TYPE(name,n_space,methods,d_space,derived) \
	OMEGA_DECLARE_STUB(OMEGA_UNIQUE_NAME(name),n_space,name,methods) \
	OMEGA_DECLARE_PROXY(OMEGA_UNIQUE_NAME(name),n_space,name,methods) \
	OMEGA_QI_MAGIC(n_space,name) \
	OMEGA_DECLARE_WIRE_STUB(OMEGA_UNIQUE_NAME(name),n_space,name,methods) \
	OMEGA_DECLARE_WIRE_PROXY(OMEGA_UNIQUE_NAME(name),n_space,name,methods) \
	OMEGA_WIRE_MAGIC(n_space,name) \
	} } }

#define OMEGA_DEFINE_INTERFACE(n_space,name,guid,methods) \
	OMEGA_DEFINE_INTERFACE_DERIVED(n_space,name,Omega,IObject,guid,methods)

#define OMEGA_METHOD_VOID(name,param_count,params) \
	(DECLARED_METHOD_VOID(TypeInfo::Synchronous,0,name,param_count,params))

#define OMEGA_METHOD(ret_type,name,param_count,params) \
	(DECLARED_METHOD(TypeInfo::Synchronous,0,ret_type,name,param_count,params))

#define OMEGA_METHOD_EX_VOID(attribs,timeout,name,param_count,params) \
	(DECLARED_METHOD_VOID(TypeInfo::attribs,timeout,name,param_count,params))

#define OMEGA_METHOD_EX(attribs,timeout,ret_type,name,param_count,params) \
	(DECLARED_METHOD(TypeInfo::attribs,timeout,ret_type,name,param_count,params))

#define OMEGA_EXPORTED_FUNCTION_VOID_IMPL(name,param_count,params) \
	extern "C" OMEGA_IMPORT Omega::System::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params)); \
	inline void name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		Omega::System::MetaInfo::IException_Safe* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_PROXY_VOID(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) Omega::System::MetaInfo::throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
	}

#define OMEGA_EXPORTED_FUNCTION_IMPL(ret_type,name,param_count,params) \
	extern "C" OMEGA_IMPORT Omega::System::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)( \
		Omega::System::MetaInfo::marshal_info<ret_type&>::safe_type::type OMEGA_CONCAT(name,_RetVal) \
		OMEGA_DECLARE_PARAMS_SAFE(param_count,params)); \
	inline ret_type name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		ret_type OMEGA_CONCAT(name,_RetVal) = Omega::System::MetaInfo::default_value<ret_type>::value(); \
		Omega::System::MetaInfo::IException_Safe* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Safe)( \
			Omega::System::MetaInfo::marshal_info<ret_type&>::safe_type::coerce(OMEGA_CONCAT(name,_RetVal)) \
			OMEGA_DECLARE_PARAMS_SAFE_PROXY(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) Omega::System::MetaInfo::throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		return OMEGA_CONCAT(name,_RetVal); \
	}

#define OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params) \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	inline void name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		OMEGA_CONCAT(name,_Impl)(OMEGA_EMIT_PARAMS_VOID(param_count,params)); \
	}

#define OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params) \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	inline ret_type name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		return OMEGA_CONCAT(name,_Impl)(OMEGA_EMIT_PARAMS_VOID(param_count,params)); \
	}

#define OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	extern "C" OMEGA_EXPORT Omega::System::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params)) \
	{ \
		try \
		{ \
			OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_SAFE_STUB(param_count,params)); \
			return 0; \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::System::MetaInfo::return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	} \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params))

#define OMEGA_DEFINE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	extern "C" OMEGA_EXPORT Omega::System::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(Omega::System::MetaInfo::marshal_info<ret_type&>::safe_type::type OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params)) \
	{ \
		try \
		{ \
			static_cast<ret_type&>(Omega::System::MetaInfo::marshal_info<ret_type&>::safe_type::coerce(OMEGA_CONCAT(name,_RetVal))) = OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_SAFE_STUB(param_count,params)); \
			return 0; \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::System::MetaInfo::return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	} \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params))

#else // defined(DOXYGEN)

#endif // defined(DOXYGEN)

#endif // OOCORE_MACROS_H_INCLUDED_

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

#include "cpp/sequence.h"
#include "cpp/repeat.h"
#include "cpp/comma.h"
#include "cpp/split.h"

#define OMEGA_UNIQUE_NAME(name) \
	OMEGA_CONCAT(name,__LINE__)

#define OMEGA_QI_MAGIC(n_space,iface) \
	class OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_RttiInit) \
	{ \
	public: \
		OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_RttiInit)() \
		{ \
			static const qi_rtti s_rtti = \
			{ \
				&Safe_Proxy<n_space::iface,n_space::iface >::bind, \
				&Safe_Stub<n_space::iface >::create \
			}; \
			register_rtti_info(OMEGA_GUIDOF(n_space::iface),&s_rtti); \
			register_typeinfo(OMEGA_GUIDOF(n_space::iface),OMEGA_WIDEN_STRINGIZE(n_space::iface),typeinfo_holder<n_space::iface >::get_type_info()); \
		} \
		~OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_RttiInit)() \
		{ \
			unregister_typeinfo(OMEGA_GUIDOF(n_space::iface),typeinfo_holder<n_space::iface >::get_type_info()); \
		} \
	}; \
	OMEGA_WEAK_VARIABLE(OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_RttiInit),OMEGA_CONCAT_R(OMEGA_CONCAT(OMEGA_MODULE_PRIVATE_NAME,_RttiInit_),OMEGA_UNIQUE_NAME(iface)));
	
#define OMEGA_WIRE_MAGIC(n_space,iface) \
	class OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_WireInit) \
	{ \
	public: \
		OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_WireInit)() \
		{ \
			static const wire_rtti s_rtti = \
			{ \
				&Wire_Proxy<n_space::iface,n_space::iface >::bind, \
				&Wire_Stub<n_space::iface >::create, \
			}; \
			register_wire_rtti_info(OMEGA_GUIDOF(n_space::iface),&s_rtti); \
		} \
	}; \
	OMEGA_WEAK_VARIABLE(OMEGA_CONCAT_R(OMEGA_UNIQUE_NAME(iface),_WireInit),OMEGA_CONCAT_R(OMEGA_CONCAT(OMEGA_MODULE_PRIVATE_NAME,_WireInit_),OMEGA_UNIQUE_NAME(iface)));
	
#define OMEGA_DECLARE_PARAM_I(meta,type,name) \
	type name

#define OMEGA_DECLARE_PARAM_VOID(index,params,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_I params

#define OMEGA_DECLARE_PARAMS_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM(index,params,d) \
	, OMEGA_DECLARE_PARAM_I params

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
	Omega::System::Internal::marshal_info<t >::safe_type::type name

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
#define OMEGA_PS_PARAM_size_is(size)     ,size OMEGA_PS_PARAM_II
#define OMEGA_PS_PARAM_II(t,name)

#define OMEGA_PS_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_PS_PARAM_,meta) d

#define OMEGA_PS_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_PS_PARAM_I,meta,(type,name))

#define OMEGA_DECLARE_SAFE_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	const SafeShim* (OMEGA_CALL* OMEGA_CONCAT_R2(pfn,OMEGA_CONCAT(name,_Safe)))(const SafeShim* OMEGA_DECLARE_PARAMS_SAFE(param_count,params) );

#define OMEGA_DECLARE_SAFE_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	const SafeShim* (OMEGA_CALL* OMEGA_CONCAT_R2(pfn,OMEGA_CONCAT(name,_Safe)))(const SafeShim*, marshal_info<ret_type&>::safe_type::type OMEGA_DECLARE_PARAMS_SAFE(param_count,params) );

#define OMEGA_DECLARE_SAFE_DECLARED_NO_METHODS()

#define OMEGA_DECLARE_SAFE_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_,method)

#define OMEGA_DECLARE_SAFE_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_METHOD,methods,0)

#define OMEGA_DECLARE_FORWARDS(n_space,name) \
	template <> struct custom_safe_type<n_space::name*> \
	{ \
		typedef iface_safe_type<n_space::name > impl; \
	}; \
	template <> struct custom_wire_type<n_space::name*> \
	{ \
		typedef iface_wire_type<n_space::name > impl; \
	}; \
	template <> struct type_kind<n_space::name> \
	{ \
		static const type_holder* type() \
		{ \
			static const type_holder t = { TypeInfo::typeObject, (const type_holder*)(&OMEGA_GUIDOF(n_space::name)) }; \
			return &t; \
		} \
	};

#define OMEGA_DECLARE_SAFE(unique,methods,n_space,name,d_space,derived) \
	struct OMEGA_CONCAT_R(unique,_Safe_VTable) \
	{ \
		vtable_info<d_space::derived >::type base_vtable; \
		OMEGA_DECLARE_SAFE_METHODS(methods) \
	}; \
	template <> struct vtable_info<n_space::name > \
	{ \
		typedef OMEGA_CONCAT_R(unique,_Safe_VTable) type; \
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

// Add extra meta info types here
#define OMEGA_DECLARE_TYPE_ATTR_in(t,name)        L""
#define OMEGA_DECLARE_TYPE_ATTR_in_out(t,name)    L""
#define OMEGA_DECLARE_TYPE_ATTR_out(t,name)       L""
#define OMEGA_DECLARE_TYPE_ATTR_iid_is(iid)       OMEGA_WIDEN_STRINGIZE(iid) OMEGA_DECLARE_TYPE_ATTR_II
#define OMEGA_DECLARE_TYPE_ATTR_size_is(size)     OMEGA_WIDEN_STRINGIZE(size) OMEGA_DECLARE_TYPE_ATTR_II
#define OMEGA_DECLARE_TYPE_ATTR_II(t,name)

#define OMEGA_DECLARE_TYPE_PARAM_III(index,meta,d) \
	OMEGA_CONCAT(OMEGA_DECLARE_TYPE_ATTR_,meta) d

#define OMEGA_DECLARE_TYPE_PARAM_I(meta,t,name) \
	{ \
		OMEGA_WIDEN_STRINGIZE(name),type_kind<t >::type(), \
		OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_DECLARE_TYPE_PARAM_II,meta,(t,name)), \
		OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_DECLARE_TYPE_PARAM_III,meta,(t,name)) \
	},

#define OMEGA_DECLARE_TYPE_PARAM(index,params,d) \
	OMEGA_DECLARE_TYPE_PARAM_I params

#define OMEGA_DECLARE_TYPE_PARAMS(param_count,params) \
	static const typeinfo_rtti::ParamInfo pi[] = { \
		OMEGA_TUPLE_FOR_EACH(param_count,OMEGA_DECLARE_TYPE_PARAM,OMEGA_SPLIT_3(param_count,params),0) \
		{ 0, 0, 0, L"" } }; \
	return pi;

#define OMEGA_DECLARE_TYPE_PARAM_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	OMEGA_DECLARE_TYPE_PARAMS(param_count,params)

#define OMEGA_DECLARE_TYPE_PARAM_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	OMEGA_DECLARE_TYPE_PARAMS(param_count,params)

#define OMEGA_DECLARE_TYPE_PARAM_DECLARED_NO_METHODS() return 0;

#define OMEGA_DECLARE_TYPE_METHOD_PARAM(index,method,d) \
	static const typeinfo_rtti::ParamInfo* OMEGA_CONCAT_R(method_param_,index)() \
	{ OMEGA_CONCAT_R(OMEGA_DECLARE_TYPE_PARAM_,method) }

#define OMEGA_DECLARE_TYPE_METHOD_PARAMS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_TYPE_METHOD_PARAM,methods,0)

#define OMEGA_DECLARE_TYPE_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	{ OMEGA_WIDEN_STRINGIZE(name),attribs,timeout,param_count,type_kind<void>::type()

#define OMEGA_DECLARE_TYPE_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	{ OMEGA_WIDEN_STRINGIZE(name),attribs,timeout,param_count,type_kind<ret_type >::type()

#define OMEGA_DECLARE_TYPE_DECLARED_NO_METHODS() \
	{ L"",0,0,0,0

#define OMEGA_DECLARE_TYPE_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_TYPE_,method) ,&OMEGA_CONCAT_R(method_param_,index) },

#define OMEGA_DECLARE_TYPE_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_TYPE_METHOD,methods,0)

#define OMEGA_DECLARE_TYPE(n_space,name,methods,d_space,derived) \
	template <> class typeinfo_holder<n_space::name > \
	{ \
	public: \
		static const typeinfo_rtti* get_type_info() \
		{ \
			static const typeinfo_rtti ti = { &method_info, method_count, &OMEGA_GUIDOF(d_space::derived) }; \
			return &ti; \
		}; \
		static const uint32_t method_count = OMEGA_SEQUENCE_SIZEOF(methods); \
	private: \
		OMEGA_DECLARE_TYPE_METHOD_PARAMS(methods) \
		static const typeinfo_rtti::MethodInfo* method_info() \
		{ \
			static const typeinfo_rtti::MethodInfo method_infos[] = \
			{ \
				OMEGA_DECLARE_TYPE_METHODS(methods) \
				{ 0, 0, 0, 0, 0, 0 } \
			}; \
			return method_infos; \
		} \
	};

#define OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	, &OMEGA_CONCAT(name,_Safe)

#define OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	, &OMEGA_CONCAT(name,_Safe)

#define OMEGA_DECLARE_SAFE_STUB_DECLARED_NO_METHODS()

#define OMEGA_DECLARE_SAFE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_STUB_,method)

#define OMEGA_DECLARE_SAFE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_STUB_METHOD,methods,0)

#define OMEGA_DEFINE_SAFE_STUB_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	static const SafeShim* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(const SafeShim* OMEGA_CONCAT(name,_shim) OMEGA_DECLARE_PARAMS_SAFE(param_count,params) ) \
	{ \
		const SafeShim* OMEGA_CONCAT(name,_except) = 0; \
		try { deref_shim(OMEGA_CONCAT(name,_shim))->name( OMEGA_DEFINE_PARAMS_SAFE_STUB_VOID(param_count,params) ); } \
		catch (std::exception& OMEGA_CONCAT(name,_exception)) { OMEGA_CONCAT(name,_except) = Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(OMEGA_CONCAT(name,_exception),OMEGA_WIDEN_STRINGIZE(name))); } \
		catch (IException* OMEGA_CONCAT(name,_exception)) { OMEGA_CONCAT(name,_except) = return_safe_exception(OMEGA_CONCAT(name,_exception)); } \
		return OMEGA_CONCAT(name,_except); \
	}

#define OMEGA_DEFINE_SAFE_STUB_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	static const SafeShim* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(const SafeShim* OMEGA_CONCAT(name,_shim), marshal_info<ret_type&>::safe_type::type OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params) ) \
	{ \
		const SafeShim* OMEGA_CONCAT(name,_except) = 0; \
		try { static_cast<ret_type&>(marshal_info<ret_type&>::safe_type::coerce(OMEGA_CONCAT(name,_RetVal))) = \
			deref_shim(OMEGA_CONCAT(name,_shim))->name( OMEGA_DEFINE_PARAMS_SAFE_STUB_VOID(param_count,params) ); } \
		catch (std::exception& OMEGA_CONCAT(name,_exception)) { OMEGA_CONCAT(name,_except) = Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(OMEGA_CONCAT(name,_exception),OMEGA_WIDEN_STRINGIZE(name))); } \
		catch (IException* OMEGA_CONCAT(name,_exception)) { OMEGA_CONCAT(name,_except) = return_safe_exception(OMEGA_CONCAT(name,_exception)); } \
		return OMEGA_CONCAT(name,_except); \
	}
	
#define OMEGA_DEFINE_SAFE_STUB_DECLARED_NO_METHODS()

#define OMEGA_DEFINE_PARAM_SAFE_STUB_I(meta,t,name) \
	Omega::System::Internal::marshal_info<t >::safe_type::coerce(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DEFINE_PARAM_SAFE_STUB_VOID(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DEFINE_PARAM_SAFE_STUB_I param

#define OMEGA_DEFINE_PARAMS_SAFE_STUB_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DEFINE_PARAM_SAFE_STUB_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DEFINE_PARAM_SAFE_STUB(index,param,d) \
	, OMEGA_DEFINE_PARAM_SAFE_STUB_I param

#define OMEGA_DEFINE_PARAMS_SAFE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DEFINE_PARAM_SAFE_STUB,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DEFINE_SAFE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DEFINE_SAFE_STUB_,method)

#define OMEGA_DEFINE_SAFE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DEFINE_SAFE_STUB_METHOD,methods,0)


#define OMEGA_DECLARE_PARAM_WIRE_STUB_I(meta,t,name) \
	marshal_info<t >::wire_type::type name;

#define OMEGA_DECLARE_PARAM_WIRE_STUB(index,param,d) \
	OMEGA_DECLARE_PARAM_WIRE_STUB_I param

#define OMEGA_DECLARE_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_READ_STUB_PARAM_in(t,name)        read(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsIn__wire__,name
#define OMEGA_WIRE_READ_STUB_PARAM_in_out(t,name)    read(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsIn__wire__,name
#define OMEGA_WIRE_READ_STUB_PARAM_out(t,name)       init(name
#define OMEGA_WIRE_READ_STUB_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_READ_STUB_PARAM_II
#define OMEGA_WIRE_READ_STUB_PARAM_size_is(size)     ,size OMEGA_WIRE_READ_STUB_PARAM_II
#define OMEGA_WIRE_READ_STUB_PARAM_II(t,name)

#define OMEGA_WIRE_READ_STUB_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_READ_STUB_PARAM_,meta) d

#define OMEGA_WIRE_READ_STUB_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_READ_STUB_PARAM_I,meta,(type,name))

#define OMEGA_READ_PARAM_WIRE_STUB_I(meta,t,name) \
	marshal_info<t >::wire_type::OMEGA_WIRE_READ_STUB_PARAM(meta,t,name) );

#define OMEGA_READ_PARAM_WIRE_STUB(index,param,d) \
	OMEGA_READ_PARAM_WIRE_STUB_I param

#define OMEGA_READ_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_READ_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_WRITE_STUB_PARAM_in(t,name)        no_op(false
#define OMEGA_WIRE_WRITE_STUB_PARAM_in_out(t,name)    write(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsOut__wire__,name
#define OMEGA_WIRE_WRITE_STUB_PARAM_out(t,name)       write(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsOut__wire__,name
#define OMEGA_WIRE_WRITE_STUB_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_WRITE_STUB_PARAM_II
#define OMEGA_WIRE_WRITE_STUB_PARAM_size_is(size)     ,size OMEGA_WIRE_WRITE_STUB_PARAM_II
#define OMEGA_WIRE_WRITE_STUB_PARAM_II(t,name)

#define OMEGA_WIRE_WRITE_STUB_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_WRITE_STUB_PARAM_,meta) d

#define OMEGA_WIRE_WRITE_STUB_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_WRITE_STUB_PARAM_I,meta,(type,name))

#define OMEGA_WRITE_PARAM_WIRE_STUB_I(meta,t,name) \
	marshal_info<t >::wire_type::OMEGA_WIRE_WRITE_STUB_PARAM(meta,t,name)

#define OMEGA_WRITE_PARAM_WIRE_STUB(index,param,d) \
	OMEGA_WRITE_PARAM_WIRE_STUB_I param ); unpack_count__wire__ = index + 1;

#define OMEGA_WRITE_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_WRITE_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_UNPACK_STUB_PARAM_in(t,name)        no_op(false
#define OMEGA_WIRE_UNPACK_STUB_PARAM_in_out(t,name)    unpack(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsOut__wire__,name
#define OMEGA_WIRE_UNPACK_STUB_PARAM_out(t,name)       unpack(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsOut__wire__,name
#define OMEGA_WIRE_UNPACK_STUB_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_UNPACK_STUB_PARAM_II
#define OMEGA_WIRE_UNPACK_STUB_PARAM_size_is(size)     ,size OMEGA_WIRE_UNPACK_STUB_PARAM_II
#define OMEGA_WIRE_UNPACK_STUB_PARAM_II(t,name)

#define OMEGA_WIRE_UNPACK_STUB_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_UNPACK_STUB_PARAM_,meta) d

#define OMEGA_WIRE_UNPACK_STUB_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_UNPACK_STUB_PARAM_I,meta,(type,name))

#define OMEGA_UNPACK_PARAM_WIRE_STUB_I(meta,t,name) \
	marshal_info<t >::wire_type::OMEGA_WIRE_UNPACK_STUB_PARAM(meta,t,name)

#define OMEGA_UNPACK_PARAM_WIRE_STUB(index,param,d) \
	if (unpack_count__wire__ > index) { OMEGA_UNPACK_PARAM_WIRE_STUB_I param ); }

#define OMEGA_UNPACK_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_UNPACK_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_WIRE_STUB_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	&OMEGA_CONCAT(name,_Wire)

#define OMEGA_DECLARE_WIRE_STUB_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	&OMEGA_CONCAT(name,_Wire)

#define OMEGA_DECLARE_WIRE_STUB_DECLARED_NO_METHODS() \
	0 	

#define OMEGA_DECLARE_WIRE_STUB_METHOD(index,method,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_CONCAT_R(OMEGA_DECLARE_WIRE_STUB_,method)

#define OMEGA_DECLARE_WIRE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_WIRE_STUB_METHOD,methods,0)

#define OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	static void OMEGA_CONCAT(name,_Wire)(Wire_Stub_Base* pThis__wire__, Remoting::IMessage* pParamsIn__wire__, Remoting::IMessage* pParamsOut__wire__) \
	{ \
		auto_iface_ptr<Remoting::IMarshaller> ptrMarshaller__wire__ = pThis__wire__->GetMarshaller(); \
		OMEGA_UNUSED_ARG(ptrMarshaller__wire__); OMEGA_UNUSED_ARG(pThis__wire__); OMEGA_UNUSED_ARG(pParamsIn__wire__); OMEGA_UNUSED_ARG(pParamsOut__wire__); \
		OMEGA_DECLARE_PARAMS_WIRE_STUB(param_count,params) \
		OMEGA_READ_PARAMS_WIRE_STUB(param_count,params) \
		size_t unpack_count__wire__ = 0; OMEGA_UNUSED_ARG(unpack_count__wire__); \
		try { pThis__wire__->get_iface<iface >()->name( OMEGA_EMIT_PARAMS_VOID(param_count,params) ); \
		OMEGA_WRITE_PARAMS_WIRE_STUB(param_count,params) } \
		catch (...) { OMEGA_UNPACK_PARAMS_WIRE_STUB(param_count,params) throw; } \
	}

#define OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	static void OMEGA_CONCAT(name,_Wire)(Wire_Stub_Base* pThis__wire__, Remoting::IMessage* pParamsIn__wire__, Remoting::IMessage* pParamsOut__wire__) \
	{ \
		auto_iface_ptr<Remoting::IMarshaller> ptrMarshaller__wire__ = pThis__wire__->GetMarshaller(); \
		OMEGA_UNUSED_ARG(ptrMarshaller__wire__); OMEGA_UNUSED_ARG(pThis__wire__); OMEGA_UNUSED_ARG(pParamsIn__wire__); OMEGA_UNUSED_ARG(pParamsOut__wire__); \
		OMEGA_DECLARE_PARAMS_WIRE_STUB(param_count,params) \
		OMEGA_READ_PARAMS_WIRE_STUB(param_count,params) \
		size_t unpack_count__wire__ = 0; OMEGA_UNUSED_ARG(unpack_count__wire__); \
		try { marshal_info<ret_type >::wire_type::type OMEGA_CONCAT(name,_RetVal); \
		static_cast<ret_type&>(OMEGA_CONCAT(name,_RetVal)) = pThis__wire__->get_iface<iface >()->name(OMEGA_EMIT_PARAMS_VOID(param_count,params) ); \
		OMEGA_WRITE_PARAMS_WIRE_STUB(param_count,params) \
		marshal_info<ret_type >::wire_type::write(L"$retval",ptrMarshaller__wire__,pParamsOut__wire__,OMEGA_CONCAT(name,_RetVal)); } \
		catch (...) { OMEGA_UNPACK_PARAMS_WIRE_STUB(param_count,params) throw; } \
	}

#define OMEGA_DEFINE_WIRE_STUB_DECLARED_NO_METHODS()

#define OMEGA_DEFINE_WIRE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DEFINE_WIRE_STUB_,method)

#define OMEGA_DEFINE_WIRE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DEFINE_WIRE_STUB_METHOD,methods,0)

#define OMEGA_DECLARE_STUB(n_space,name,d_space,derived,methods) \
	template <> class Safe_Stub<n_space::name > : public Safe_Stub<d_space::derived > \
	{ \
	public: \
		static const SafeShim* create(IObject* pI) \
		{ \
			Safe_Stub* pThis; OMEGA_NEW(pThis,Safe_Stub(static_cast<n_space::name*>(pI),OMEGA_GUIDOF(n_space::name))); \
			return &pThis->m_shim; \
		} \
	protected: \
		Safe_Stub(n_space::name* pI, const guid_t& iid) : Safe_Stub<d_space::derived >(pI,iid) \
		{ m_shim.m_vtable = get_vt(); } \
		static const vtable_info<n_space::name >::type* get_vt() \
		{ \
			static const vtable_info<n_space::name >::type vt = { \
				*Safe_Stub<d_space::derived >::get_vt() \
				OMEGA_DECLARE_SAFE_STUB_METHODS(methods) \
			}; return &vt; \
		} \
		virtual bool IsDerived(const guid_t& iid) const \
		{ \
			if (iid == OMEGA_GUIDOF(n_space::name)) return true; \
			return Safe_Stub<d_space::derived >::IsDerived(iid); \
		} \
	private: \
		static n_space::name* deref_shim(const SafeShim* shim) { return static_cast<n_space::name*>(static_cast<Safe_Stub*>(shim->m_stub)->m_pI); } \
		OMEGA_DEFINE_SAFE_STUB_METHODS(methods) \
	};

#define OMEGA_DECLARE_WIRE_STUB(n_space,name,d_space,derived,methods) \
	template <> \
	class Wire_Stub<n_space::name > : public Wire_Stub<d_space::derived > \
	{ \
	public: \
		static Remoting::IStub* create(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, IObject* pI) \
		{ \
			Wire_Stub* pThis; \
			OMEGA_NEW(pThis,Wire_Stub(pController,pMarshaller,pI)); \
			return pThis; \
		} \
	protected: \
		Wire_Stub(Remoting::IStubController* pController, Remoting::IMarshaller* pMarshaller, IObject* pI) : \
			Wire_Stub<d_space::derived >(pController,pMarshaller,pI) \
		{ } \
		virtual bool_t SupportsInterface(const guid_t& iid) \
		{ \
			if (iid == OMEGA_GUIDOF(n_space::name)) return true; \
			return Wire_Stub<d_space::derived >::SupportsInterface(iid); \
		} \
		virtual void Invoke(uint32_t method_id, Remoting::IMessage* pParamsIn, Remoting::IMessage* pParamsOut) \
		{ \
			static const MethodTableEntry MethodTable[] = \
			{ \
				OMEGA_DECLARE_WIRE_STUB_METHODS(methods) \
			}; \
			if (method_id >= Wire_Stub<d_space::derived >::MethodCount && method_id < MethodCount) \
				return MethodTable[method_id - Wire_Stub<d_space::derived >::MethodCount](this,pParamsIn,pParamsOut); \
			return Wire_Stub<d_space::derived >::Invoke(method_id,pParamsIn,pParamsOut); \
		} \
		static const uint32_t MethodCount = Wire_Stub<d_space::derived >::MethodCount + OMEGA_SEQUENCE_SIZEOF(methods); \
	private: \
		typedef n_space::name iface; \
		OMEGA_DEFINE_WIRE_STUB_METHODS(methods) \
	};

#define OMEGA_DECLARE_PARAM_SAFE_PROXY_I(meta,t,name) \
	Omega::System::Internal::marshal_info<t >::safe_type::coerce(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DECLARE_PARAM_SAFE_PROXY_VOID(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_SAFE_PROXY_I param

#define OMEGA_DECLARE_PARAMS_SAFE_PROXY_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_PROXY_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_SAFE_PROXY(index,param,d) \
	, OMEGA_DECLARE_PARAM_SAFE_PROXY_I param

#define OMEGA_DECLARE_PARAMS_SAFE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_PROXY,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	void name(OMEGA_DECLARE_PARAMS_VOID(param_count,params) ) \
	{ \
		const SafeShim* OMEGA_CONCAT(name,_except) = deref_vt()->OMEGA_CONCAT_R2(pfn,OMEGA_CONCAT(name,_Safe))(this->m_shim OMEGA_DECLARE_PARAMS_SAFE_PROXY(param_count,params)); \
		if (OMEGA_CONCAT(name,_except)) \
			throw_correct_exception(OMEGA_CONCAT(name,_except)); \
	}

#define OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	ret_type name(OMEGA_DECLARE_PARAMS_VOID(param_count,params) ) \
	{ \
		ret_type OMEGA_CONCAT(name,_RetVal) = default_value<ret_type >::value(); \
		const SafeShim* OMEGA_CONCAT(name,_except) = deref_vt()->OMEGA_CONCAT_R2(pfn,OMEGA_CONCAT(name,_Safe))( \
			this->m_shim,marshal_info<ret_type&>::safe_type::coerce(OMEGA_CONCAT(name,_RetVal)) \
			OMEGA_DECLARE_PARAMS_SAFE_PROXY(param_count,params)); \
		if (OMEGA_CONCAT(name,_except)) \
			throw_correct_exception(OMEGA_CONCAT(name,_except)); \
		return OMEGA_CONCAT(name,_RetVal); \
	}

#define OMEGA_DECLARE_SAFE_PROXY_DECLARED_NO_METHODS()

#define OMEGA_DECLARE_SAFE_PROXY_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_PROXY_,method)

#define OMEGA_DECLARE_SAFE_PROXY_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_PROXY_METHOD,methods,0)

// Add extra meta info types here
#define OMEGA_WIRE_READ_PROXY_PARAM_in(t,name)        no_op(false
#define OMEGA_WIRE_READ_PROXY_PARAM_in_out(t,name)    read(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsIn__wire__,name
#define OMEGA_WIRE_READ_PROXY_PARAM_out(t,name)       read(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsIn__wire__,name
#define OMEGA_WIRE_READ_PROXY_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_READ_PROXY_PARAM_II
#define OMEGA_WIRE_READ_PROXY_PARAM_size_is(size)     ,size OMEGA_WIRE_READ_PROXY_PARAM_II
#define OMEGA_WIRE_READ_PROXY_PARAM_II(t,name)

#define OMEGA_WIRE_READ_PROXY_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_READ_PROXY_PARAM_,meta) d

#define OMEGA_WIRE_READ_PROXY_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_READ_PROXY_PARAM_I,meta,(type,name))

#define OMEGA_READ_PARAM_WIRE_PROXY_I(meta,t,name) \
	marshal_info<t >::wire_type:: OMEGA_WIRE_READ_PROXY_PARAM(meta,t,name)

#define OMEGA_READ_PARAM_WIRE_PROXY(index,param,d) \
	OMEGA_READ_PARAM_WIRE_PROXY_I param );

#define OMEGA_READ_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_READ_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_WRITE_PROXY_PARAM_in(t,name)        write(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsOut__wire__,name
#define OMEGA_WIRE_WRITE_PROXY_PARAM_in_out(t,name)    write(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsOut__wire__,name
#define OMEGA_WIRE_WRITE_PROXY_PARAM_out(t,name)       no_op(false
#define OMEGA_WIRE_WRITE_PROXY_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_WRITE_PROXY_PARAM_II
#define OMEGA_WIRE_WRITE_PROXY_PARAM_size_is(size)     ,size OMEGA_WIRE_WRITE_PROXY_PARAM_II
#define OMEGA_WIRE_WRITE_PROXY_PARAM_II(t,name)

#define OMEGA_WIRE_WRITE_PROXY_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_WRITE_PROXY_PARAM_,meta) d

#define OMEGA_WIRE_WRITE_PROXY_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_WRITE_PROXY_PARAM_I,meta,(type,name))

#define OMEGA_WRITE_PARAM_WIRE_PROXY_I(meta,t,name) \
	marshal_info<t >::wire_type:: OMEGA_WIRE_WRITE_PROXY_PARAM(meta,t,name)

#define OMEGA_WRITE_PARAM_WIRE_PROXY(index,param,d) \
	OMEGA_WRITE_PARAM_WIRE_PROXY_I param ); unpack_count__wire__ = index + 1;

#define OMEGA_WRITE_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_WRITE_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_in(t,name)        no_op(false
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_in_out(t,name)    unpack(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsOut__wire__,name
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_out(t,name)       unpack(OMEGA_WIDEN_STRINGIZE(name),ptrMarshaller__wire__,pParamsOut__wire__,name
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_UNPACK_PROXY_PARAM_II
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_size_is(size)     ,size OMEGA_WIRE_UNPACK_PROXY_PARAM_II
#define OMEGA_WIRE_UNPACK_PROXY_PARAM_II(t,name)

#define OMEGA_WIRE_UNPACK_PROXY_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_UNPACK_PROXY_PARAM_,meta) d

#define OMEGA_WIRE_UNPACK_PROXY_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_UNPACK_PROXY_PARAM_I,meta,(type,name))

#define OMEGA_UNPACK_PARAM_WIRE_PROXY_I(meta,t,name) \
	marshal_info<t >::wire_type:: OMEGA_WIRE_UNPACK_PROXY_PARAM(meta,t,name)

#define OMEGA_UNPACK_PARAM_WIRE_PROXY(index,param,d) \
	if (unpack_count__wire__ > index) { OMEGA_UNPACK_PARAM_WIRE_PROXY_I param ); }

#define OMEGA_UNPACK_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_UNPACK_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD_VOID(attribs,timeout,name,param_count,params) \
	void name(OMEGA_DECLARE_PARAMS_VOID(param_count,params) ) \
	{ \
		auto_iface_ptr<Remoting::IMarshaller> ptrMarshaller__wire__ = this->GetMarshaller(); \
		auto_iface_ptr<Remoting::IMessage> pParamsOut__wire__ = this->CreateMessage(OMEGA_CONCAT(name,_MethodId)); \
		auto_iface_ptr<Remoting::IMessage> pParamsIn__wire__; \
		size_t unpack_count__wire__ = 0; OMEGA_UNUSED_ARG(unpack_count__wire__); \
		IException* OMEGA_CONCAT(name,_Exception) = 0; \
		try \
		{ \
			OMEGA_WRITE_PARAMS_WIRE_PROXY(param_count,params) \
			pParamsOut__wire__->WriteStructEnd(L"ipc_request"); \
			OMEGA_CONCAT(name,_Exception) = ptrMarshaller__wire__->SendAndReceive(attribs,pParamsOut__wire__,pParamsIn__wire__,timeout); \
		} catch (...) { \
			this->UnpackHeader(pParamsOut__wire__); \
			OMEGA_UNPACK_PARAMS_WIRE_PROXY(param_count,params) \
			throw; \
		} \
		if (OMEGA_CONCAT(name,_Exception)) OMEGA_CONCAT(name,_Exception)->Throw(); \
		OMEGA_READ_PARAMS_WIRE_PROXY(param_count,params) \
	} \
	static const uint32_t OMEGA_CONCAT(name,_MethodId) = Base::MethodCount + 

#define OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(attribs,timeout,ret_type,name,param_count,params) \
	ret_type name(OMEGA_DECLARE_PARAMS_VOID(param_count,params) ) \
	{ \
		auto_iface_ptr<Remoting::IMarshaller> ptrMarshaller__wire__ = this->GetMarshaller(); \
		auto_iface_ptr<Remoting::IMessage> pParamsOut__wire__ = this->CreateMessage(OMEGA_CONCAT(name,_MethodId)); \
		auto_iface_ptr<Remoting::IMessage> pParamsIn__wire__; \
		size_t unpack_count__wire__ = 0; OMEGA_UNUSED_ARG(unpack_count__wire__); \
		IException* OMEGA_CONCAT(name,_Exception) = 0; \
		try \
		{ \
			OMEGA_WRITE_PARAMS_WIRE_PROXY(param_count,params) \
			pParamsOut__wire__->WriteStructEnd(L"ipc_request"); \
			OMEGA_CONCAT(name,_Exception) = ptrMarshaller__wire__->SendAndReceive(attribs,pParamsOut__wire__,pParamsIn__wire__,timeout); \
		} catch (...) { \
			this->UnpackHeader(pParamsOut__wire__); \
			OMEGA_UNPACK_PARAMS_WIRE_PROXY(param_count,params) \
			throw; \
		} \
		if (OMEGA_CONCAT(name,_Exception)) OMEGA_CONCAT(name,_Exception)->Throw(); \
		OMEGA_READ_PARAMS_WIRE_PROXY(param_count,params) \
		ret_type OMEGA_CONCAT(name,_RetVal) = default_value<ret_type >::value(); \
		marshal_info<ret_type&>::wire_type::read(L"$retval",ptrMarshaller__wire__,pParamsIn__wire__,OMEGA_CONCAT(name,_RetVal)); \
		return OMEGA_CONCAT(name,_RetVal); \
	} \
	static const uint32_t OMEGA_CONCAT(name,_MethodId) = Base::MethodCount + 
	
#define OMEGA_DECLARE_WIRE_PROXY_DECLARED_NO_METHODS() \
	static const uint32_t OMEGA_CONCAT(name,_MethodId) = Base::MethodCount +

#define OMEGA_DECLARE_WIRE_PROXY_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_WIRE_PROXY_,method) index;

#define OMEGA_DECLARE_WIRE_PROXY_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_WIRE_PROXY_METHOD,methods,0)

#define OMEGA_DECLARE_PROXY(n_space,name,d_space,derived,methods) \
	template <typename D> \
	class Safe_Proxy<n_space::name,D> : public Safe_Proxy<d_space::derived,D> \
	{ \
		const vtable_info<n_space::name >::type* deref_vt() { return static_cast<const vtable_info<n_space::name >::type*>(this->m_shim->m_vtable); } \
	public: \
		static IObject* bind(const SafeShim* shim) \
		{ \
			Safe_Proxy* pThis; OMEGA_NEW(pThis,Safe_Proxy(shim)); \
			return pThis->QIReturn__proxy__(); \
		} \
	protected: \
		Safe_Proxy(const SafeShim* shim) : Safe_Proxy<d_space::derived,D>(shim) {} \
		virtual bool IsDerived__proxy__(const guid_t& iid) const \
		{ \
			if (iid == OMEGA_GUIDOF(n_space::name)) return true; \
			return Safe_Proxy<d_space::derived,D>::IsDerived__proxy__(iid); \
		} \
	private: \
		OMEGA_DECLARE_SAFE_PROXY_METHODS(methods) \
	};

#define OMEGA_DECLARE_WIRE_PROXY(n_space,name,d_space,derived,methods) \
	template <typename D> \
	class Wire_Proxy<n_space::name,D> : public Wire_Proxy<d_space::derived,D> \
	{ \
		typedef Wire_Proxy<d_space::derived,D> Base; \
	public: \
		static IObject* bind(Remoting::IProxy* pProxy) \
		{ \
			Wire_Proxy* pThis; OMEGA_NEW(pThis,Wire_Proxy(pProxy)); \
			return pThis->QIReturn__proxy__(); \
		} \
	protected: \
		Wire_Proxy(Remoting::IProxy* pProxy) : Base(pProxy) {} \
		virtual bool IsDerived__proxy__(const guid_t& iid) const \
		{ \
			if (iid == OMEGA_GUIDOF(n_space::name)) return true; \
			return Base::IsDerived__proxy__(iid); \
		} \
		static const uint32_t MethodCount = Base::MethodCount + OMEGA_SEQUENCE_SIZEOF(methods); \
	private: \
		OMEGA_DECLARE_WIRE_PROXY_METHODS(methods) \
	};

#define OMEGA_DEFINE_INTERNAL_INTERFACE_NOPROXY(n_space,name,methods) \
	OMEGA_DECLARE_SAFE(OMEGA_UNIQUE_NAME(name),methods,n_space,name,Omega,IObject) \
	OMEGA_DECLARE_TYPE(n_space,name,methods,Omega,IObject) \
	OMEGA_DECLARE_STUB(n_space,name,Omega,IObject,methods)
	
#define OMEGA_DEFINE_INTERNAL_INTERFACE(n_space,name,methods) \
	OMEGA_DECLARE_SAFE(OMEGA_UNIQUE_NAME(name),methods,n_space,name,Omega,IObject) \
	OMEGA_DECLARE_TYPE(n_space,name,methods,Omega,IObject) \
	OMEGA_DECLARE_PROXY(n_space,name,Omega,IObject,methods) \
	OMEGA_DECLARE_STUB(n_space,name,Omega,IObject,methods)

#define OMEGA_DEFINE_INTERNAL_INTERFACE_PART2_NOPROXY(n_space,name,methods) \
	OMEGA_DECLARE_WIRE_STUB(n_space,name,Omega,IObject,methods) \
	OMEGA_WIRE_MAGIC(n_space,name)

#define OMEGA_DEFINE_INTERNAL_INTERFACE_PART2(n_space,name,methods) \
	OMEGA_DECLARE_WIRE_PROXY(n_space,name,Omega,IObject,methods) \
	OMEGA_DECLARE_WIRE_STUB(n_space,name,Omega,IObject,methods) \
	OMEGA_WIRE_MAGIC(n_space,name)

#define OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL(n_space,name,d_space,derived,guid,methods) \
	OMEGA_SET_GUIDOF(n_space,name,guid) \
	namespace Omega { namespace System { namespace Internal { \
	OMEGA_DECLARE_FORWARDS(n_space,name) \
	OMEGA_DECLARE_SAFE(OMEGA_UNIQUE_NAME(name),methods,n_space,name,d_space,derived) \
	OMEGA_DECLARE_TYPE(n_space,name,methods,d_space,derived) \
	OMEGA_DECLARE_PROXY(n_space,name,d_space,derived,methods) \
	OMEGA_DECLARE_STUB(n_space,name,d_space,derived,methods) \
	OMEGA_QI_MAGIC(n_space,name) \
	} } }

#define OMEGA_DEFINE_INTERFACE_LOCAL(n_space,name,guid,methods) \
	OMEGA_DEFINE_INTERFACE_DERIVED_LOCAL(n_space,name,Omega,IObject,guid,methods)

#define OMEGA_DEFINE_INTERFACE_DERIVED(n_space,name,d_space,derived,guid,methods) \
	OMEGA_SET_GUIDOF(n_space,name,guid) \
	namespace Omega { namespace System { namespace Internal { \
	OMEGA_DECLARE_FORWARDS(n_space,name) \
	OMEGA_DECLARE_SAFE(OMEGA_UNIQUE_NAME(name),methods,n_space,name,d_space,derived) \
	OMEGA_DECLARE_TYPE(n_space,name,methods,d_space,derived) \
	OMEGA_DECLARE_PROXY(n_space,name,d_space,derived,methods) \
	OMEGA_DECLARE_STUB(n_space,name,d_space,derived,methods) \
	OMEGA_QI_MAGIC(n_space,name) \
	OMEGA_DECLARE_WIRE_PROXY(n_space,name,d_space,derived,methods) \
	OMEGA_DECLARE_WIRE_STUB(n_space,name,d_space,derived,methods) \
	OMEGA_WIRE_MAGIC(n_space,name) \
	} } }

#define OMEGA_DEFINE_INTERFACE(n_space,name,guid,methods) \
	OMEGA_DEFINE_INTERFACE_DERIVED(n_space,name,Omega,IObject,guid,methods)

#define OMEGA_METHOD_VOID(name,param_count,params) \
	(DECLARED_METHOD_VOID(TypeInfo::Synchronous,0,name,param_count,params))

#define OMEGA_METHOD(ret_type,name,param_count,params) \
	(DECLARED_METHOD(TypeInfo::Synchronous,0,ret_type,name,param_count,params))

#define OMEGA_NO_METHODS() \
	(DECLARED_NO_METHODS())

#define OMEGA_METHOD_EX_VOID(attribs,timeout,name,param_count,params) \
	(DECLARED_METHOD_VOID(TypeInfo::attribs,timeout,name,param_count,params))

#define OMEGA_METHOD_EX(attribs,timeout,ret_type,name,param_count,params) \
	(DECLARED_METHOD(TypeInfo::attribs,timeout,ret_type,name,param_count,params))

#define OMEGA_EXPORTED_FUNCTION_VOID_IMPL(name,param_count,params) \
	extern "C" OMEGA_IMPORT const Omega::System::Internal::SafeShim* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params)); \
	inline void name(OMEGA_DECLARE_PARAMS_VOID(param_count,params)) \
	{ \
		const Omega::System::Internal::SafeShim* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_PROXY_VOID(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) Omega::System::Internal::throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
	}

#define OMEGA_EXPORTED_FUNCTION_IMPL(ret_type,name,param_count,params) \
	extern "C" OMEGA_IMPORT const Omega::System::Internal::SafeShim* OMEGA_CALL OMEGA_CONCAT(name,_Safe)( \
		Omega::System::Internal::marshal_info<ret_type&>::safe_type::type OMEGA_CONCAT(name,_RetVal) \
		OMEGA_DECLARE_PARAMS_SAFE(param_count,params)); \
	inline ret_type name(OMEGA_DECLARE_PARAMS_VOID(param_count,params)) \
	{ \
		ret_type OMEGA_CONCAT(name,_RetVal) = Omega::System::Internal::default_value<ret_type >::value(); \
		const Omega::System::Internal::SafeShim* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Safe)( \
			Omega::System::Internal::marshal_info<ret_type&>::safe_type::coerce(OMEGA_CONCAT(name,_RetVal)) \
			OMEGA_DECLARE_PARAMS_SAFE_PROXY(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) Omega::System::Internal::throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		return OMEGA_CONCAT(name,_RetVal); \
	}

#define OMEGA_RAW_EXPORTED_FUNCTION_VOID_IMPL(name,param_count,params) \
	extern "C" OMEGA_IMPORT const Omega::System::Internal::SafeShim* OMEGA_CALL OMEGA_CONCAT(name,_Raw)(OMEGA_DECLARE_PARAMS_VOID(param_count,params)); \
	inline void name(OMEGA_DECLARE_PARAMS_VOID(param_count,params)) \
	{ \
		const Omega::System::Internal::SafeShim* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Raw)(OMEGA_EMIT_PARAMS_VOID(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) Omega::System::Internal::throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
	}

#define OMEGA_RAW_EXPORTED_FUNCTION_IMPL(ret_type,name,param_count,params) \
	extern "C" OMEGA_IMPORT ret_type OMEGA_CALL OMEGA_CONCAT(name,_Raw)( \
		const Omega::System::Internal::SafeShim** OMEGA_CONCAT(name,_Exception) OMEGA_DECLARE_PARAMS(param_count,params)); \
	inline ret_type name(OMEGA_DECLARE_PARAMS_VOID(param_count,params)) \
	{ \
		const Omega::System::Internal::SafeShim* OMEGA_CONCAT(name,_Exception) = 0; \
		ret_type OMEGA_CONCAT(name,_RetVal) = OMEGA_CONCAT(name,_Raw)( \
			&OMEGA_CONCAT(name,_Exception) OMEGA_EMIT_PARAMS(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) Omega::System::Internal::throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		return OMEGA_CONCAT(name,_RetVal); \
	}

#define OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params) \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params)); \
	inline void name(OMEGA_DECLARE_PARAMS_VOID(param_count,params)) \
	{ \
		OMEGA_CONCAT(name,_Impl)(OMEGA_EMIT_PARAMS_VOID(param_count,params)); \
	}

#define OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params) \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params)); \
	inline ret_type name(OMEGA_DECLARE_PARAMS_VOID(param_count,params)) \
	{ \
		return OMEGA_CONCAT(name,_Impl)(OMEGA_EMIT_PARAMS_VOID(param_count,params)); \
	}

#define OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params)); \
	extern "C" OMEGA_EXPORT const Omega::System::Internal::SafeShim* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params)) \
	{ \
		try \
		{ \
			OMEGA_CONCAT(name,_Impl)(OMEGA_DEFINE_PARAMS_SAFE_STUB_VOID(param_count,params)); \
			return 0; \
		} \
		catch (std::exception& OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(OMEGA_CONCAT(name,_Exception),OMEGA_WIDEN_STRINGIZE(name))); \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::System::Internal::return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
		catch (...) \
		{ \
			return Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(L"Unhandled exception",OMEGA_WIDEN_STRINGIZE(name))); \
		} \
	} \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params))

#define OMEGA_DEFINE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params)); \
	extern "C" OMEGA_EXPORT const Omega::System::Internal::SafeShim* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(Omega::System::Internal::marshal_info<ret_type&>::safe_type::type OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params)) \
	{ \
		try \
		{ \
			static_cast<ret_type&>(Omega::System::Internal::marshal_info<ret_type&>::safe_type::coerce(OMEGA_CONCAT(name,_RetVal))) = OMEGA_CONCAT(name,_Impl)(OMEGA_DEFINE_PARAMS_SAFE_STUB_VOID(param_count,params)); \
			return 0; \
		} \
		catch (std::exception& OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(OMEGA_CONCAT(name,_Exception),OMEGA_WIDEN_STRINGIZE(name))); \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::System::Internal::return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
		catch (...) \
		{ \
			return Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(L"Unhandled exception",OMEGA_WIDEN_STRINGIZE(name))); \
		} \
	} \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params))

#define OMEGA_DEFINE_RAW_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params)); \
	extern "C" OMEGA_EXPORT const Omega::System::Internal::SafeShim* OMEGA_CALL OMEGA_CONCAT(name,_Raw)(OMEGA_DECLARE_PARAMS_VOID(param_count,params)) \
	{ \
		try \
		{ \
			OMEGA_CONCAT(name,_Impl)(OMEGA_EMIT_PARAMS_VOID(param_count,params)); \
			return 0; \
		} \
		catch (std::exception& OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(OMEGA_CONCAT(name,_Exception),OMEGA_WIDEN_STRINGIZE(name))); \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::System::Internal::return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
		catch (...) \
		{ \
			return Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(L"Unhandled exception",OMEGA_WIDEN_STRINGIZE(name))); \
		} \
	} \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params))

#define OMEGA_DEFINE_RAW_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params)); \
	extern "C" OMEGA_EXPORT ret_type OMEGA_CALL OMEGA_CONCAT(name,_Raw)(const Omega::System::Internal::SafeShim** OMEGA_CONCAT(name,_Exception) OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		try \
		{ \
			return OMEGA_CONCAT(name,_Impl)(OMEGA_EMIT_PARAMS_VOID(param_count,params)); \
		} \
		catch (std::exception& OMEGA_CONCAT(name,_Exception2)) \
		{ \
			*OMEGA_CONCAT(name,_Exception) = Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(OMEGA_CONCAT(name,_Exception2),OMEGA_WIDEN_STRINGIZE(name))); \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception2)) \
		{ \
			*OMEGA_CONCAT(name,_Exception) = Omega::System::Internal::return_safe_exception(OMEGA_CONCAT(name,_Exception2)); \
		} \
		catch (...) \
		{ \
			*OMEGA_CONCAT(name,_Exception) = Omega::System::Internal::return_safe_exception(Omega::ISystemException::Create(L"Unhandled exception",OMEGA_WIDEN_STRINGIZE(name))); \
		} \
		return Omega::System::Internal::default_value<ret_type>::value(); \
	} \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_VOID(param_count,params))

#endif // defined(DOXYGEN)

#endif // OOCORE_MACROS_H_INCLUDED_

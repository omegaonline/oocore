#ifndef OOCORE_MACROS_H_INCLUDED_
#define OOCORE_MACROS_H_INCLUDED_

#include <OOCore/Preprocessor/sequence.h>
#include <OOCore/Preprocessor/repeat.h>
#include <OOCore/Preprocessor/comma.h>
#include <OOCore/Preprocessor/split.h>

#if !defined(OMEGA_MAX_INTERFACES)
#ifdef _MSC_VER
#define OMEGA_MAX_INTERFACES		249
#else
#define OMEGA_MAX_INTERFACES		256
#endif
#endif

#define OMEGA_MAGIC_BEGIN(N,D)		if_then_else_< (sizeof(get_qi_rtti((const qi_rtti**)0,(size_t_<N>*)0,Omega::IID_IObject)) != sizeof(yes_t)),size_t_<N>*,
#define OMEGA_THROW_MAGIC(N,D)		if_then_else_< (sizeof(safe_throw((qi_rtti*)0,(size_t_<N>*)0,Omega::IID_IObject,(IException_Safe*)0)) != sizeof(yes_t)),size_t_<N>*,
#define OMEGA_MAGIC_END(N,D)		>::type 

#define OMEGA_QI_MAGIC(n_space,iface) \
	inline yes_t get_qi_rtti(const qi_rtti** ppRtti,OMEGA_REPEAT(OMEGA_MAX_INTERFACES,OMEGA_MAGIC_BEGIN,G) char OMEGA_REPEAT(OMEGA_MAX_INTERFACES,OMEGA_MAGIC_END,G),const guid_t& iid) \
	{ \
		if (iid_traits<n_space::iface>::GetIID() == iid) \
		{ \
			static const qi_rtti rtti = \
			{ \
				&StubImpl<interface_info<n_space::iface>::stub_class<n_space::iface>::type,n_space::iface>::Create, \
				&ProxyImpl<interface_info<n_space::iface>::proxy_class<n_space::iface>::type,n_space::iface>::Create, \
				&ThrowImpl<n_space::iface>::Throw, \
			}; \
			*ppRtti = &rtti; \
			return true; \
		} \
		else \
			return false; \
	}
	
#define OMEGA_UNIQUE_NAME(name) \
	OMEGA_CONCAT(name,__LINE__)

#define OMEGA_DECLARE_FORWARDS(n_space,name,unique,d_space,derived) \
	template <class Base> interface OMEGA_CONCAT_R(unique,_Safe); \
	template <class I, class Base> struct OMEGA_CONCAT_R(unique,_Stub); \
	template <class I, class Base> struct OMEGA_CONCAT_R(unique,_Proxy); \
	template <> \
	struct interface_info<n_space::name> \
	{ \
		typedef OMEGA_CONCAT_R(unique,_Safe)<interface_info<d_space::derived>::safe_class> safe_class; \
		template <class I> struct stub_class \
		{ \
			typedef OMEGA_CONCAT_R(unique,_Stub)<I,typename interface_info<d_space::derived>::stub_class<I>::type> type; \
		}; \
		template <class I> struct proxy_class \
		{ \
			typedef OMEGA_CONCAT_R(unique,_Proxy)<I,typename interface_info<d_space::derived>::proxy_class<I>::type> type; \
		}; \
	}; \
	template <> struct interface_info<n_space::name*> \
	{ \
		typedef interface_info<n_space::name>::safe_class* safe; \
		typedef stub_functor<n_space::name> stub; \
		typedef proxy_functor<n_space::name> proxy; \
	}; \
	template <> struct interface_info<n_space::name**> \
	{ \
		typedef interface_info<n_space::name>::safe_class** safe; \
		typedef stub_functor_out<n_space::name> stub; \
		typedef proxy_functor_out<n_space::name> proxy; \
	}; 
	
#define OMEGA_DECLARE_PARAM_I(meta,type,name) \
	type name

#define OMEGA_DECLARE_PARAM(index,params,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_I params

#define OMEGA_DECLARE_PARAMS(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_EMIT_PARAM_I(meta,type,name) \
	name

#define OMEGA_EMIT_PARAM(index,params,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_EMIT_PARAM_I params

#define OMEGA_EMIT_PARAMS(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_EMIT_PARAM,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_SAFE_I(meta,type,name) \
	Omega::MetaInfo::interface_info<type>::safe name

#define OMEGA_DECLARE_PARAM_SAFE_VOID(index,params,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_SAFE_I params

#define OMEGA_DECLARE_PARAM_SAFE(index,params,d) \
	, OMEGA_DECLARE_PARAM_SAFE_I params

#define OMEGA_DECLARE_PARAMS_SAFE_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAMS_SAFE(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_PS_PARAM_in(p0,p1)
#define OMEGA_PS_PARAM_in_out(p0,p1)
#define OMEGA_PS_PARAM_size_is(size)	OMEGA_PS_PARAM_size_is_I
#define OMEGA_PS_PARAM_size_is_I(t,name) 
#define OMEGA_PS_PARAM_iid_is(id)		,id OMEGA_PS_PARAM_iid_is_I
#define OMEGA_PS_PARAM_iid_is_I(t,name) 
#define OMEGA_PS_PARAM_out(t,name)

#define OMEGA_PS_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_PS_PARAM_,meta) d

#define OMEGA_PS_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_PS_PARAM_I,meta,(type,name))

#define OMEGA_DECLARE_PARAM_STUB_I(meta,t,name) \
	Omega::MetaInfo::interface_info<t>::stub(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DECLARE_PARAM_STUB_VOID(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_STUB_I param

#define OMEGA_DECLARE_PARAM_STUB(index,param,d) \
	, OMEGA_DECLARE_PARAM_STUB_I param

#define OMEGA_DECLARE_PARAMS_STUB_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_STUB_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAMS_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_STUB,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_PROXY_I(meta,t,name) \
	Omega::MetaInfo::interface_info<t>::proxy(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DECLARE_PARAM_PROXY(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_PROXY_I param

#define OMEGA_DECLARE_PARAMS_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_PROXY,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_ZERO_PARAM_in(p0,p1)
#define OMEGA_ZERO_PARAM_in_out(p0,p1)
#define OMEGA_ZERO_PARAM_size_is(size)	OMEGA_ZERO_PARAM_size_is_I
#define OMEGA_ZERO_PARAM_size_is_I(t,name) 
#define OMEGA_ZERO_PARAM_iid_is(id)		OMEGA_ZERO_PARAM_iid_is_I
#define OMEGA_ZERO_PARAM_iid_is_I(t,name) 
#define OMEGA_ZERO_PARAM_out(t,name) \
	Omega::MetaInfo::set_null(name);

#define OMEGA_ZERO_PARAM_II(index,meta,d) \
	OMEGA_CONCAT(OMEGA_ZERO_PARAM_,meta) d

#define OMEGA_ZERO_PARAM_I(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_ZERO_PARAM_II,meta,(type,name))

#define OMEGA_ZERO_PARAM(index,param,d) \
	OMEGA_ZERO_PARAM_I param

#define OMEGA_ZERO_PARAMS(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_ZERO_PARAM,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_SAFE_DECLARED_METHOD_VOID(name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params) ) = 0;

#define OMEGA_DECLARE_SAFE_DECLARED_METHOD(ret_type,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (interface_info<ret_type*>::safe OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params) ) = 0;

#define OMEGA_DECLARE_SAFE_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_,method)

#define OMEGA_DECLARE_SAFE_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_METHOD,methods,0)

#define OMEGA_DECLARE_SAFE(methods,unique,d_space,derived) \
	template <class Base> \
	interface OMEGA_CONCAT_R(unique,_Safe) : public Base \
	{ \
		OMEGA_DECLARE_SAFE_METHODS(methods) \
	}; 

#define OMEGA_DECLARE_STUB_DECLARED_METHOD_VOID(name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params) ) \
	{ \
		try \
		{ \
			this->m_pI->name( OMEGA_DECLARE_PARAMS_PROXY(param_count,params) ); \
			return 0; \
		} \
		catch (IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return return_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	}
	
#define OMEGA_DECLARE_STUB_DECLARED_METHOD(ret_type,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (interface_info<ret_type*>::safe OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params) ) \
	{ \
		try \
		{ \
			*static_cast<ret_type*>(interface_info<ret_type*>::proxy(OMEGA_CONCAT(name,_RetVal))) = this->m_pI->name( OMEGA_DECLARE_PARAMS_PROXY(param_count,params) ); \
			return 0; \
		} \
		catch (IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return return_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	}

#define OMEGA_DECLARE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_STUB_,method)

#define OMEGA_DECLARE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_STUB_METHOD,methods,0)

#define OMEGA_DECLARE_STUB(n_space,name,unique,methods) \
	template <class I, class Base> \
	struct OMEGA_CONCAT_R(unique,_Stub) : public Base \
	{ \
		OMEGA_CONCAT_R(unique,_Stub)(I* pI) : Base(pI) \
		{ } \
		virtual IException_Safe* Internal_QueryInterface_Safe(IObject_Safe** ppS, const guid_t& iid) \
		{ \
			if (iid == iid_traits<n_space::name>::GetIID()) \
			{ \
				*ppS = this; \
				return this->AddRef_Safe(); \
			} \
			return Base::Internal_QueryInterface_Safe(ppS,iid); \
		} \
		OMEGA_DECLARE_STUB_METHODS(methods) \
	};

#define OMEGA_DECLARE_PROXY_DECLARED_METHOD_VOID(name,param_count,params) \
	void name(OMEGA_DECLARE_PARAMS(param_count,params) ) \
	{ \
		OMEGA_ZERO_PARAMS(param_count,params) \
		IException_Safe* OMEGA_CONCAT(name,_Exception) = this->m_pS->OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_STUB_VOID(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
	}

#define OMEGA_DECLARE_PROXY_DECLARED_METHOD(ret_type,name,param_count,params) \
	ret_type name(OMEGA_DECLARE_PARAMS(param_count,params) ) \
	{ \
		ret_type OMEGA_CONCAT(name,_RetVal) = Omega::MetaInfo::null_info<ret_type>::value(); \
		OMEGA_ZERO_PARAMS(param_count,params) \
		IException_Safe* OMEGA_CONCAT(name,_Exception) = this->m_pS->OMEGA_CONCAT(name,_Safe)( \
		interface_info<ret_type*>::stub(&OMEGA_CONCAT(name,_RetVal)) OMEGA_DECLARE_PARAMS_STUB(param_count,params) ); \
		if (OMEGA_CONCAT(name,_Exception)) throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		return OMEGA_CONCAT(name,_RetVal); \
	}

#define OMEGA_DECLARE_PROXY_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_PROXY_,method)

#define OMEGA_DECLARE_PROXY_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_PROXY_METHOD,methods,0)

#define OMEGA_DECLARE_PROXY(n_space,name,unique,methods) \
	template <class I, class Base> \
	struct OMEGA_CONCAT_R(unique,_Proxy) : public Base \
	{ \
		OMEGA_CONCAT_R(unique,_Proxy)(typename interface_info<I*>::safe pS) : Base(pS) \
		{ } \
		virtual IObject* Internal_QueryInterface(const guid_t& iid) \
		{ \
			if (iid == iid_traits<n_space::name>::GetIID()) \
			{ \
				this->AddRef(); \
				return this; \
			} \
			return Base::Internal_QueryInterface(iid); \
		} \
		OMEGA_DECLARE_PROXY_METHODS(methods) \
	};

#define OMEGA_EXPORT_INTERFACE_DERIVED_I(n_space,name,unique,d_space,derived,l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, methods) \
	OMEGA_DEFINE_IID(n_space,name,l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8) \
	namespace Omega { namespace MetaInfo { \
	OMEGA_DECLARE_FORWARDS(n_space,name,unique,d_space,derived) \
	OMEGA_DECLARE_SAFE(methods,unique,d_space,derived) \
	OMEGA_DECLARE_STUB(n_space,name,unique,methods) \
	OMEGA_DECLARE_PROXY(n_space,name,unique,methods) \
	OMEGA_QI_MAGIC(n_space,name) \
	} } 

#define OMEGA_EXPORT_INTERFACE_DERIVED(n_space,name,d_space,derived,l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, methods) \
	OMEGA_EXPORT_INTERFACE_DERIVED_I(n_space,name,OMEGA_UNIQUE_NAME(name),d_space,derived,l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, methods)

#define OMEGA_EXPORT_INTERFACE(n_space,name,l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, methods) \
	OMEGA_EXPORT_INTERFACE_DERIVED(n_space,name,Omega,IObject,l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8, methods)
	
#define OMEGA_METHOD_VOID(name,param_count,params) \
	(DECLARED_METHOD_VOID(name,param_count,params))

#define OMEGA_METHOD(ret_type,name,param_count,params) \
	(DECLARED_METHOD(ret_type,name,param_count,params))

#define OMEGA_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	extern "C" OMEGA_IMPORT Omega::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params)); \
	inline void name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		OMEGA_ZERO_PARAMS(param_count,params) \
		Omega::MetaInfo::IException_Safe* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_STUB_VOID(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
	}

#define OMEGA_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	extern "C" OMEGA_IMPORT Omega::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)( \
		Omega::MetaInfo::interface_info<ret_type*>::safe OMEGA_CONCAT(name,_RetVal) \
		OMEGA_DECLARE_PARAMS_SAFE(param_count,params)); \
	inline ret_type name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		ret_type OMEGA_CONCAT(name,_RetVal) = Omega::MetaInfo::null_info<ret_type>::value(); \
		OMEGA_ZERO_PARAMS(param_count,params) \
		Omega::MetaInfo::IException_Safe* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Safe)( \
			Omega::MetaInfo::interface_info<ret_type*>::stub(&OMEGA_CONCAT(name,_RetVal)) \
			OMEGA_DECLARE_PARAMS_STUB(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) Omega::MetaInfo::throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		return OMEGA_CONCAT(name,_RetVal); \
	}

#define OMEGA_LOCAL_FUNCTION_VOID(name,param_count,params) \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	inline void name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		OMEGA_CONCAT(name,_Impl)(OMEGA_EMIT_PARAMS(param_count,params)); \
	}

#define OMEGA_LOCAL_FUNCTION(ret_type,name,param_count,params) \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	inline ret_type name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		return OMEGA_CONCAT(name,_Impl)(OMEGA_EMIT_PARAMS(param_count,params)); \
	}

#define OMEGA_DEFINE_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	extern "C" OMEGA_EXPORT Omega::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params)) \
	{ \
		try \
		{ \
			OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_PROXY(param_count,params)); \
			return 0; \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::MetaInfo::return_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	} \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params))

#define OMEGA_DEFINE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	extern "C" OMEGA_EXPORT Omega::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(Omega::MetaInfo::interface_info<ret_type*>::safe OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params)) \
	{ \
		try \
		{ \
			*static_cast<ret_type*>(Omega::MetaInfo::interface_info<ret_type*>::proxy(OMEGA_CONCAT(name,_RetVal))) = OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_PROXY(param_count,params)); \
			return 0; \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::MetaInfo::return_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	} \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params))

#endif // OOCORE_MACROS_H_INCLUDED_

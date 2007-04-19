#ifndef OOCORE_MACROS_H_INCLUDED_
#define OOCORE_MACROS_H_INCLUDED_

#include <OOCore/Preprocessor/sequence.h>
#include <OOCore/Preprocessor/repeat.h>
#include <OOCore/Preprocessor/comma.h>
#include <OOCore/Preprocessor/split.h>

#if !defined(OMEGA_MAX_INTERFACES)
#define OMEGA_MAX_INTERFACES		OMEGA_MAX_DEFINES
#endif

#define OMEGA_UNIQUE_NAME(name) \
	OMEGA_CONCAT(name,__LINE__)

#define OMEGA_MAGIC_BEGIN(N,D)		if_then_else_< (sizeof(get_qi_rtti((const qi_rtti**)0,(size_t_<N>*)0,Omega::IID_IObject)) != sizeof(yes_t)),size_t_<N>*,
#define OMEGA_MAGIC_END(N,D)		>::type
#define OMEGA_THROW_MAGIC(N,D)		if_then_else_< (sizeof(safe_throw((qi_rtti*)0,(size_t_<N>*)0,Omega::IID_IObject,(IException_Safe*)0)) != sizeof(yes_t)),size_t_<N>*,

#define OMEGA_QI_MAGIC(n_space,iface) \
	typedef OMEGA_REPEAT(OMEGA_MAX_INTERFACES,OMEGA_MAGIC_BEGIN,G) char OMEGA_REPEAT(OMEGA_MAX_INTERFACES,OMEGA_MAGIC_END,G) OMEGA_UNIQUE_NAME(iface); \
	inline yes_t get_qi_rtti(const qi_rtti** ppRtti,OMEGA_UNIQUE_NAME(iface),const guid_t& iid) \
	{ \
		if (iid_traits<n_space::iface>::GetIID() == iid) \
		{ \
			static const qi_rtti rtti = \
			{ \
				SafeStubImpl<interface_info<n_space::iface>::safe_stub_factory<n_space::iface>::type,n_space::iface>::Create, \
				SafeProxyImpl<interface_info<n_space::iface>::safe_proxy_factory<n_space::iface>::type,n_space::iface>::Create, \
				SafeThrow<n_space::iface>, \
				CreateWireStub<interface_info<n_space::iface>::wire_stub_factory<n_space::iface>::type>, \
				WireProxyImpl<interface_info<n_space::iface>::wire_proxy_factory<n_space::iface>::type,n_space::iface>::Create \
			}; \
			*ppRtti = &rtti; \
			return true; \
		} \
		else \
			return false; \
	}

#define OMEGA_DECLARE_FORWARDS(n_space,name,unique,d_space,derived) \
	template <class Base> interface OMEGA_CONCAT_R(unique,_Safe); \
	template <class I, class Base> struct OMEGA_CONCAT_R(unique,_SafeStub); \
	template <class I, class Base> struct OMEGA_CONCAT_R(unique,_SafeProxy); \
	template <class I, class Base> struct OMEGA_CONCAT_R(unique,_WireStub); \
	template <class I, class Base> struct OMEGA_CONCAT_R(unique,_WireProxy); \
	template <> \
	struct interface_info<n_space::name> \
	{ \
		typedef OMEGA_CONCAT_R(unique,_Safe)<interface_info<d_space::derived>::safe_class> safe_class; \
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
			typedef OMEGA_CONCAT_R(unique,_WireStub)<I,typename interface_info<d_space::derived>::wire_stub_factory<I>::type> type; \
		}; \
		template <class I> struct wire_proxy_factory \
		{ \
			typedef OMEGA_CONCAT_R(unique,_WireProxy)<I,typename interface_info<d_space::derived>::wire_proxy_factory<I>::type> type; \
		}; \
	}; \
	template <> struct interface_info<n_space::name*> \
	{ \
		typedef interface_info<n_space::name>::safe_class* safe_class; \
		typedef iface_stub_functor<n_space::name> stub_functor; \
		typedef iface_proxy_functor<n_space::name> proxy_functor; \
		typedef iface_wire_type<n_space::name> wire_type; \
	}; \
	template <> struct interface_info<n_space::name**> \
	{ \
		typedef interface_info<n_space::name*>::safe_class* safe_class; \
		typedef iface_stub_functor_array<n_space::name> stub_functor; \
		typedef iface_proxy_functor_array<n_space::name> proxy_functor; \
		typedef std_wire_type_array<n_space::name> wire_type; \
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
	Omega::MetaInfo::interface_info<type>::safe_class name

#define OMEGA_DECLARE_PARAM_SAFE_VOID(index,params,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_SAFE_I params

#define OMEGA_DECLARE_PARAMS_SAFE_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_SAFE(index,params,d) \
	, OMEGA_DECLARE_PARAM_SAFE_I params

#define OMEGA_DECLARE_PARAMS_SAFE(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_PS_PARAM_in(p0,p1)
#define OMEGA_PS_PARAM_in_out(p0,p1)
#define OMEGA_PS_PARAM_out(t,name)
#define OMEGA_PS_PARAM_iid_is(iid)       ,iid OMEGA_PS_PARAM_iid_is_I
#define OMEGA_PS_PARAM_iid_is_I(t,name)
#define OMEGA_PS_PARAM_size_is(size)     OMEGA_PS_PARAM_size_is_I
#define OMEGA_PS_PARAM_size_is_I(t,name)

#define OMEGA_PS_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_PS_PARAM_,meta) d

#define OMEGA_PS_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_PS_PARAM_I,meta,(type,name))

#define OMEGA_DECLARE_SAFE_DECLARED_METHOD_VOID(attribs,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params) ) = 0;

#define OMEGA_DECLARE_SAFE_DECLARED_METHOD(attribs,ret_type,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (interface_info<ret_type>::safe_class* OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params) ) = 0;

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

#define OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD_VOID(attribs,name,param_count,params) \
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

#define OMEGA_DECLARE_SAFE_STUB_DECLARED_METHOD(attribs,ret_type,name,param_count,params) \
	virtual IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe) (interface_info<ret_type&>::safe_class OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params) ) \
	{ \
		try \
		{ \
			static_cast<ret_type&>(interface_info<ret_type&>::stub_functor(OMEGA_CONCAT(name,_RetVal))) = this->m_pI->name( OMEGA_DECLARE_PARAMS_SAFE_STUB(param_count,params) ); \
			return 0; \
		} \
		catch (IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	}

#define OMEGA_DECLARE_PARAM_SAFE_STUB_I(meta,t,name) \
	Omega::MetaInfo::interface_info<t>::stub_functor(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DECLARE_PARAM_SAFE_STUB(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_SAFE_STUB_I param

#define OMEGA_DECLARE_PARAMS_SAFE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_STUB,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_SAFE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_STUB_,method)

#define OMEGA_DECLARE_SAFE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_STUB_METHOD,methods,0)

// Add extra meta info types here
#define OMEGA_WIRE_READ_PARAM_in(t,name)        static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager, __wire__pParamsIn
#define OMEGA_WIRE_READ_PARAM_in_out(t,name)    static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager, __wire__pParamsIn
#define OMEGA_WIRE_READ_PARAM_out(t,name)       static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager
#define OMEGA_WIRE_READ_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_READ_PARAM_iid_is_I
#define OMEGA_WIRE_READ_PARAM_iid_is_I(t,name)
#define OMEGA_WIRE_READ_PARAM_size_is(size)     ,static_cast<const uint32_t&>(size) OMEGA_WIRE_READ_PARAM_size_is_I
#define OMEGA_WIRE_READ_PARAM_size_is_I(t,name)

#define OMEGA_WIRE_READ_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_READ_PARAM_,meta) d

#define OMEGA_WIRE_READ_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_READ_PARAM_I,meta,(type,name))

#define OMEGA_DECLARE_PARAM_WIRE_STUB_I(meta,t,name) \
	interface_info<t>::wire_type name( OMEGA_WIRE_READ_PARAM(meta,t,name) );

#define OMEGA_DECLARE_PARAM_WIRE_STUB(index,param,d) \
	OMEGA_DECLARE_PARAM_WIRE_STUB_I param

#define OMEGA_DECLARE_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

// Add extra meta info types here
#define OMEGA_WIRE_WRITE_PARAM_in(t,name)
#define OMEGA_WIRE_WRITE_PARAM_in_out(t,name)
#define OMEGA_WIRE_WRITE_PARAM_out(t,name)
#define OMEGA_WIRE_WRITE_PARAM_iid_is(iid)       ,iid OMEGA_WIRE_WRITE_PARAM_iid_is_I
#define OMEGA_WIRE_WRITE_PARAM_iid_is_I(t,name)
#define OMEGA_WIRE_WRITE_PARAM_size_is(size)     ,static_cast<const uint32_t&>(size) OMEGA_WIRE_WRITE_PARAM_size_is_I
#define OMEGA_WIRE_WRITE_PARAM_size_is_I(t,name)

#define OMEGA_WIRE_WRITE_PARAM_I(index,meta,d) \
	OMEGA_CONCAT(OMEGA_WIRE_WRITE_PARAM_,meta) d

#define OMEGA_WIRE_WRITE_PARAM(meta,type,name) \
	OMEGA_SEQUENCE_FOR_EACH_R2(OMEGA_WIRE_WRITE_PARAM_I,meta,(type,name))

#define OMEGA_WRITE_PARAM_WIRE_STUB_I(meta,t,name) \
	name.out(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut OMEGA_WIRE_WRITE_PARAM(meta,t,name) );

#define OMEGA_WRITE_PARAM_WIRE_STUB(index,param,d) \
	OMEGA_WRITE_PARAM_WIRE_STUB_I param

#define OMEGA_WRITE_PARAMS_WIRE_STUB(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_WRITE_PARAM_WIRE_STUB,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_WIRE_STUB_DECLARED_METHOD_VOID(attribs,name,param_count,params) \
	OMEGA_CONCAT(name,_Wire),

#define OMEGA_DECLARE_WIRE_STUB_DECLARED_METHOD(attribs,ret_type,name,param_count,params) \
	OMEGA_CONCAT(name,_Wire),

#define OMEGA_DECLARE_WIRE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_WIRE_STUB_,method)

#define OMEGA_DECLARE_WIRE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_WIRE_STUB_METHOD,methods,0)

#define OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD_VOID(attribs,name,param_count,params) \
	inline static void OMEGA_CONCAT(name,_Wire)(void* __wire__pParam, I* __wire__pI, Serialize::IFormattedStream* __wire__pParamsIn, Serialize::IFormattedStream* __wire__pParamsOut) \
	{ \
		OMEGA_UNUSED_ARG(__wire__pParam); OMEGA_UNUSED_ARG(__wire__pParamsIn); OMEGA_UNUSED_ARG(__wire__pParamsOut); \
		OMEGA_DECLARE_PARAMS_WIRE_STUB(param_count,params) \
		__wire__pI->name( OMEGA_EMIT_PARAMS(param_count,params) ); \
		OMEGA_WRITE_PARAMS_WIRE_STUB(param_count,params) \
	}

#define OMEGA_DEFINE_WIRE_STUB_DECLARED_METHOD(attribs,ret_type,name,param_count,params) \
	inline static void OMEGA_CONCAT(name,_Wire)(void* __wire__pParam, I* __wire__pI, Serialize::IFormattedStream* __wire__pParamsIn, Serialize::IFormattedStream* __wire__pParamsOut) \
	{ \
		OMEGA_UNUSED_ARG(__wire__pParam); OMEGA_UNUSED_ARG(__wire__pParamsIn); OMEGA_UNUSED_ARG(__wire__pParamsOut); \
		OMEGA_DECLARE_PARAMS_WIRE_STUB(param_count,params) \
		interface_info<ret_type>::wire_type OMEGA_CONCAT(name,_RetVal) ( __wire__pI->name( OMEGA_EMIT_PARAMS(param_count,params) ) ); \
		OMEGA_WRITE_PARAMS_WIRE_STUB(param_count,params) \
		OMEGA_CONCAT(name,_RetVal).out(static_cast<IObject_WireStub<I>*>(__wire__pParam)->m_pManager,__wire__pParamsOut); \
	}

#define OMEGA_DEFINE_WIRE_STUB_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DEFINE_WIRE_STUB_,method)

#define OMEGA_DEFINE_WIRE_STUB_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DEFINE_WIRE_STUB_METHOD,methods,0)

#define OMEGA_DECLARE_STUB(n_space,name,unique,methods) \
	template <class I, class Base> \
	struct OMEGA_CONCAT_R(unique,_SafeStub) : public Base \
	{ \
		OMEGA_CONCAT_R(unique,_SafeStub)(I* pI) : Base(pI) \
		{ } \
		virtual IException_Safe* Internal_QueryInterface_Safe(IObject_Safe** ppS, const guid_t& iid) \
		{ \
			if (iid == iid_traits<n_space::name>::GetIID()) \
			{ \
				*ppS = this; \
				this->AddRef_Safe(); \
				return 0; \
			} \
			return Base::Internal_QueryInterface_Safe(ppS,iid); \
		} \
		OMEGA_DECLARE_SAFE_STUB_METHODS(methods) \
	}; \
	template <class I, class Base> \
	struct OMEGA_CONCAT_R(unique,_WireStub) : public Base \
	{ \
		OMEGA_CONCAT_R(unique,_WireStub)(IWireManager* pManager, IObject* pObj, uint32_t id) : Base(pManager,pObj,id) \
		{} \
		virtual void Invoke(uint32_t method_id, Serialize::IFormattedStream* pParamsIn, Serialize::IFormattedStream* pParamsOut, uint32_t timeout) \
		{ \
			static const typename Base::MethodTableEntry MethodTable[] = \
			{ \
				OMEGA_DECLARE_WIRE_STUB_METHODS(methods) \
				0 \
			}; \
			if (method_id < Base::MethodCount) \
				Base::Invoke(method_id,pParamsIn,pParamsOut,timeout); \
			else if (method_id < MethodCount) \
				MethodTable[method_id - Base::MethodCount](this,this->m_pI,pParamsIn,pParamsOut); \
			else \
				OMEGA_THROW("Invalid method index"); \
		} \
		static const uint32_t MethodCount = Base::MethodCount + OMEGA_SEQUENCE_SIZEOF(methods); \
		OMEGA_DEFINE_WIRE_STUB_METHODS(methods) \
	private: \
		OMEGA_CONCAT_R(unique,_WireStub)() {}; \
		OMEGA_CONCAT_R(unique,_WireStub)(const OMEGA_CONCAT_R(unique,_WireStub)&) {}; \
		OMEGA_CONCAT_R(unique,_WireStub)& operator =(const OMEGA_CONCAT_R(unique,_WireStub)&) {}; \
	};

#define OMEGA_DECLARE_PARAM_SAFE_PROXY_I(meta,t,name) \
	Omega::MetaInfo::interface_info<t>::proxy_functor(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DECLARE_PARAM_SAFE_PROXY_VOID(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_SAFE_PROXY_I param

#define OMEGA_DECLARE_PARAMS_SAFE_PROXY_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_PROXY_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_SAFE_PROXY(index,param,d) \
	, OMEGA_DECLARE_PARAM_SAFE_PROXY_I param

#define OMEGA_DECLARE_PARAMS_SAFE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_SAFE_PROXY,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD_VOID(attribs,name,param_count,params) \
	void name(OMEGA_DECLARE_PARAMS(param_count,params) ) \
	{ \
		IException_Safe* OMEGA_CONCAT(name,_Exception) = this->m_pS->OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_PROXY_VOID(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
	}

#define OMEGA_DECLARE_SAFE_PROXY_DECLARED_METHOD(attribs,ret_type,name,param_count,params) \
	ret_type name(OMEGA_DECLARE_PARAMS(param_count,params) ) \
	{ \
		ret_type OMEGA_CONCAT(name,_RetVal) = Omega::MetaInfo::default_value<ret_type>::value(); \
		IException_Safe* OMEGA_CONCAT(name,_Exception) = this->m_pS->OMEGA_CONCAT(name,_Safe)( \
			interface_info<ret_type&>::proxy_functor(OMEGA_CONCAT(name,_RetVal)) \
			OMEGA_DECLARE_PARAMS_SAFE_PROXY(param_count,params) ); \
		if (OMEGA_CONCAT(name,_Exception)) throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
		return OMEGA_CONCAT(name,_RetVal); \
	}

#define OMEGA_DECLARE_SAFE_PROXY_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_SAFE_PROXY_,method)

#define OMEGA_DECLARE_SAFE_PROXY_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_SAFE_PROXY_METHOD,methods,0)

#define OMEGA_READ_PARAM_WIRE_PROXY_I(meta,t,name) \
	interface_info<t>::wire_type::proxy_read(this->m_pManager,__wire__pParamsIn,name OMEGA_WIRE_WRITE_PARAM(meta,t,name) );

#define OMEGA_READ_PARAM_WIRE_PROXY(index,param,d) \
	OMEGA_READ_PARAM_WIRE_PROXY_I param

#define OMEGA_READ_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_READ_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_WRITE_PARAM_WIRE_PROXY_I(meta,t,name) \
	interface_info<t>::wire_type::proxy_write(this->m_pManager,__wire__pParamsOut,name OMEGA_WIRE_WRITE_PARAM(meta,t,name) );

#define OMEGA_WRITE_PARAM_WIRE_PROXY(index,param,d) \
	OMEGA_WRITE_PARAM_WIRE_PROXY_I param

#define OMEGA_WRITE_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_WRITE_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_WIRE_PROXY_I(meta,t,name) \
	interface_info<t>::proxy_functor(name OMEGA_PS_PARAM(meta,t,name) )

#define OMEGA_DECLARE_PARAM_WIRE_PROXY_VOID(index,param,d) \
	OMEGA_COMMA_NOT_FIRST(index) OMEGA_DECLARE_PARAM_WIRE_PROXY_I param

#define OMEGA_DECLARE_PARAMS_WIRE_PROXY_VOID(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_WIRE_PROXY_VOID,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_PARAM_WIRE_PROXY(index,param,d) \
	, OMEGA_DECLARE_PARAM_WIRE_PROXY_I param

#define OMEGA_DECLARE_PARAMS_WIRE_PROXY(count,params) \
	OMEGA_TUPLE_FOR_EACH(count,OMEGA_DECLARE_PARAM_WIRE_PROXY,OMEGA_SPLIT_3(count,params),0)

#define OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD_VOID(attribs,name,param_count,params) \
	void name(OMEGA_DECLARE_PARAMS(param_count,params) ) \
	{ \
		auto_iface_ptr<Serialize::IFormattedStream> __wire__pParamsOut(this->m_pManager->CreateOutputStream()); \
		this->WriteKey(__wire__pParamsOut); \
		wire_write(this->m_pManager,__wire__pParamsOut,OMEGA_CONCAT(name,_MethodId)); \
		OMEGA_WRITE_PARAMS_WIRE_PROXY(param_count,params) \
		auto_iface_ptr<Serialize::IFormattedStream> __wire__pParamsIn(this->m_pManager->SendAndReceive(attribs,__wire__pParamsOut)); \
		OMEGA_READ_PARAMS_WIRE_PROXY(param_count,params) \
	} \
	static const uint32_t OMEGA_CONCAT(name,_MethodId) = Base::MethodCount +

#define OMEGA_DECLARE_WIRE_PROXY_DECLARED_METHOD(attribs,ret_type,name,param_count,params) \
	ret_type name(OMEGA_DECLARE_PARAMS(param_count,params) ) \
	{ \
		auto_iface_ptr<Serialize::IFormattedStream> __wire__pParamsOut(this->m_pManager->CreateOutputStream()); \
		ret_type OMEGA_CONCAT(name,_RetVal) = Omega::MetaInfo::default_value<ret_type>::value(); \
		this->WriteKey(__wire__pParamsOut); \
		wire_write(this->m_pManager,__wire__pParamsOut,OMEGA_CONCAT(name,_MethodId)); \
		OMEGA_WRITE_PARAMS_WIRE_PROXY(param_count,params) \
		auto_iface_ptr<Serialize::IFormattedStream> __wire__pParamsIn(this->m_pManager->SendAndReceive(attribs,__wire__pParamsOut)); \
		OMEGA_READ_PARAMS_WIRE_PROXY(param_count,params) \
		interface_info<ret_type>::wire_type::proxy_read(this->m_pManager,__wire__pParamsIn,static_cast<ret_type&>(OMEGA_CONCAT(name,_RetVal))); \
		return OMEGA_CONCAT(name,_RetVal); \
	} \
	static const uint32_t OMEGA_CONCAT(name,_MethodId) = Base::MethodCount +

#define OMEGA_DECLARE_WIRE_PROXY_METHOD(index,method,d) \
	OMEGA_CONCAT_R(OMEGA_DECLARE_WIRE_PROXY_,method) index;

#define OMEGA_DECLARE_WIRE_PROXY_METHODS(methods) \
	OMEGA_SEQUENCE_FOR_EACH_R(OMEGA_DECLARE_WIRE_PROXY_METHOD,methods,0) \
	static const uint32_t MethodCount = Base::MethodCount + OMEGA_SEQUENCE_SIZEOF(methods);

#define OMEGA_DECLARE_PROXY(n_space,name,unique,methods) \
	template <class I, class Base> \
	struct OMEGA_CONCAT_R(unique,_SafeProxy) : public Base \
	{ \
		OMEGA_CONCAT_R(unique,_SafeProxy)(typename interface_info<I*>::safe_class pS) : Base(pS) \
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
		OMEGA_DECLARE_SAFE_PROXY_METHODS(methods) \
	private: \
		OMEGA_CONCAT_R(unique,_SafeProxy)(const OMEGA_CONCAT_R(unique,_SafeProxy)&) {}; \
		OMEGA_CONCAT_R(unique,_SafeProxy)& operator = (const OMEGA_CONCAT_R(unique,_SafeProxy)&) {}; \
	}; \
	template <class I, class Base> \
	struct OMEGA_CONCAT_R(unique,_WireProxy) : public Base \
	{ \
		OMEGA_CONCAT_R(unique,_WireProxy)(IWireManager* pManager) : Base(pManager) \
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
		OMEGA_DECLARE_WIRE_PROXY_METHODS(methods) \
	private: \
		OMEGA_CONCAT_R(unique,_WireProxy)(const OMEGA_CONCAT_R(unique,_WireProxy)&) {}; \
		OMEGA_CONCAT_R(unique,_WireProxy)& operator = (const OMEGA_CONCAT_R(unique,_WireProxy)&) {}; \
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
	(DECLARED_METHOD_VOID(0,name,param_count,params))

#define OMEGA_METHOD(ret_type,name,param_count,params) \
	(DECLARED_METHOD(0,ret_type,name,param_count,params))

#define OMEGA_EXPORTED_FUNCTION_VOID(name,param_count,params) \
	extern "C" OMEGA_IMPORT Omega::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_VOID(param_count,params)); \
	inline void name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		Omega::MetaInfo::IException_Safe* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Safe)(OMEGA_DECLARE_PARAMS_SAFE_PROXY_VOID(param_count,params)); \
		if (OMEGA_CONCAT(name,_Exception)) Omega::MetaInfo::throw_correct_exception(OMEGA_CONCAT(name,_Exception)); \
	}

#define OMEGA_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	extern "C" OMEGA_IMPORT Omega::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)( \
		Omega::MetaInfo::interface_info<ret_type>::safe_class* OMEGA_CONCAT(name,_RetVal) \
		OMEGA_DECLARE_PARAMS_SAFE(param_count,params)); \
	inline ret_type name(OMEGA_DECLARE_PARAMS(param_count,params)) \
	{ \
		ret_type OMEGA_CONCAT(name,_RetVal) = Omega::MetaInfo::default_value<ret_type>::value(); \
		Omega::MetaInfo::IException_Safe* OMEGA_CONCAT(name,_Exception) = OMEGA_CONCAT(name,_Safe)( \
			Omega::MetaInfo::interface_info<ret_type&>::proxy_functor(OMEGA_CONCAT(name,_RetVal)) \
			OMEGA_DECLARE_PARAMS_SAFE_PROXY(param_count,params)); \
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
			OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_SAFE_STUB(param_count,params)); \
			return 0; \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::MetaInfo::return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	} \
	void OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params))

#define OMEGA_DEFINE_EXPORTED_FUNCTION(ret_type,name,param_count,params) \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params)); \
	extern "C" OMEGA_EXPORT Omega::MetaInfo::IException_Safe* OMEGA_CALL OMEGA_CONCAT(name,_Safe)(Omega::MetaInfo::interface_info<ret_type>::safe_class* OMEGA_CONCAT(name,_RetVal) OMEGA_DECLARE_PARAMS_SAFE(param_count,params)) \
	{ \
		try \
		{ \
			static_cast<ret_type&>(Omega::MetaInfo::interface_info<ret_type&>::stub_functor(OMEGA_CONCAT(name,_RetVal))) = OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS_SAFE_STUB(param_count,params)); \
			return 0; \
		} \
		catch (Omega::IException* OMEGA_CONCAT(name,_Exception)) \
		{ \
			return Omega::MetaInfo::return_safe_exception(OMEGA_CONCAT(name,_Exception)); \
		} \
	} \
	ret_type OMEGA_CONCAT(name,_Impl)(OMEGA_DECLARE_PARAMS(param_count,params))

#endif // OOCORE_MACROS_H_INCLUDED_

//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "ProxyStub.h" instead
//
//////////////////////////////////////////////////////
//
// I am really, really sorry about this file 
// it is complete black magic!
// Be very very careful before fiddling with it!
//
//////////////////////////////////////////////////////

#ifndef OOCORE_PROXYSTUB_MACROS_H_INCLUDED_
#define OOCORE_PROXYSTUB_MACROS_H_INCLUDED_

#include <boost/preprocessor.hpp> 
#include <boost/mpl/equal_to.hpp>
#include <boost/mpl/comparison.hpp>
#include <boost/mpl/int.hpp>

// Limits
#define PROXY_STUB_MAX_METHODS			20

#define OOCORE_PS_DECLARE_INVOKE_TABLE()			switch (method) { BOOST_PP_REPEAT(BOOST_PP_ADD(PROXY_STUB_MAX_METHODS,1),PS_DECLARE_INVOKE_TABLE_I,_) default:	return -1; }
#define PS_DECLARE_INVOKE_TABLE_I(z,n,d)			case n:	return pT->invoke(boost::mpl::int_< n >(),manager,ret_code,iface,input,output);

// IDL style attribute macros
#define OOCORE_PS_ATTRIB_in			(0)
#define OOCORE_PS_ATTRIB_out		(1)
#define OOCORE_PS_ATTRIB_size_is	(2)
#define OOCORE_PS_ATTRIB_iid_is		(3)
#define OOCORE_PS_ATTRIB_string		(4)

#define OOCORE_PS_ATTRIB_IS(a,type)					BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,type)))

// Parameter macros
#define OOCORE_PS_PARAM_ATTRIB(param)				BOOST_PP_SEQ_ELEM(0,param)
#define OOCORE_PS_PARAM_TYPE(param)					BOOST_PP_SEQ_ELEM(1,param)
#define OOCORE_PS_PARAM_VAR(param)					BOOST_PP_SEQ_ELEM(2,param)
#define OOCORE_PS_PARAM_NAME(param)					BOOST_PP_STRINGIZE(OOCORE_PS_PARAM_VAR(param))

#define OOCORE_PS_PARAM_ATTRIB_IS(param,type)		BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE(BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(OOCORE_PS_PARAM_ATTRIB_IS_I,type,OOCORE_PS_PARAM_ATTRIB(param)),-1)),2)
#define OOCORE_PS_PARAM_ATTRIB_IS_I(s,type,a)		BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,type)))

#define OOCORE_PS_PARAM_ATTRIB_ELEM_I(n,seq)		BOOST_PP_ARRAY_ELEM(BOOST_PP_ADD(n,1),BOOST_PP_SEQ_TO_ARRAY(seq))
#define OOCORE_PS_PARAM_ATTRIB_ELEM(param,type,n)	OOCORE_PS_PARAM_ATTRIB_ELEM_I(n,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,BOOST_PP_SEQ_ELEM(0,BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(OOCORE_PS_PARAM_ATTRIB_IS_I,type,OOCORE_PS_PARAM_ATTRIB(param)),-1))))

#define OOCORE_PS_PARSE_PARAMS(n,params,m)			BOOST_PP_REPEAT_FROM_TO(0,n,OOCORE_PS_PARSE_PARAMS_I,(params,m))
#define OOCORE_PS_PARSE_PARAMS_I(z,n,d)				BOOST_PP_TUPLE_ELEM(2,1,d)(n,BOOST_PP_SEQ_SUBSEQ(BOOST_PP_TUPLE_ELEM(2,0,d),BOOST_PP_MUL(n,3),3))

// Proxy function declaration
#define OOCORE_PS_DECLARE_PROXY_FN(fn,n,params)		public: virtual OOObject::int32_t fn( OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_PROXY_PARAM_DECL) )
#define OOCORE_PS_PROXY_PARAM_DECL(n,param)			BOOST_PP_COMMA_IF(n) OOCORE_PS_PARAM_TYPE(param) OOCORE_PS_PARAM_VAR(param)

// Proxy function implementation
#define OOCORE_PS_IMPL_PROXY_FN(id,n,seq)			{ OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_DECL_IMPL) \
													OOProxyStub::Impl::marshaller_t method_PROXY_FUNC(method(BOOST_PP_CAT(id,::value))); \
													method_PROXY_FUNC OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_IN_IMPL); \
													OOObject::int32_t method_PROXY_FUNC_RESULT = method_PROXY_FUNC.send_and_recv(); \
													method_PROXY_FUNC OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_OUT_IMPL); return method_PROXY_FUNC_RESULT; }

#define OOCORE_PS_PROXY_PARAM_DECL_IMPL(n,param)	BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
														OOProxyStub::Impl::array_t < OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_ARRAY_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_COMMA() OOCORE_PS_PARAM_ATTRIB_ELEM(param,size_is,0) BOOST_PP_RPAREN();, \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string), \
															OOProxyStub::Impl::string_t< OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_STRING_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_RPAREN();, \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																OOProxyStub::Impl::object_t< OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_OBJECT_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_COMMA() OOCORE_PS_PARAM_ATTRIB_ELEM(param,iid_is,0) BOOST_PP_RPAREN();, \
																BOOST_PP_EMPTY() \
															) \
														) \
													)

#define OOCORE_PS_PROXY_PARAM_IN_IMPL(n,param)		BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,in), \
														<< BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
															BOOST_PP_CAT(PROXY_ARRAY_,OOCORE_PS_PARAM_VAR(param)), \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string), \
																BOOST_PP_CAT(PROXY_STRING_,OOCORE_PS_PARAM_VAR(param)), \
																BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																	BOOST_PP_CAT(PROXY_OBJECT_,OOCORE_PS_PARAM_VAR(param)), \
																	OOCORE_PS_PARAM_VAR(param) \
																) \
															) \
														), \
														BOOST_PP_EMPTY() \
													) 

#define OOCORE_PS_PROXY_PARAM_OUT_IMPL(n,param)		BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,out), \
														>> BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
															BOOST_PP_CAT(PROXY_ARRAY_,OOCORE_PS_PARAM_VAR(param)), \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																BOOST_PP_CAT(PROXY_OBJECT_,OOCORE_PS_PARAM_VAR(param)), \
																OOCORE_PS_PARAM_VAR(param) \
															) \
														), \
														BOOST_PP_EMPTY() \
													)

// Stub function declaration
#define OOCORE_PS_DECLARE_STUB_FN(id)				private: int invoke(const id&, OOCore::ProxyStubManager* manager, OOObject::int32_t& ret_code, iface_class* obj, OOCore::InputStream* input, OOCore::OutputStream* output )

// Stub function implementation
#define OOCORE_PS_IMPL_STUB_FN(fn,n,params)			{ OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_DECL_IMPL) ret_code=obj->fn( OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_CALL_IMPL) ); \
													if (ret_code==0) { OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_OUT_IMPL) } return 0; }

#define OOCORE_PS_STUB_PARAM_DECL_IMPL(n,param)		OOProxyStub::Impl::stub_param_t< \
													BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
														OOProxyStub::Impl::array_t < OOCORE_PS_PARAM_TYPE(param) >, \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string), \
															OOProxyStub::Impl::string_t< OOCORE_PS_PARAM_TYPE(param) >, \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																OOProxyStub::Impl::object_t< OOCORE_PS_PARAM_TYPE(param) >, \
																OOCORE_PS_PARAM_TYPE(param) \
															) \
														) \
													) \
													> OOCORE_PS_PARAM_VAR(param) \
													BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,in), \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
															(input,OOCORE_PS_PARAM_ATTRIB_ELEM(param,size_is,0)), \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																(input,manager,OOCORE_PS_PARAM_ATTRIB_ELEM(param,iid_is,0)), \
																(input) \
															) \
														) ; if (OOCORE_PS_PARAM_VAR(param).failed()) return -1, \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
															(OOCORE_PS_PARAM_ATTRIB_ELEM(param,iid_is,0)), \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
																(OOCORE_PS_PARAM_ATTRIB_ELEM(param,size_is,0)), \
																BOOST_PP_EMPTY() \
															) \
														) \
													);
										
#define OOCORE_PS_STUB_PARAM_CALL_IMPL(n,param)		BOOST_PP_COMMA_IF(n) OOCORE_PS_PARAM_VAR(param)

#define OOCORE_PS_STUB_PARAM_OUT_IMPL(n,param)		BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,out), \
														if (OOCORE_PS_PARAM_VAR(param).respond \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
															(output,manager), \
															(output) \
														) \
														!=0 ) return -1;, \
														BOOST_PP_EMPTY() \
													)

// Method declaration macros
#define OOCORE_PS_METHOD_I(fn,n,params,id)			OOCORE_PS_DECLARE_METHOD_ID(id) \
													OOCORE_PS_DECLARE_PROXY_FN(fn,n,params) OOCORE_PS_IMPL_PROXY_FN(id,n,params) \
													OOCORE_PS_DECLARE_STUB_FN(id) OOCORE_PS_IMPL_STUB_FN(fn,n,params)

// ID generation
#define OOCORE_PS_GENERATE_UNIQUE_ID(fn,n)				OOCORE_PS_GENERATE_UNIQUE_ID_I(fn,n,__LINE__)
#define OOCORE_PS_GENERATE_UNIQUE_ID_I(fn,n,__LINE__)	OOCORE_PS_GENERATE_UNIQUE_ID_II(fn,n,__LINE__)
#define OOCORE_PS_GENERATE_UNIQUE_ID_II(fn,n,Line)		BOOST_PP_CAT(PROXYSTUB_ID_,BOOST_PP_CAT(fn,BOOST_PP_CAT(n,Line)))

// ID declaration - this is pure witchcraft!
#ifdef NO_BLACK_MAGIC
#define OOCORE_PS_DECLARE_METHOD_ID(id)			[Magic occurs here!]
#else
#define OOCORE_PS_DECLARE_METHOD_ID(id)			public: typedef \
												BOOST_PP_ENUM(PROXY_STUB_MAX_METHODS,OOCORE_PS_DECLARE_METHOD_ID_I,_), \
												boost::mpl::int_<PROXY_STUB_MAX_METHODS> \
												BOOST_PP_REPEAT(PROXY_STUB_MAX_METHODS,OOCORE_PS_DECLARE_METHOD_ID_II,_) \
												id; friend OOProxyStub::Impl::used_t Method_Id_Gen_(this_class*,id *);

#define OOCORE_PS_DECLARE_METHOD_ID_I(z,n,d)		boost::mpl::if_<boost::mpl::bool_<(sizeof(Method_Id_Gen_((this_class*)0,(boost::mpl::int_< n >*)0)) == sizeof(OOProxyStub::Impl::unused_t))>, boost::mpl::int_< n > 
#define OOCORE_PS_DECLARE_METHOD_ID_II(z,n,d)		>::type

#endif

// Default fn implementation
#define OOCORE_PS_DECLARE_RELEASE()				OOCORE_PS_DECLARE_RELEASE_I(OOCORE_PS_GENERATE_UNIQUE_ID(Release,0))
#define OOCORE_PS_DECLARE_RELEASE_I(id)			OOCORE_PS_DECLARE_METHOD_ID(id) OOObject::int32_t Release() { return Release_i(id::value); }

#define OOCORE_PS_DECLARE_QI()					OOCORE_PS_DECLARE_QI_I(OOCORE_PS_GENERATE_UNIQUE_ID(QueryInterface,2))
#define OOCORE_PS_DECLARE_QI_I(id)				OOCORE_PS_DECLARE_METHOD_ID(id) OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal) \
												{ return QueryInterface_i(id::value,iid,ppVal); }

// Method declaration macros
#define METHOD(fn,n,p)							OOCORE_PS_METHOD_I(fn,n,BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_MUL(n,3),p),OOCORE_PS_GENERATE_UNIQUE_ID(fn,n))

// Proxy/Stub declaration
#define BEGIN_AUTO_PROXY_STUB(iface)				OOCORE_PS_BEGIN_AUTO_PROXY_I(iface,BOOST_PP_CAT(iface,_Proxy_Stub_Impl))
#define OOCORE_PS_BEGIN_AUTO_PROXY_I(iface,name)	class name : public OOProxyStub::ProxyStub_Impl<iface> { \
													friend class OOProxyStub::Impl::invoker_t; \
													template <class T> int invoke(const T&, OOCore::ProxyStubManager* manager, OOObject::int32_t& ret_code, iface* obj, OOCore::InputStream* input, OOCore::OutputStream* output ) { return -1; } \
													name(OOCore::ProxyStubManager* manager, iface* obj) : OOProxyStub::ProxyStub_Impl<iface>(manager,obj) {} \
													name(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& key) : OOProxyStub::ProxyStub_Impl<iface>(manager,key) {} \
													friend OOProxyStub::Impl::unused_t Method_Id_Gen_(name*,...); \
													int invoke_i(iface* obj, OOObject::uint32_t method, OOCore::ProxyStubManager* manager, OOObject::int32_t& ret_code, OOCore::InputStream* input, OOCore::OutputStream* output) { \
													return OOProxyStub::Impl::invoker_t::Invoke(this,obj,manager,method,ret_code,input,output); } \
													public: typedef name this_class; typedef iface iface_class; \
													static iface* create_proxy(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& key) { name* proxy; ACE_NEW_RETURN(proxy,this_class(manager,key),0); return proxy;} \
													static OOCore::Stub* create_stub(OOCore::ProxyStubManager* manager, iface* obj) { name* stub; ACE_NEW_RETURN(stub,this_class(manager,obj),0); return stub;} \
													OOCORE_PS_DECLARE_RELEASE() OOCORE_PS_DECLARE_QI()
												
#define END_AUTO_PROXY_STUB()					};

#define CREATE_AUTO_STUB(iface,manager,obj)		BOOST_PP_CAT(iface,_Proxy_Stub_Impl::create_stub(manager,static_cast<iface*>(obj)))
#define CREATE_AUTO_PROXY(iface,manager,key) 	BOOST_PP_CAT(iface,_Proxy_Stub_Impl::create_proxy(manager,key))

#endif // OOCORE_PROXYSTUB_MACROS_H_INCLUDED_

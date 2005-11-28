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

#define DECLARE_INVOKE_TABLE()			switch (method) { BOOST_PP_REPEAT(BOOST_PP_ADD(PROXY_STUB_MAX_METHODS,1),DECLARE_INVOKE_TABLE_I,_) default:	return -1; }
#define DECLARE_INVOKE_TABLE_I(z,n,d)	case n:	return pT->invoke(boost::mpl::int_< n >(),ret_code,iface,input,output);

// IDL style attribute macros
#define ATTRIB_in		(0)
#define ATTRIB_out		(1)
#define ATTRIB_size_is	(2)
#define ATTRIB_iid_is	(3)
#define ATTRIB_string	(4)

#define ATTRIB_IS(a,type)				BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(ATTRIB_,type)))

// Parameter macros
#define PARAM_ATTRIB(param)				BOOST_PP_SEQ_ELEM(0,param)
#define PARAM_TYPE(param)				BOOST_PP_SEQ_ELEM(1,param)
#define PARAM_VAR(param)				BOOST_PP_SEQ_ELEM(2,param)
#define PARAM_NAME(param)				BOOST_PP_STRINGIZE(PARAM_VAR(param))

#define PARAM_ATTRIB_IS(param,type)		BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE(BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(PARAM_ATTRIB_IS_I,type,PARAM_ATTRIB(param)),-1)),2)
#define PARAM_ATTRIB_IS_I(s,type,a)		BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(ATTRIB_,type)))

#define PARAM_ATTRIB_ELEM_I(n,seq)		BOOST_PP_ARRAY_ELEM(BOOST_PP_ADD(n,1),BOOST_PP_SEQ_TO_ARRAY(seq))
#define PARAM_ATTRIB_ELEM(param,type,n)	PARAM_ATTRIB_ELEM_I(n,BOOST_PP_CAT(ATTRIB_,BOOST_PP_SEQ_ELEM(0,BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(PARAM_ATTRIB_IS_I,type,PARAM_ATTRIB(param)),-1))))

#define PARSE_PARAMS(n,params,m)		BOOST_PP_REPEAT_FROM_TO(0,n,PARSE_PARAMS_I,(params,m))
#define PARSE_PARAMS_I(z,n,d)			BOOST_PP_TUPLE_ELEM(2,1,d)(n,BOOST_PP_SEQ_SUBSEQ(BOOST_PP_TUPLE_ELEM(2,0,d),BOOST_PP_MUL(n,3),3))

// Proxy function declaration
#define DECLARE_PROXY_FN(fn,n,params)	public: virtual OOObj::int32_t fn( PARSE_PARAMS(n,params,PROXY_PARAM_DECL) )
#define PROXY_PARAM_DECL(n,param)		BOOST_PP_COMMA_IF(n) PARAM_TYPE(param) PARAM_VAR(param)

// Proxy function implementation
#define IMPL_PROXY_FN(id,n,seq)			{ PARSE_PARAMS(n,seq,PROXY_PARAM_DECL_IMPL) \
										Proxy_Stub::marshaller_t method_PROXY_FUNC(method(BOOST_PP_CAT(id,::value))); \
										method_PROXY_FUNC PARSE_PARAMS(n,seq,PROXY_PARAM_IN_IMPL); \
										OOObj::int32_t method_PROXY_FUNC_RESULT = method_PROXY_FUNC.send_and_recv(); \
										method_PROXY_FUNC PARSE_PARAMS(n,seq,PROXY_PARAM_OUT_IMPL); return method_PROXY_FUNC_RESULT; }

#define PROXY_PARAM_DECL_IMPL(n,param)	BOOST_PP_IF(PARAM_ATTRIB_IS(param,size_is), \
											Proxy_Stub::array_t< PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_ARRAY_,PARAM_VAR(param)) BOOST_PP_LPAREN() PARAM_VAR(param) BOOST_PP_COMMA() PARAM_ATTRIB_ELEM(param,size_is,0) BOOST_PP_RPAREN();, \
											BOOST_PP_IF(PARAM_ATTRIB_IS(param,string), \
												Proxy_Stub::string_t< PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_STRING_,PARAM_VAR(param)) BOOST_PP_LPAREN() PARAM_VAR(param) BOOST_PP_RPAREN();, \
												BOOST_PP_IF(PARAM_ATTRIB_IS(param,iid_is), \
													Proxy_Stub::object_t< PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_OBJECT_,PARAM_VAR(param)) BOOST_PP_LPAREN() PARAM_VAR(param) BOOST_PP_COMMA() PARAM_ATTRIB_ELEM(param,iid_is,0) BOOST_PP_RPAREN();, \
													BOOST_PP_EMPTY() \
												) \
											) \
										)

#define PROXY_PARAM_IN_IMPL(n,param)	BOOST_PP_IF(PARAM_ATTRIB_IS(param,in), \
											<< BOOST_PP_IF(PARAM_ATTRIB_IS(param,size_is), \
												BOOST_PP_CAT(PROXY_ARRAY_,PARAM_VAR(param)), \
												BOOST_PP_IF(PARAM_ATTRIB_IS(param,string), \
													BOOST_PP_CAT(PROXY_STRING_,PARAM_VAR(param)), \
													BOOST_PP_IF(PARAM_ATTRIB_IS(param,iid_is), \
														BOOST_PP_CAT(PROXY_OBJECT_,PARAM_VAR(param)), \
														PARAM_VAR(param) \
													) \
												) \
											), \
											BOOST_PP_EMPTY() \
										) 

#define PROXY_PARAM_OUT_IMPL(n,param)	BOOST_PP_IF(PARAM_ATTRIB_IS(param,out), \
											>> BOOST_PP_IF(PARAM_ATTRIB_IS(param,size_is), \
												BOOST_PP_CAT(PROXY_ARRAY_,PARAM_VAR(param)), \
												BOOST_PP_IF(PARAM_ATTRIB_IS(param,iid_is), \
													BOOST_PP_CAT(PROXY_OBJECT_,PARAM_VAR(param)), \
													PARAM_VAR(param) \
												) \
											), \
											BOOST_PP_EMPTY() \
										)

// Stub function declaration
#define DECLARE_STUB_FN(id)			private: int invoke(const id&, OOObj::int32_t& ret_code, iface_class* obj, OOCore::InputStream* input, OOCore::OutputStream* output )

// Stub function implementation
#define IMPL_STUB_FN(fn,n,params)		{ PARSE_PARAMS(n,params,STUB_PARAM_DECL_IMPL) ret_code=obj->fn( PARSE_PARAMS(n,params,STUB_PARAM_CALL_IMPL) ); \
										if (ret_code==0) { PARSE_PARAMS(n,params,STUB_PARAM_OUT_IMPL) } return 0; }

#define STUB_PARAM_DECL_IMPL(n,param)	Proxy_Stub::stub_param_t< \
										BOOST_PP_IF(PARAM_ATTRIB_IS(param,size_is), \
											Proxy_Stub::array_t< PARAM_TYPE(param) >, \
											BOOST_PP_IF(PARAM_ATTRIB_IS(param,string), \
												Proxy_Stub::string_t< PARAM_TYPE(param) >, \
													BOOST_PP_IF(PARAM_ATTRIB_IS(param,iid_is), \
													Proxy_Stub::object_t< PARAM_TYPE(param) >, \
													PARAM_TYPE(param) \
												) \
											) \
										) \
										> PARAM_VAR(param) \
                                        BOOST_PP_IF(PARAM_ATTRIB_IS(param,in), \
											BOOST_PP_IF(PARAM_ATTRIB_IS(param,size_is), \
												(input,PARAM_ATTRIB_ELEM(param,size_is,0)), \
												BOOST_PP_IF(PARAM_ATTRIB_IS(param,iid_is), \
													(input,PARAM_ATTRIB_ELEM(param,iid_is,0)), \
													(input) \
												) \
											) ; if (PARAM_VAR(param).failed()) return -1, \
											BOOST_PP_IF(PARAM_ATTRIB_IS(param,iid_is), \
												(PARAM_ATTRIB_ELEM(param,iid_is,0)), \
												BOOST_PP_IF(PARAM_ATTRIB_IS(param,size_is), \
													(PARAM_ATTRIB_ELEM(param,size_is,0)), \
													BOOST_PP_EMPTY() \
												) \
											) \
										);
										
#define STUB_PARAM_CALL_IMPL(n,param)	BOOST_PP_COMMA_IF(n) PARAM_VAR(param)

#define STUB_PARAM_OUT_IMPL(n,param)	BOOST_PP_IF(PARAM_ATTRIB_IS(param,out), \
											if (PARAM_VAR(param).respond(output) !=0 ) return -1;, \
										BOOST_PP_EMPTY() )

// Method declaration macros
#define METHOD(fn,n,p)				METHOD_I(fn,n,BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_MUL(n,3),p),GENERATE_UNIQUE_ID(fn,n))
#define METHOD_I(fn,n,params,id)	DECLARE_METHOD_ID(id) \
									DECLARE_PROXY_FN(fn,n,params) IMPL_PROXY_FN(id,n,params) \
									DECLARE_STUB_FN(id) IMPL_STUB_FN(fn,n,params)

// ID generation
#define GENERATE_UNIQUE_ID(fn,n)			GENERATE_UNIQUE_ID_I(fn,n,__LINE__)
#define GENERATE_UNIQUE_ID_I(fn,n,__LINE__)	GENERATE_UNIQUE_ID_II(fn,n,__LINE__)
#define GENERATE_UNIQUE_ID_II(fn,n,Line)	BOOST_PP_CAT(PROXYSTUB_ID_,BOOST_PP_CAT(fn,BOOST_PP_CAT(n,Line)))

// ID declaration - this is pure witchcraft!
#ifdef NO_BLACK_MAGIC
#define DECLARE_METHOD_ID(id)			[Magic occurs here!]
#else
#define DECLARE_METHOD_ID(id)			public: typedef \
										BOOST_PP_ENUM(PROXY_STUB_MAX_METHODS,DECLARE_METHOD_ID_I,_), \
										boost::mpl::int_<PROXY_STUB_MAX_METHODS> \
										BOOST_PP_REPEAT(PROXY_STUB_MAX_METHODS,DECLARE_METHOD_ID_II,_) \
										id; friend Proxy_Stub::used_t Method_Id_Gen_(this_class*,id *);

#define DECLARE_METHOD_ID_I(z,n,d)		boost::mpl::if_<boost::mpl::bool_<(sizeof(Method_Id_Gen_((this_class*)0,(boost::mpl::int_< n >*)0)) == sizeof(Proxy_Stub::unused_t))>, boost::mpl::int_< n > 
#define DECLARE_METHOD_ID_II(z,n,d)		>::type

#endif

// Default fn implementation
#define DECLARE_RELEASE()				DECLARE_RELEASE_I(GENERATE_UNIQUE_ID(Release,0))
#define DECLARE_RELEASE_I(id)			DECLARE_METHOD_ID(id) OOObj::int32_t Release() { return Release_i(id::value); }

#define DECLARE_QI()					DECLARE_QI_I(GENERATE_UNIQUE_ID(QueryInterface,2))
#define DECLARE_QI_I(id)				DECLARE_METHOD_ID(id) OOObj::int32_t QueryInterface(const OOObj::guid_t& iid, OOObj::Object** ppVal) \
										{ return QueryInterface_i(id::value,iid,ppVal); }

// Proxy/Stub declaration
#define BEGIN_AUTO_PROXY_STUB(iface)	BEGIN_AUTO_PROXY_I(iface,BOOST_PP_CAT(iface,_Proxy_Stub_Impl))
#define BEGIN_AUTO_PROXY_I(iface,name)	class name : public Proxy_Stub::ProxyStub_Impl<iface> { \
										friend class Proxy_Stub::invoker_t; \
										template <class T> int invoke(const T&, OOObj::int32_t& ret_code, iface* obj, OOCore::InputStream* input, OOCore::OutputStream* output ) { return -1; } \
										name(OOCore::ProxyStubManager* manager, OOObj::Object* obj) : Proxy_Stub::ProxyStub_Impl<iface>(manager,obj) {} \
										name(OOCore::ProxyStubManager* manager, const OOObj::cookie_t& key) : Proxy_Stub::ProxyStub_Impl<iface>(manager,key) {} \
										friend Proxy_Stub::unused_t Method_Id_Gen_(name*,...); \
										public: typedef name this_class; typedef iface iface_class; \
										static iface* create_proxy(OOCore::ProxyStubManager* manager, const OOObj::cookie_t& key) { name* proxy; ACE_NEW_RETURN(proxy,this_class(manager,key),0); return proxy;} \
										static OOCore::Stub* create_stub(OOCore::ProxyStubManager* manager, OOObj::Object* obj) { name* stub; ACE_NEW_RETURN(stub,this_class(manager,obj),0); return stub;} \
										DECLARE_RELEASE() DECLARE_QI()
										
#define END_AUTO_PROXY_STUB()			};

#define CREATE_AUTO_STUB(iface,manager,obj)		BOOST_PP_CAT(iface,_Proxy_Stub_Impl::create_stub(manager,obj))
#define CREATE_AUTO_PROXY(iface,manager,key) 	BOOST_PP_CAT(iface,_Proxy_Stub_Impl::create_proxy(manager,key))

#endif // OOCORE_PROXYSTUB_MACROS_H_INCLUDED_

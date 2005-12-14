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

#define OOCORE_PS_DECLARE_INVOKE_TABLE()			switch (method) { BOOST_PP_REPEAT(BOOST_PP_ADD(PROXY_STUB_MAX_METHODS,1),OOCORE_PS_DECLARE_INVOKE_TABLE_I,_) default:errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),method),-1); }
#define OOCORE_PS_DECLARE_INVOKE_TABLE_I(z,n,d)		case n:	return pT->invoke(boost::mpl::int_< n >(),manager,iface,input,output);

// IDL style attribute macros
#define OOCORE_PS_ATTRIB_in						(0)
#define OOCORE_PS_ATTRIB_out					(1)
#define OOCORE_PS_ATTRIB_size_is				(2)
#define OOCORE_PS_ATTRIB_iid_is					(3)
#define OOCORE_PS_ATTRIB_string					(4)

#define OOCORE_PS_ATTRIB_IS(a,type)				BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,type)))

// Parameter macros
#define OOCORE_PS_PARAM_ATTRIB(param)			BOOST_PP_SEQ_ELEM(0,param)
#define OOCORE_PS_PARAM_TYPE(param)				BOOST_PP_SEQ_ELEM(1,param)
#define OOCORE_PS_PARAM_VAR(param)				BOOST_PP_SEQ_ELEM(2,param)
#define OOCORE_PS_PARAM_NAME(param)				BOOST_PP_STRINGIZE(OOCORE_PS_PARAM_VAR(param))

#define OOCORE_PS_PARAM_ATTRIB_IS(param,type)		BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE(BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(OOCORE_PS_PARAM_ATTRIB_IS_I,type,OOCORE_PS_PARAM_ATTRIB(param)),99)),2)
#define OOCORE_PS_PARAM_ATTRIB_IS_I(s,type,a)		BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,type)))

#define OOCORE_PS_PARAM_ATTRIB_ELEM_I(n,seq)		BOOST_PP_ARRAY_ELEM(BOOST_PP_ADD(n,1),BOOST_PP_SEQ_TO_ARRAY(seq))
#define OOCORE_PS_PARAM_ATTRIB_ELEM(param,type,n)	OOCORE_PS_PARAM_ATTRIB_ELEM_I(n,BOOST_PP_CAT(OOCORE_PS_ATTRIB_,BOOST_PP_SEQ_ELEM(0,BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(OOCORE_PS_PARAM_ATTRIB_IS_I,type,OOCORE_PS_PARAM_ATTRIB(param)),99))))

#define OOCORE_PS_PARSE_PARAMS(n,params,m)			BOOST_PP_REPEAT_FROM_TO(0,n,OOCORE_PS_PARSE_PARAMS_I,(params,m))
#define OOCORE_PS_PARSE_PARAMS_I(z,n,d)				BOOST_PP_TUPLE_ELEM(2,1,d)(n,BOOST_PP_SEQ_SUBSEQ(BOOST_PP_TUPLE_ELEM(2,0,d),BOOST_PP_MUL(n,3),3))

// ID generation
#define OOCORE_PS_GENERATE_UNIQUE_ID(fn,n)				OOCORE_PS_GENERATE_UNIQUE_ID_I(fn,n,__LINE__)
#define OOCORE_PS_GENERATE_UNIQUE_ID_I(fn,n,__LINE__)	OOCORE_PS_GENERATE_UNIQUE_ID_II(fn,n,__LINE__)
#define OOCORE_PS_GENERATE_UNIQUE_ID_II(fn,n,Line)		BOOST_PP_CAT(PROXYSTUB_ID_,BOOST_PP_CAT(fn,BOOST_PP_CAT(n,Line)))

// ID declaration - this is pure witchcraft!
#define OOCORE_PS_DECLARE_METHOD_ID(id,name)		public: typedef \
													BOOST_PP_ENUM(PROXY_STUB_MAX_METHODS,OOCORE_PS_DECLARE_METHOD_ID_I,name), \
													boost::mpl::int_<PROXY_STUB_MAX_METHODS> \
													BOOST_PP_REPEAT(PROXY_STUB_MAX_METHODS,OOCORE_PS_DECLARE_METHOD_ID_II,name) \
													id; friend OOCore::Impl::used_t BOOST_PP_CAT(Method_Id_Gen_,name)(this_class*,id *);

#define OOCORE_PS_DECLARE_METHOD_ID_I(z,n,name)		boost::mpl::if_<boost::mpl::bool_<(sizeof(BOOST_PP_CAT(Method_Id_Gen_,name)((this_class*)0,(boost::mpl::int_< n >*)0)) == sizeof(OOCore::Impl::unused_t))>, boost::mpl::int_< n > 
#define OOCORE_PS_DECLARE_METHOD_ID_II(z,n,name)	>::type

// Proxy function declaration
#define OOCORE_PS_DECLARE_PROXY_FN(fn,n,params)		public: virtual OOObject::int32_t fn( OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_PROXY_PARAM_DECL) )
#define OOCORE_PS_PROXY_PARAM_DECL(n,param)			BOOST_PP_COMMA_IF(n) OOCORE_PS_PARAM_TYPE(param) OOCORE_PS_PARAM_VAR(param)

// Proxy function implementation
#define OOCORE_PS_IMPL_PROXY_FN(id,n,seq)			{ OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_DECL_IMPL) \
													OOCore::Impl::marshaller_t method_PROXY_FUNC(method(id::value)); \
													method_PROXY_FUNC OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_IN_IMPL); \
													OOObject::int32_t method_PROXY_FUNC_RESULT = method_PROXY_FUNC.send_and_recv(); \
													OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_OUT_IMPL) return method_PROXY_FUNC_RESULT; }

#define OOCORE_PS_PROXY_PARAM_DECL_IMPL(n,param)	BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
														OOCore::Impl::array_t < OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_ARRAY_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_COMMA() OOCORE_PS_PARAM_ATTRIB_ELEM(param,size_is,0) BOOST_PP_RPAREN();, \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string), \
															OOCore::Impl::string_t< OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_STRING_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_RPAREN();, \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																OOCore::Impl::object_t< OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_OBJECT_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_COMMA() OOCORE_PS_PARAM_ATTRIB_ELEM(param,iid_is,0) BOOST_PP_RPAREN();, \
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
														method_PROXY_FUNC >> BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
															BOOST_PP_CAT(PROXY_ARRAY_,OOCORE_PS_PARAM_VAR(param)), \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																BOOST_PP_CAT(PROXY_OBJECT_,OOCORE_PS_PARAM_VAR(param)), \
																OOCORE_PS_PARAM_VAR(param) \
															) \
														);, \
														BOOST_PP_EMPTY() \
													)

// Stub function declaration
#define OOCORE_PS_DECLARE_STUB_FN(id)				private: int invoke(const id&, OOCore::ProxyStubManager* manager, iface_class* obj, OOCore::Impl::InputStream_Wrapper& input, OOCore::Impl::OutputStream_Wrapper& output )

// Stub function implementation
#define OOCORE_PS_IMPL_STUB_FN(fn,n,params)			{ OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_DECL_IMPL) \
													OOObject::int32_t ret_code=obj->fn( OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_CALL_IMPL) ); \
													if (output.write(ret_code) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write return code\n")),-1); \
													if (ret_code!=0) { if (output->WriteLong(errno) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write errno\n")),-1); } \
													else { OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_OUT_IMPL) } return 0; }

#define OOCORE_PS_STUB_PARAM_DECL_IMPL(n,param)		OOCore::Impl::param_t< \
													BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
														OOCore::Impl::array_t < OOCORE_PS_PARAM_TYPE(param) >, \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string), \
															OOCore::Impl::string_t< OOCORE_PS_PARAM_TYPE(param) >, \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																OOCore::Impl::object_t< OOCORE_PS_PARAM_TYPE(param) >, \
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
														) ; if (OOCORE_PS_PARAM_VAR(param).failed()) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to read in param\n")),-1), \
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
														!=0 ) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write out param\n")),-1);, \
														BOOST_PP_EMPTY() \
													)

// Method declaration macros
#define OOCORE_PS_METHOD_I(fn,n,params,id)			OOCORE_PS_DECLARE_METHOD_ID(id,PS) \
													OOCORE_PS_DECLARE_PROXY_FN(fn,n,params) OOCORE_PS_IMPL_PROXY_FN(id,n,params) \
													OOCORE_PS_DECLARE_STUB_FN(id) OOCORE_PS_IMPL_STUB_FN(fn,n,params)

// Default fn implementation
#define OOCORE_PS_DECLARE_RELEASE()				OOCORE_PS_DECLARE_RELEASE_I(OOCORE_PS_GENERATE_UNIQUE_ID(Release,0))
#define OOCORE_PS_DECLARE_RELEASE_I(id)			OOCORE_PS_DECLARE_METHOD_ID(id,PS) OOObject::int32_t Release() { return Release_i(id::value,false); } \
												OOCORE_PS_DECLARE_STUB_FN(id) { \
												OOObject::int32_t ret_code=Release_i(id::value,true); \
												if (output.write(ret_code) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write return code\n")),-1); \
												if (ret_code!=0) { if (output->WriteLong(errno) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write errno\n")),-1); } \
												return 0; }

#define OOCORE_PS_DECLARE_QI()					OOCORE_PS_DECLARE_QI_I(OOCORE_PS_GENERATE_UNIQUE_ID(QueryInterface,2))
#define OOCORE_PS_DECLARE_QI_I(id)				OOCORE_PS_DECLARE_METHOD_ID(id,PS) OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal) \
												{ return QueryInterface_i(id::value,iid,ppVal); } \
												OOCORE_PS_DECLARE_STUB_FN(id) { OOCore::Impl::param_t<const OOObject::guid_t&> iid(input); \
												if (iid.failed()) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to read in param\n")),-1); \
												OOCore::Impl::param_t<OOCore::Impl::object_t<OOObject::Object**> > ppVal(iid); \
												OOObject::int32_t ret_code=QueryInterface_i(id::value,iid,ppVal); \
												if (output.write(ret_code) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write return code\n")),-1); \
												if (ret_code!=0) { if (output->WriteLong(errno) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write errno\n")),-1); } \
												else { if (ppVal.respond(output,manager)!=0 ) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write out param\n")),-1); } return 0; }

// Method declaration macros
#define METHOD(fn,n,p)							OOCORE_PS_METHOD_I(fn,n,BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_MUL(n,3),p),OOCORE_PS_GENERATE_UNIQUE_ID(fn,n))

// Proxy/Stub declaration
#define BEGIN_AUTO_PROXY_STUB(iface)				OOCORE_PS_BEGIN_AUTO_PROXY_I(iface,BOOST_PP_CAT(iface,_Proxy_Stub_Impl))
#define OOCORE_PS_BEGIN_AUTO_PROXY_I(iface,name)	class name : public OOCore::ProxyStub_Impl<iface> { \
													friend class OOCore::Impl::invoker_t; \
													template <class T> int invoke(const T&, OOCore::ProxyStubManager* manager, iface* obj, OOCore::Impl::InputStream_Wrapper& input, OOCore::Impl::OutputStream_Wrapper& output ) { errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),T::value),-1); } \
													name(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& key, iface* obj) : OOCore::ProxyStub_Impl<iface>(manager,key,obj) {} \
													name(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& key) : OOCore::ProxyStub_Impl<iface>(manager,key) {} \
													friend OOCore::Impl::unused_t BOOST_PP_CAT(Method_Id_Gen_,PS)(name*,...); \
													int invoke_i(iface* obj, OOObject::uint32_t& method, OOCore::Object_Ptr<OOCore::ProxyStubManager>& manager, OOCore::Impl::InputStream_Wrapper input, OOCore::Impl::OutputStream_Wrapper output) { \
													return OOCore::Impl::invoker_t::Invoke(this,obj,manager,method,input,output); } \
													public: typedef name this_class; typedef iface iface_class; \
													static iface* create_proxy(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& key) { name* proxy; ACE_NEW_RETURN(proxy,this_class(manager,key),0); return proxy;} \
													static OOCore::Stub* create_stub(OOCore::ProxyStubManager* manager, const OOObject::cookie_t& key, iface* obj) { name* stub; ACE_NEW_RETURN(stub,this_class(manager,key,obj),0); return stub;} \
													OOCORE_PS_DECLARE_RELEASE() OOCORE_PS_DECLARE_QI()
												
#define END_AUTO_PROXY_STUB()						};

#define OOCORE_PS_CREATE_AUTO_STUB(iface,manager,key,obj)	BOOST_PP_CAT(iface,_Proxy_Stub_Impl::create_stub(manager,key,static_cast<iface*>(obj)))
#define OOCORE_PS_CREATE_AUTO_PROXY(iface,manager,key) 		BOOST_PP_CAT(iface,_Proxy_Stub_Impl::create_proxy(manager,key))

#define BEGIN_PROXY_STUB_MAP(export,dll)	static int CreateProxyStub(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOObject::Object** proxy, OOCore::Stub** stub, const char* dll_name); \
											extern "C" export int RegisterLib(bool bRegister) { \
											return CreateProxyStub((bRegister?2:3),0,OOObject::guid_t::NIL,0,OOObject::cookie_t(),0,0, #dll ); } \
											extern "C" export int CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::cookie_t& key, OOObject::Object** proxy) { \
                                            return CreateProxyStub(0,manager,iid,0,key,proxy,0,0); } \
                                            extern "C" export int CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOCore::Stub** stub) { \
                                            return CreateProxyStub(1,manager,iid,obj,key,0,stub,0); } \
                                            static int CreateProxyStub(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::cookie_t& key, OOObject::Object** proxy, OOCore::Stub** stub, const char* dll_name) { \
											if ((type==0 && proxy==0) || (type==1 && stub==0) || ((type==2 || type==3) && dll_name==0) || type<0 || type>3) { errno = EINVAL; ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1); } \
											if (type==0) *proxy=0; if (type==1) *stub=0;

#define PROXY_STUB_AUTO_ENTRY(t)			if (type==2) OOCore::RegisterProxyStub(t::IID, dll_name ); \
											else if (type==3) OOCore::UnregisterProxyStub(t::IID, dll_name ); \
											else if (iid==t::IID) { if (type==0) *proxy=OOCORE_PS_CREATE_AUTO_PROXY(t,manager,key); else if (type==1) *stub=OOCORE_PS_CREATE_AUTO_STUB(t,manager,key,obj); goto end;}

#define END_PROXY_STUB_MAP()				end: if ((type==0 && *proxy==0) || (type==1 && *stub==0)) { errno = ENOENT; ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Proxy/Stub create failed\n")),-1); } \
                                            if (type==0) (*proxy)->AddRef(); if (type==1) (*stub)->AddRef(); return 0; }


#endif // OOCORE_PROXYSTUB_MACROS_H_INCLUDED_

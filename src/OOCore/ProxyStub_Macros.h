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

// Helpers
#define OOCORE_PS_DECLARE_INVOKE_TABLE()				switch (method) { BOOST_PP_REPEAT(BOOST_PP_ADD(PROXY_STUB_MAX_METHODS,1),OOCORE_PS_DECLARE_INVOKE_TABLE_I,_) default:errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),method),-1); }
#define OOCORE_PS_DECLARE_INVOKE_TABLE_I(z,n,d)			case n:	return pT->invoke(boost::mpl::size_t< n >(),manager,iface,input,output);

#define OOCORE_PS_DECLARE_METHODINFO_TABLE()			switch (method) { BOOST_PP_REPEAT(BOOST_PP_ADD(PROXY_STUB_MAX_METHODS,1),OOCORE_PS_DECLARE_METHODINFO_TABLE_I,_) default:errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),method),-1); }
#define OOCORE_PS_DECLARE_METHODINFO_TABLE_I(z,n,d)		case n:	return pT->get_method_info(boost::mpl::size_t< n >(),method_name,param_count,attributes,wait_secs);

#define OOCORE_PS_DECLARE_PARAMINFO_TABLE()				switch (method) { BOOST_PP_REPEAT(BOOST_PP_ADD(PROXY_STUB_MAX_METHODS,1),OOCORE_PS_DECLARE_PARAMINFO_TABLE_I,_) default:errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),method),-1); }
#define OOCORE_PS_DECLARE_PARAMINFO_TABLE_I(z,n,d)		case n:	return pT->get_param_info(boost::mpl::size_t< n >(),param,param_name,type);

#define OOCORE_PS_DECLARE_PARAMATTR_TABLE()				switch (method) { BOOST_PP_REPEAT(BOOST_PP_ADD(PROXY_STUB_MAX_METHODS,1),OOCORE_PS_DECLARE_PARAMATTR_TABLE_I,_) default:errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),method),-1); }
#define OOCORE_PS_DECLARE_PARAMATTR_TABLE_I(z,n,d)		case n:	return pT->get_param_attrib(boost::mpl::size_t< n >(),param,data);
															
#define OOCORE_PS_METATYPE_BUILDER(t)					template <> class type_info_t< OOObject::t > { public: enum { value = OOCore::TypeInfo::t }; }

// IDL style param attribute macros
#define OOCORE_PS_PRM_ATTRIB_in						(0)
#define OOCORE_PS_PRM_ATTRIB_out					(1)
#define OOCORE_PS_PRM_ATTRIB_size_is				(2)
#define OOCORE_PS_PRM_ATTRIB_iid_is					(3)
#define OOCORE_PS_PRM_ATTRIB_string					(4)

#define OOCORE_PS_PRM_ATTRIB_IS(a,type)				BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_PRM_ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_PRM_ATTRIB_,type)))

// Parameter macros
#define OOCORE_PS_PARAM_ATTRIB(param)				BOOST_PP_SEQ_ELEM(0,param)
#define OOCORE_PS_PARAM_TYPE(param)					BOOST_PP_SEQ_ELEM(1,param)
#define OOCORE_PS_PARAM_VAR(param)					BOOST_PP_SEQ_ELEM(2,param)
#define OOCORE_PS_PARAM_NAME(param)					BOOST_PP_STRINGIZE(OOCORE_PS_PARAM_VAR(param))

#define OOCORE_PS_PARAM_ATTRIB_IS(param,type)		BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE(BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(OOCORE_PS_PARAM_ATTRIB_IS_I,type,OOCORE_PS_PARAM_ATTRIB(param)),99)),2)
#define OOCORE_PS_PARAM_ATTRIB_IS_I(s,type,a)		BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_PRM_ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_PRM_ATTRIB_,type)))

#define OOCORE_PS_PARAM_ATTRIB_ELEM_I(n,seq)		BOOST_PP_ARRAY_ELEM(BOOST_PP_ADD(n,1),BOOST_PP_SEQ_TO_ARRAY(seq))
#define OOCORE_PS_PARAM_ATTRIB_ELEM(param,type,n)	OOCORE_PS_PARAM_ATTRIB_ELEM_I(n,BOOST_PP_CAT(OOCORE_PS_PRM_ATTRIB_,BOOST_PP_SEQ_ELEM(0,BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(OOCORE_PS_PARAM_ATTRIB_IS_I,type,OOCORE_PS_PARAM_ATTRIB(param)),99))))

// Parameter enumeration macros
#define OOCORE_PS_PARSE_PARAMS(n,params,m)			BOOST_PP_REPEAT_FROM_TO(0,n,OOCORE_PS_PARSE_PARAMS_I,(params,m))
#define OOCORE_PS_PARSE_PARAMS_I(z,n,d)				BOOST_PP_TUPLE_ELEM(2,1,d)(n,BOOST_PP_SEQ_SUBSEQ(BOOST_PP_TUPLE_ELEM(2,0,d),BOOST_PP_MUL(n,3),3))

// ID generation
#define OOCORE_PS_GENERATE_UNIQUE_ID(fn,n)				OOCORE_PS_GENERATE_UNIQUE_ID_I(fn,n,__LINE__)
#define OOCORE_PS_GENERATE_UNIQUE_ID_I(fn,n,__LINE__)	OOCORE_PS_GENERATE_UNIQUE_ID_II(fn,n,__LINE__)
#define OOCORE_PS_GENERATE_UNIQUE_ID_II(fn,n,Line)		BOOST_PP_CAT(PROXYSTUB_ID_,BOOST_PP_CAT(fn,BOOST_PP_CAT(n,Line)))

// IDL style method attribute macros
#define OOCORE_PS_MTHD_ATTRIB_sync					(0)
#define OOCORE_PS_MTHD_ATTRIB_async					(1)
#define OOCORE_PS_MTHD_ATTRIB_wait					(100)

#define OOCORE_PS_MTHD_ATTRIB_IS(a,type)			BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_MTHD_ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_MTHD_ATTRIB_,type)))

// Method attribute macros
#define OOCORE_PS_METHOD_ATTRIB_IS(attr,type)		BOOST_PP_EQUAL(BOOST_PP_SEQ_SIZE(BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(OOCORE_PS_METHOD_ATTRIB_IS_I,type,attr),99)),2)
#define OOCORE_PS_METHOD_ATTRIB_IS_I(s,type,a)		BOOST_PP_EQUAL(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_MTHD_ATTRIB_,a)),BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_MTHD_ATTRIB_,type)))

#define OOCORE_PS_METHOD_ATTRIB_IS_FLAG(attr)		BOOST_PP_LESS(BOOST_PP_SEQ_ELEM(0,BOOST_PP_CAT(OOCORE_PS_MTHD_ATTRIB_,attr)),100)

#define OOCORE_PS_METHOD_ATTRIB_ELEM_I(n,seq)		BOOST_PP_ARRAY_ELEM(BOOST_PP_ADD(n,1),BOOST_PP_SEQ_TO_ARRAY(seq))
#define OOCORE_PS_METHOD_ATTRIB_ELEM(attr,type,n)	OOCORE_PS_METHOD_ATTRIB_ELEM_I(n,BOOST_PP_CAT(OOCORE_PS_MTHD_ATTRIB_,BOOST_PP_SEQ_ELEM(0,BOOST_PP_SEQ_PUSH_BACK(BOOST_PP_SEQ_FILTER(OOCORE_PS_METHOD_ATTRIB_IS_I,type,attr),99))))

// ID declaration - this is pure witchcraft!
#define OOCORE_PS_DECLARE_METHOD_ID(id,name)		public: typedef \
													BOOST_PP_ENUM(PROXY_STUB_MAX_METHODS,OOCORE_PS_DECLARE_METHOD_ID_I,name), \
													boost::mpl::size_t<PROXY_STUB_MAX_METHODS> \
													BOOST_PP_REPEAT(PROXY_STUB_MAX_METHODS,OOCORE_PS_DECLARE_METHOD_ID_II,name) \
													id; friend OOCore::Impl::used_t BOOST_PP_CAT(Method_Id_Gen_,name)(this_class*,id *);

#define OOCORE_PS_DECLARE_METHOD_ID_I(z,n,name)		boost::mpl::if_<boost::mpl::bool_<(sizeof(BOOST_PP_CAT(Method_Id_Gen_,name)((this_class*)0,(boost::mpl::size_t< n >*)0)) == sizeof(OOCore::Impl::unused_t))>, boost::mpl::size_t< n > 
#define OOCORE_PS_DECLARE_METHOD_ID_II(z,n,name)	>::type

// Proxy function declaration
#define OOCORE_PS_DECLARE_PROXY_FN(fn,n,params)		public: virtual OOObject::int32_t fn( OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_PROXY_PARAM_DECL) )
#define OOCORE_PS_PROXY_PARAM_DECL(n,param)			BOOST_PP_COMMA_IF(n) OOCORE_PS_PARAM_TYPE(param) OOCORE_PS_PARAM_VAR(param)

// Proxy function implementation
#define OOCORE_PS_PROXY_METHOD_ATTR(attr)			BOOST_PP_SEQ_FOR_EACH_I(OOCORE_PS_PROXY_METHOD_ATTR_I,0,attr)
#define OOCORE_PS_PROXY_METHOD_ATTR_I(r,d,n,attr)	BOOST_PP_EXPR_IF(n,|) \
													BOOST_PP_IF(OOCORE_PS_METHOD_ATTRIB_IS_FLAG(attr),OOCore::TypeInfo:: BOOST_PP_CAT(attr,_method),0 )

#define OOCORE_PS_PROXY_METHOD_WAIT(attr)			BOOST_PP_IF(OOCORE_PS_METHOD_ATTRIB_IS(attr,wait), \
														OOCORE_PS_METHOD_ATTRIB_ELEM(attr,wait,0), \
														DEFAULT_WAIT \
													)

#define OOCORE_PS_IMPL_PROXY_FN(id,n,seq,attr)		{ OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_DECL_IMPL) \
													OOCore::Impl::marshaller_t method_PROXY_FUNC(method(id::value,OOCORE_PS_PROXY_METHOD_ATTR(attr),OOCORE_PS_PROXY_METHOD_WAIT(attr) )); \
													(void)method_PROXY_FUNC; OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_IN_IMPL); \
													OOObject::int32_t method_PROXY_FUNC_RESULT = method_PROXY_FUNC.send_and_recv(); \
													OOCORE_PS_PARSE_PARAMS(n,seq,OOCORE_PS_PROXY_PARAM_OUT_IMPL) return method_PROXY_FUNC_RESULT; }

#define OOCORE_PS_PROXY_PARAM_DECL_IMPL(n,param)	BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
														OOCore::Impl::array_t < OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_ARRAY_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_COMMA() OOCORE_PS_PARAM_ATTRIB_ELEM(param,size_is,0) BOOST_PP_RPAREN();, \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string), \
															OOCore::Impl::string_t< OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_STRING_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_RPAREN();, \
															BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																OOCore::Impl::object_t< OOCORE_PS_PARAM_TYPE(param) > BOOST_PP_CAT(PROXY_OBJECT_,OOCORE_PS_PARAM_VAR(param)) BOOST_PP_LPAREN() OOCORE_PS_PARAM_VAR(param) BOOST_PP_COMMA() OOCORE_PS_PARAM_ATTRIB_ELEM(param,iid_is,0) BOOST_PP_RPAREN(); \
															) \
														) \
													)

#define OOCORE_PS_PROXY_PARAM_IN_IMPL(n,param)		BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,in), \
														method_PROXY_FUNC << BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
															BOOST_PP_CAT(PROXY_ARRAY_,OOCORE_PS_PARAM_VAR(param)), \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string), \
																BOOST_PP_CAT(PROXY_STRING_,OOCORE_PS_PARAM_VAR(param)), \
																BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																	BOOST_PP_CAT(PROXY_OBJECT_,OOCORE_PS_PARAM_VAR(param)), \
																	OOCORE_PS_PARAM_VAR(param) \
																) \
															) \
														); \
													) 

#define OOCORE_PS_PROXY_PARAM_OUT_IMPL(n,param)		BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,out), \
														method_PROXY_FUNC >> BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
															BOOST_PP_CAT(PROXY_ARRAY_,OOCORE_PS_PARAM_VAR(param)), \
															BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string), \
																BOOST_PP_CAT(PROXY_STRING_,OOCORE_PS_PARAM_VAR(param)), \
																BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																	BOOST_PP_CAT(PROXY_OBJECT_,OOCORE_PS_PARAM_VAR(param)), \
																	OOCORE_PS_PARAM_VAR(param) \
																) \
															) \
														); \
													)

// Stub function declaration
#define OOCORE_PS_DECLARE_STUB_FN(id)				private: int invoke(const id&, OOCore::ProxyStubManager* manager, iface_class* obj, OOCore::InputStream_Wrapper& input, OOCore::OutputStream_Wrapper& output )

// Stub function implementation
#define OOCORE_PS_IMPL_STUB_FN(fn,n,params)			{ OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_DECL_IMPL) \
													OOObject::int32_t ret_code=obj->fn( OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_CALL_IMPL) ); \
													if (output) { if (output.write(ret_code) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write return code\n")),-1); \
													if (ret_code!=0) { if (output->WriteLong(errno) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write errno\n")),-1); } \
													else { OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_STUB_PARAM_OUT_IMPL) } } return 0; }

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
															BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
																(OOCORE_PS_PARAM_ATTRIB_ELEM(param,size_is,0)) \
															) \
														) \
													);
										
#define OOCORE_PS_STUB_PARAM_CALL_IMPL(n,param)		BOOST_PP_COMMA_IF(n) OOCORE_PS_PARAM_VAR(param)

#define OOCORE_PS_STUB_PARAM_OUT_IMPL(n,param)		BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,out), \
														if (OOCORE_PS_PARAM_VAR(param).respond \
														BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
															(output,manager), \
															(output) \
														) \
														!=0 ) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write out param\n")),-1); \
													)

// MethodInfo function declaration
#define OOCORE_PS_DECLARE_PARAMINFO_SWITCH(n,params)	switch (param) { OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_DECLARE_PARAMINFO_SWITCH_I) case -1: default: errno=EINVAL;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid param id %d\n"),param),-1); }
#define OOCORE_PS_DECLARE_PARAMINFO_SWITCH_I(n,param)	case n:	*param_name = OOCORE_PS_PARAM_NAME(param); OOCORE_PS_DECLARE_PARAMINFO_IF(param) return 0;

#define OOCORE_PS_DECLARE_PARAMINFO_IF(param)			*type = BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
															OOCore::TypeInfo::Object, \
															OOCore::Impl::type_info_t< OOCORE_PS_PARAM_TYPE(param) >::value \
														) | OOCORE_PS_PARAMINFO_ATTRIB(param);

#define OOCORE_PS_PARAMINFO_ATTRIB(param)				BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,in),OOCore::TypeInfo::in,0) \
														BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,out),| OOCore::TypeInfo::out ) \
														BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,string),| OOCore::TypeInfo::string ) \
														BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is),| OOCore::TypeInfo::array )

#define OOCORE_PS_DECLARE_PARAMINFO_ATTRIB_SRCH(n,params)		OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_DECLARE_PARAMINFO_ATTRIB_SRCH_I) \
																switch (param) { \
																OOCORE_PS_PARSE_PARAMS(n,params,OOCORE_PS_DECLARE_PARAMINFO_ATTRIB_SRCH_II) \
																case -1: default: return -1; }

#define OOCORE_PS_DECLARE_PARAMINFO_ATTRIB_SRCH_I(n,param)		OOCore::Impl::param_t< \
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
																BOOST_PP_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																	(OOCORE_PS_PARAM_ATTRIB_ELEM(param,iid_is,0)), \
																	BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
																		(OOCORE_PS_PARAM_ATTRIB_ELEM(param,size_is,0)) \
																	) \
																);

#define OOCORE_PS_DECLARE_PARAMINFO_ATTRIB_SRCH_II(n,param)		BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,iid_is), \
																	case n: return unpack_iid_attr(method_tag__,data,OOCORE_PS_PARAM_ATTRIB_ELEM(param,iid_is,0),BOOST_PP_STRINGIZE(OOCORE_PS_PARAM_ATTRIB_ELEM(param,iid_is,0))); ) \
																BOOST_PP_EXPR_IF(OOCORE_PS_PARAM_ATTRIB_IS(param,size_is), \
																	case n: return unpack_iid_attr(method_tag__,data,OOObject::guid_t::NIL,BOOST_PP_STRINGIZE(OOCORE_PS_PARAM_ATTRIB_ELEM(param,size_is,0))); )

#define OOCORE_PS_DECLARE_METHODINFO_FN(id,attribute,fn,n,params)	private: int get_method_info(const id&, const OOObject::char_t** method_name, size_t* param_count, OOCore::TypeInfo::Method_Attributes_t* attributes, OOObject::uint16_t* wait_secs) \
																	{ *method_name = #fn; *param_count = n; *wait_secs = OOCORE_PS_PROXY_METHOD_WAIT(attribute); return 0; } \
																	int get_param_info(const id&, size_t param, const OOObject::char_t** param_name, OOCore::TypeInfo::Type_t* type) \
																	{ OOCORE_PS_DECLARE_PARAMINFO_SWITCH(n,params) } \
																	int get_param_attrib(const id&, size_t param, OOCore::TypeInfo::Param_Attrib_Data_t* data) \
																	{ const size_t method_tag__ = id::value; (void)method_tag__; OOCORE_PS_DECLARE_PARAMINFO_ATTRIB_SRCH(n,params) }

// Method declaration macros
#define OOCORE_PS_METHOD_I(fn,n,params,id,attr)		OOCORE_PS_DECLARE_METHOD_ID(id,PS) \
													OOCORE_PS_DECLARE_METHODINFO_FN(id,attr,fn,n,params) \
													OOCORE_PS_DECLARE_PROXY_FN(fn,n,params) OOCORE_PS_IMPL_PROXY_FN(id,n,params,attr) \
													OOCORE_PS_DECLARE_STUB_FN(id) OOCORE_PS_IMPL_STUB_FN(fn,n,params)

// Default fn implementation
#define OOCORE_PS_DECLARE_ADDREF()				OOCORE_PS_DECLARE_ADDREF_I(OOCORE_PS_GENERATE_UNIQUE_ID(AddRef,0))
#define OOCORE_PS_DECLARE_ADDREF_I(id)			OOCORE_PS_DECLARE_METHOD_ID(id,PS) OOCORE_PS_DECLARE_METHODINFO_FN(id,(sync),AddRef,0,() )

#define OOCORE_PS_DECLARE_RELEASE()				OOCORE_PS_DECLARE_RELEASE_I(OOCORE_PS_GENERATE_UNIQUE_ID(Release,0))
#define OOCORE_PS_DECLARE_RELEASE_I(id)			OOCORE_PS_DECLARE_METHOD_ID(id,PS) OOCORE_PS_DECLARE_METHODINFO_FN(id,(async),Release,0,() ) OOObject::int32_t Release() { return Release_i(id::value,false); } \
												OOCORE_PS_DECLARE_STUB_FN(id) { return Release_i(id::value,true); }

#define OOCORE_PS_DECLARE_QI()					OOCORE_PS_DECLARE_QI_I(OOCORE_PS_GENERATE_UNIQUE_ID(QueryInterface,2))
#define OOCORE_PS_DECLARE_QI_I(id)				OOCORE_PS_DECLARE_METHOD_ID(id,PS) OOCORE_PS_DECLARE_METHODINFO_FN(id,(sync),QueryInterface,2,((in))(const OOObject::guid_t&)(iid)((out)(iid_is(iid)))(OOObject::Object**)(ppVal)) OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal) { return QueryInterface_i(id::value,iid,ppVal); } \
												OOCORE_PS_DECLARE_STUB_FN(id) { OOCore::Impl::param_t<const OOObject::guid_t&> iid(input); \
												if (iid.failed()) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to read in param\n")),-1); \
												OOCore::Impl::param_t<OOCore::Impl::object_t<OOObject::Object**> > ppVal(iid); \
												OOObject::int32_t ret_code=QueryInterface_i(id::value,iid,ppVal); \
												if (output) { if (output.write(ret_code) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write return code\n")),-1); \
												if (ret_code!=0) { if (output->WriteLong(errno) != 0) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write errno\n")),-1); } \
												else { if (ppVal.respond(output,manager)!=0 ) ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Failed to write out param\n")),-1); } } return 0; }

#define OOCORE_PS_END_TYPEINFO()				OOCORE_PS_END_TYPEINFO_I(OOCORE_PS_GENERATE_UNIQUE_ID(EndTypeInfo__Marker,0))
#define OOCORE_PS_END_TYPEINFO_I(id)			OOCORE_PS_DECLARE_METHOD_ID(id,PS) int GetMetaInfo(const OOObject::char_t** type_name, size_t* method_count) { if (method_count==0) { errno=EINVAL; ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid NULL pointer\n")),-1); } if (get_type_name(type_name)!=0) return -1; *method_count = id::value; return 0; }

// Method declaration macros
#define METHOD(fn,n,p)								METHOD_EX((sync),fn,n,p)
#define METHOD_EX(attr,fn,n,p)						OOCORE_PS_METHOD_I(fn,n,BOOST_PP_TUPLE_TO_SEQ(BOOST_PP_MUL(n,3),p),OOCORE_PS_GENERATE_UNIQUE_ID(fn,n),attr)

// Interface meta-information declaration macros
#define BEGIN_META_INFO(iface)						OOCORE_PS_BEGIN_PROXY_I(iface,BOOST_PP_CAT(iface,_MetaInfo__))
#define OOCORE_PS_BEGIN_PROXY_I(iface,name)			class name : public OOCore::ProxyStub_Impl<iface> { \
													friend class OOCore::Impl::metainfo_t; \
													template <class T> int invoke(const T&, OOCore::ProxyStubManager* manager, iface* obj, OOCore::InputStream_Wrapper& input, OOCore::OutputStream_Wrapper& output ) { errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),T::value),-1); } \
													template <class T> int get_method_info(const T&, const OOObject::char_t** method_name, size_t* param_count, OOCore::TypeInfo::Method_Attributes_t* attributes, OOObject::uint16_t* wait_secs) { errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),T::value),-1); } \
													template <class T> int get_param_info(const T&, size_t param, const OOObject::char_t** param_name, OOCore::TypeInfo::Type_t* type) { errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),T::value),-1); } \
													template <class T> int get_param_attrib(const T&, size_t param, OOCore::TypeInfo::Param_Attrib_Data_t* data) { errno=ENOSYS;ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid method id %d\n"),T::value),-1); } \
													name(OOCore::ProxyStubManager* manager, const OOObject::uint32_t& key, iface* obj) : OOCore::ProxyStub_Impl<iface>(manager,key,obj) {} \
													name(OOCore::ProxyStubManager* manager, const OOObject::uint32_t& key) : OOCore::ProxyStub_Impl<iface>(manager,key) {} \
													name() : OOCore::ProxyStub_Impl<iface>() {} \
													friend OOCore::Impl::unused_t BOOST_PP_CAT(Method_Id_Gen_,PS)(name*,...); \
													int get_type_name(const OOObject::char_t** type_name) { if (type_name==0) { errno=EINVAL; ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("Invalid NULL pointer\n")),-1); } *type_name = #iface; return 0; } \
													int Invoke_i(iface* obj, OOObject::uint32_t& method, OOCore::Object_Ptr<OOCore::ProxyStubManager>& manager, OOCore::InputStream_Wrapper input, OOCore::OutputStream_Wrapper output) { \
													return OOCore::Impl::metainfo_t::Invoke(this,obj,manager,method,input,output); } \
													int GetMethodInfo(size_t method, const OOObject::char_t** method_name, size_t* param_count, OOCore::TypeInfo::Method_Attributes_t* attributes, OOObject::uint16_t* wait_secs) { \
													return OOCore::Impl::metainfo_t::GetMethodInfo(this,method,method_name,param_count,attributes,wait_secs); } \
													int GetParamInfo(size_t method, size_t param, const OOObject::char_t** param_name, OOCore::TypeInfo::Type_t* type) { \
													return OOCore::Impl::metainfo_t::GetParamInfo(this,method,param,param_name,type); } \
													int GetParamAttributeData(size_t method, size_t param, OOCore::TypeInfo::Param_Attrib_Data_t* data) { \
													return OOCore::Impl::metainfo_t::GetParamAttributeData(this,method,param,data); } \
													public: typedef name this_class; typedef iface iface_class; \
													static iface* create_proxy(OOCore::ProxyStubManager* manager, const OOObject::uint32_t& key) { name* proxy; ACE_NEW_RETURN(proxy,this_class(manager,key),0); return proxy;} \
													static OOCore::Stub* create_stub(OOCore::ProxyStubManager* manager, const OOObject::uint32_t& key, iface* obj) { name* stub; ACE_NEW_RETURN(stub,this_class(manager,key,obj),0); return stub;} \
													static OOCore::TypeInfo* create_typeinfo() { name* typeinfo; ACE_NEW_RETURN(typeinfo,this_class,0); return typeinfo;} \
													OOCORE_PS_DECLARE_ADDREF() OOCORE_PS_DECLARE_RELEASE() OOCORE_PS_DECLARE_QI()
												
#define END_META_INFO()								OOCORE_PS_END_TYPEINFO() };

#define OOCORE_PS_CREATE_STUB(iface,manager,key,obj)	BOOST_PP_CAT(iface,_MetaInfo__::create_stub(manager,key,static_cast<iface*>(obj)))
#define OOCORE_PS_CREATE_PROXY(iface,manager,key) 		BOOST_PP_CAT(iface,_MetaInfo__::create_proxy(manager,key))
#define OOCORE_PS_CREATE_TYPEINFO(iface) 				BOOST_PP_CAT(iface,_MetaInfo__::create_typeinfo())

// Meta info map declaration macros
#define BEGIN_META_INFO_MAP(lib)			BEGIN_META_INFO_MAP_EX(lib,lib)
#define BEGIN_META_INFO_MAP_EX(exp,lib)		static int CreateProxyStub(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::uint32_t& key, OOObject::Object** proxy, OOCore::Stub** stub, OOCore::TypeInfo** typeinfo, const char* dll_name); \
											extern "C" exp##_Export int RegisterLib(bool bRegister) { \
											return CreateProxyStub((bRegister?2:3),0,OOObject::guid_t::NIL,0,0,0,0,0,#lib ); } \
											extern "C" exp##_Export int CreateProxy(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, const OOObject::uint32_t& key, OOObject::Object** proxy) { \
                                            return CreateProxyStub(0,manager,iid,0,key,proxy,0,0,0); } \
                                            extern "C" exp##_Export int CreateStub(OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::uint32_t& key, OOCore::Stub** stub) { \
                                            return CreateProxyStub(1,manager,iid,obj,key,0,stub,0,0); } \
											extern "C" exp##_Export int GetTypeInfo(const OOObject::guid_t& iid, OOCore::TypeInfo** typeinfo) { \
                                            return CreateProxyStub(5,0,iid,0,0,0,0,typeinfo,0); } static OOCORE_PS_BEGIN_PROXY_STUB_MAP(CreateProxyStub)

#define OOCORE_PS_BEGIN_PROXY_STUB_MAP(fn)	int fn(int type, OOCore::ProxyStubManager* manager, const OOObject::guid_t& iid, OOObject::Object* obj, const OOObject::uint32_t& key, OOObject::Object** proxy, OOCore::Stub** stub, OOCore::TypeInfo** typeinfo, const char* dll_name) { \
											if ((type==0 && proxy==0) || (type==1 && stub==0) || ((type==2 || type==3 || type==4) && dll_name==0) || (type==5 && typeinfo==0) ) { errno = EINVAL; ACE_ERROR_RETURN((LM_ERROR,ACE_TEXT("(%P|%t) Invalid NULL pointer\n")),-1); } \
											if (type==0) *proxy=0; if (type==1) *stub=0; if (type==5) *typeinfo=0;

#define OOCORE_PS_ENTRY_SHIM(t)				if (type==4) reinterpret_cast<OOCore::Impl::Proxy_Stub_Factory*>(const_cast<char*>(dll_name))->m_dll_map.insert(std::map<OOObject::guid_t,proxystub_node*>::value_type(t::IID,&OOCore::Impl::Proxy_Stub_Factory::m_core_node)); else META_INFO_ENTRY(t)

#define META_INFO_ENTRY(t)					if (type==2) OOCore::RegisterProxyStub(t::IID, dll_name ); \
											else if (type==3) OOCore::UnregisterProxyStub(t::IID, dll_name ); \
											else if (iid==t::IID) { \
											if (type==0) *proxy=OOCORE_PS_CREATE_PROXY(t,manager,key); \
											else if (type==1) *stub=OOCORE_PS_CREATE_STUB(t,manager,key,obj); \
											else if (type==5) *typeinfo=OOCORE_PS_CREATE_TYPEINFO(t); \
											goto end; }

#define END_META_INFO_MAP()					end: if ((type==0 && *proxy==0) || (type==1 && *stub==0) || (type==5 && *typeinfo==0)) { errno = ENOENT; ACE_ERROR_RETURN((LM_DEBUG,ACE_TEXT("(%P|%t) Proxy/Stub create failed\n")),-1); } \
                                            if (type==0) (*proxy)->AddRef(); if (type==1) (*stub)->AddRef(); if (type==5) (*typeinfo)->AddRef(); return 0; }


#endif // OOCORE_PROXYSTUB_MACROS_H_INCLUDED_

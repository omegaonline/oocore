

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 6.00.0366 */
/* at Sat Dec 31 15:58:47 2005
 */
/* Compiler settings for .\OOInteropCOM.idl:
    Oicf, W1, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __OOInteropCOM_h__
#define __OOInteropCOM_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __ITest_Object_FWD_DEFINED__
#define __ITest_Object_FWD_DEFINED__
typedef interface ITest_Object ITest_Object;
#endif 	/* __ITest_Object_FWD_DEFINED__ */


#ifndef __Test_Object_FWD_DEFINED__
#define __Test_Object_FWD_DEFINED__

#ifdef __cplusplus
typedef class Test_Object Test_Object;
#else
typedef struct Test_Object Test_Object;
#endif /* __cplusplus */

#endif 	/* __Test_Object_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 

void * __RPC_USER MIDL_user_allocate(size_t);
void __RPC_USER MIDL_user_free( void * ); 

#ifndef __ITest_Object_INTERFACE_DEFINED__
#define __ITest_Object_INTERFACE_DEFINED__

/* interface ITest_Object */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_ITest_Object;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("AE012905-C1E7-4BAA-AD9C-7659029B10E4")
    ITest_Object : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct ITest_ObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            ITest_Object * This,
            /* [in] */ REFIID riid,
            /* [iid_is][out] */ void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            ITest_Object * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            ITest_Object * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            ITest_Object * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            ITest_Object * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            ITest_Object * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            ITest_Object * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } ITest_ObjectVtbl;

    interface ITest_Object
    {
        CONST_VTBL struct ITest_ObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define ITest_Object_QueryInterface(This,riid,ppvObject)	\
    (This)->lpVtbl -> QueryInterface(This,riid,ppvObject)

#define ITest_Object_AddRef(This)	\
    (This)->lpVtbl -> AddRef(This)

#define ITest_Object_Release(This)	\
    (This)->lpVtbl -> Release(This)


#define ITest_Object_GetTypeInfoCount(This,pctinfo)	\
    (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo)

#define ITest_Object_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo)

#define ITest_Object_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)

#define ITest_Object_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __ITest_Object_INTERFACE_DEFINED__ */



#ifndef __OOInteropCOMLib_LIBRARY_DEFINED__
#define __OOInteropCOMLib_LIBRARY_DEFINED__

/* library OOInteropCOMLib */
/* [helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_OOInteropCOMLib;

EXTERN_C const CLSID CLSID_Test_Object;

#ifdef __cplusplus

class DECLSPEC_UUID("6EDC3D99-2624-4967-82C4-0BD84B343854")
Test_Object;
#endif
#endif /* __OOInteropCOMLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif



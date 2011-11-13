///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOCOMInterop, the Omega Online COM Interop library.
//
// OOCOMInterop is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCOMInterop is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCOMInterop.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "disp_nspace.h"
#include "disp_obj.h"

namespace
{
	LONG s_moduleLockCount = 0;
	OTL::ObjectPtr<Omega::Compartment::ICompartment> m_ptrCompt;
	LONG s_bInitialised = 0;
	Omega::string_t s_init_args;
	
	void IncModuleLockCount()
	{
		InterlockedIncrement(&s_moduleLockCount);
	}

	void DecModuleLockCount()
	{
		if (InterlockedDecrement(&s_moduleLockCount) == 0)
		{
			m_ptrCompt.Release();
			
			if (InterlockedCompareExchange(&s_bInitialised,1,0) == 1)
				Omega::Uninitialize();
		}
	}
	
	Omega::string_t GetDesc(Omega::IException* pE)
	{
		Omega::string_t desc = pE->GetDescription();
		
		OTL::ObjectPtr<Omega::IInternalException> ptrInternal = OTL::QueryInterface<Omega::IInternalException>(pE);
		if (ptrInternal)
			desc += L" at " + ptrInternal->GetSource();
		
		OTL::ObjectPtr<Omega::IException> ptrCause = pE->GetCause();
		if (ptrCause)
			desc += L". Cause: " + GetDesc(ptrCause);
			
		return desc;
	}	
}

class ClassFactory : public IClassFactory
{
public:
	ClassFactory() : m_refCount(1)
	{ 
		IncModuleLockCount();
	}

	virtual ~ClassFactory()
	{
		DecModuleLockCount();
	}

private:
	LONG m_refCount;

public:
	STDMETHOD_(ULONG,AddRef)()
	{
		return InterlockedIncrement(&m_refCount);
	}

	STDMETHOD_(ULONG,Release)()
	{
		ULONG ret = InterlockedDecrement(&m_refCount);
		if (ret == 0)
			delete this;
		return ret;
	}

	STDMETHOD(QueryInterface)(REFIID riid, LPVOID* ppvObject)
	{
		if (riid == IID_IUnknown ||
			riid == IID_IClassFactory)
		{
			AddRef();
			*ppvObject = this;
			return S_OK;
		}
		return E_NOINTERFACE;
	}

	STDMETHOD(CreateInstance)(IUnknown *pUnkOuter, REFIID riid, void **ppvObject);
	
	STDMETHOD(LockServer)(BOOL fLock)
	{
		if (fLock)
			IncModuleLockCount();
		else
			DecModuleLockCount();

		return S_OK;
	}
};

HRESULT CreateClassFactory(void** ppv)
{
	*ppv = new (std::nothrow) ClassFactory();
	if (!*ppv)
		return E_OUTOFMEMORY;
	
	return S_OK;
}

bool CanUnloadNow()
{
	return InterlockedCompareExchange(&s_moduleLockCount,0,0) == 0;
}

IDispatchImpl::IDispatchImpl() : m_refCount(1)
{
	IncModuleLockCount();
}

IDispatchImpl::~IDispatchImpl()
{
	DecModuleLockCount();
}

static HRESULT GetInitArgs(LCID /*lcid*/, VARIANT* pVarResult, EXCEPINFO* /*pExcepInfo*/)
{
	pVarResult->vt = VT_BSTR;
	pVarResult->bstrVal = SysAllocString(s_init_args.c_wstr());
	
	return S_OK;
}

static HRESULT PutInitArgs(LCID lcid, DISPPARAMS* pDispParams, UINT* puArgErr, EXCEPINFO* pExcepInfo, bool byRef)
{
	VARIANTARG v;
	VariantInit(&v);
	
	HRESULT hr = VariantChangeTypeEx(&v,&pDispParams->rgvarg[0],lcid,0,VT_BSTR);
	if (hr != S_OK)
	{
		if (puArgErr)
			*puArgErr = 0;
		return hr;
	}
		
	s_init_args = Omega::string_t(v.bstrVal,size_t(-1),true);
	
	return S_OK;
}

static HRESULT CreateInstance(LCID lcid, DISPPARAMS* pDispParams, VARIANT* pVarResult, UINT* puArgErr, EXCEPINFO* pExcepInfo)
{
	if (!pDispParams || !pVarResult)
		return E_POINTER;
		
	if (pDispParams->cArgs > 2  ||
			pDispParams->cArgs < 1)
	{
		return DISP_E_BADPARAMCOUNT;
	}	
	
	if (InterlockedCompareExchange(&s_bInitialised,0,1) == 0)
	{
		Omega::IException* pE = Omega::Initialize(s_init_args);
		if (pE)
			return FillExcepInfo(L"CreateInstance",pE,pExcepInfo);
					
		try
		{
			m_ptrCompt = OTL::ObjectPtr<Omega::Compartment::ICompartment>(Omega::Compartment::OID_Compartment);
		}
		catch (Omega::IException* pE2)
		{
			return FillExcepInfo(L"CreateInstance",pE2,pExcepInfo);
		}			
	}
	
	HRESULT hr = VariantClear(pVarResult);
	if (hr != S_OK)
		return hr;
		
	// See http://msdn.microsoft.com/en-us/library/ms221653.aspx for details of DISPPARAMS
	// Key points: args are in reverse order (right to left)
	//             unnamed args processed before named args
	
	Omega::any_t values[2];
	values[1] = Omega::Activation::Default;	
	
	UINT arg = pDispParams->cArgs;
	for (;arg > pDispParams->cNamedArgs;--arg)
	{
		hr = variant_to_any(lcid,pDispParams->rgvarg[arg-1],values[arg-1]);
		if (hr != S_OK)
		{
			if (puArgErr)
				*puArgErr = arg-1;
			return hr;
		}
	}
	
	for (;arg > 0;--arg)
	{
		UINT idx = 0;
		for (; idx < pDispParams->cNamedArgs; ++idx)
		{
			if (pDispParams->rgdispidNamedArgs[idx] == static_cast<INT>(arg-1))
				break;
		}
		
		if (idx != pDispParams->cNamedArgs)
		{
			hr = variant_to_any(lcid,pDispParams->rgvarg[arg-1],values[arg-1]);
			if (hr != S_OK)
			{
				if (puArgErr)
					*puArgErr = arg-1;
				return hr;
			}
		}
		else
		{
			if (puArgErr)
				*puArgErr = arg-1;
			
			return DISP_E_PARAMNOTFOUND;
		}
	}
	
	Omega::IObject* pObj = NULL;
	m_ptrCompt->CreateInstance(values[0],values[1].cast<Omega::Activation::Flags_t>(),NULL,OMEGA_GUIDOF(Omega::TypeInfo::IProvideObjectInfo),pObj);
	OTL::ObjectPtr<Omega::TypeInfo::IProvideObjectInfo> ptrPOI = static_cast<Omega::TypeInfo::IProvideObjectInfo*>(pObj);

	if (!ptrPOI)
	{
		pExcepInfo->wCode = 1002;
		pExcepInfo->wReserved = 0;
		pExcepInfo->bstrSource = NULL;
		pExcepInfo->bstrDescription = NULL;
		pExcepInfo->bstrHelpFile = NULL;
		pExcepInfo->dwHelpContext = 0;
		pExcepInfo->pvReserved = NULL;
		pExcepInfo->pfnDeferredFillIn = NULL;
		pExcepInfo->scode = 0;
		pExcepInfo->bstrSource = SysAllocString(L"CreateInstance");
		pExcepInfo->bstrDescription = SysAllocString(L"Created object is not suitable for scripting");
		
		return DISP_E_EXCEPTION;
	}
	
	Omega::TypeInfo::IProvideObjectInfo::iid_list_t iids = ptrPOI->EnumInterfaces();
	if (iids.empty())
		return E_NOINTERFACE;
		
	OTL::ObjectPtr<Omega::IObject> ptrObj = ptrPOI->QueryInterface(iids.front());
	if (!ptrObj)
		return E_NOINTERFACE;
	
	OTL::ObjectPtr<Omega::Remoting::IProxy> ptrProxy = Omega::Remoting::GetProxy(ptrObj);
	
	IDispatchObjImpl* pDisp = new (std::nothrow) IDispatchObjImpl(ptrProxy,iids.front());
	if (!pDisp)
		return E_OUTOFMEMORY;
		
	hr = pDisp->init();
	if (hr != S_OK)
	{
		pDisp->Release();
		return hr;
	}
	
	pVarResult->pdispVal = pDisp;	
	pVarResult->vt = VT_DISPATCH;
	
	return S_OK;
}

static const wchar_t* CreateInstance_names[] = { L"CreateInstance",L"oid",L"flags",NULL };
static const wchar_t* InitializeArgs_names[] = { L"InitializeArgs",NULL };

static const NSpaceDefn Omega_nspace[] = 
{
	{ CreateInstance_names, &CreateInstance, NULL, NULL },
	{ InitializeArgs_names, NULL, &GetInitArgs, &PutInitArgs },
	{ NULL, 0, NULL }
};

STDMETHODIMP ClassFactory::CreateInstance(IUnknown *pUnkOuter, REFIID riid, void **ppvObject)
{
	if (pUnkOuter)
		return CLASS_E_NOAGGREGATION;
		
	if (!ppvObject)
		return E_POINTER;
		
	if (riid != IID_IDispatch && riid != IID_IUnknown)
		return E_NOINTERFACE;
		
	*ppvObject = new (std::nothrow) IDispatchNSpaceImpl(Omega_nspace);
	if (!*ppvObject)
		return E_OUTOFMEMORY;
	
	return S_OK;
}

HRESULT FillExcepInfo(const wchar_t* wszSource, Omega::IException* pE, EXCEPINFO* pExcepInfo)
{
	pExcepInfo->wCode = 1001;
	pExcepInfo->wReserved = 0;
	pExcepInfo->bstrSource = NULL;
	pExcepInfo->bstrDescription = NULL;
	pExcepInfo->bstrHelpFile = NULL;
	pExcepInfo->dwHelpContext = 0;
	pExcepInfo->pvReserved = NULL;
	pExcepInfo->pfnDeferredFillIn = NULL;
	pExcepInfo->scode = 0;
	pExcepInfo->bstrSource = SysAllocString(wszSource);
	pExcepInfo->bstrDescription = SysAllocString(L"Omega exception thrown");
	
	try
	{
		Omega::string_t desc = GetDesc(pE);
		
		SysFreeString(pExcepInfo->bstrDescription);
		pExcepInfo->bstrDescription = SysAllocString(desc.c_wstr());
	}
	catch (Omega::IException* pE2)
	{
		pE2->Release();
	}
	
	pE->Release();
	return DISP_E_EXCEPTION;
}

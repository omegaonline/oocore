#pragma once

class CCOMClassFactory : public CComClassFactory
{
public:
	CCOMClassFactory(void);
	virtual ~CCOMClassFactory(void);

	static HRESULT GetClassObject(REFCLSID rclsid, REFIID riid, LPVOID* ppv);

private:
	OOObject::guid_t						m_OO_clsid;
	OOObject::guid_t						m_OO_iid;
	OOCore::Object_Ptr<OOCore::TypeInfo>	m_ptr_OO_TypeInfo;
	
	HRESULT init(REFCLSID rclsid);

// IClassFactory members
public:
	STDMETHOD(CreateInstance)(LPUNKNOWN pUnkOuter, REFIID riid, void** ppvObj);
};

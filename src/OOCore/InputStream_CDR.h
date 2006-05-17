//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "OOCore.h" instead
//
//////////////////////////////////////////////////////

#ifndef OOCORE_INPUTSTREAM_CDR_H_INCLUDED_
#define OOCORE_INPUTSTREAM_CDR_H_INCLUDED_

#include "./OOUtil.h"

namespace OOCore
{
namespace Impl
{

class InputStream_CDR : 
	public OOUtil::Object_Root<InputStream_CDR>,
	public OOObject::InputStream,
	public ACE_InputCDR
{
	friend class OutputStream_CDR;

public:
	HAS_IID;

BEGIN_INTERFACE_MAP(InputStream_CDR)
	INTERFACE_ENTRY(OOObject::InputStream)
	INTERFACE_ENTRY(InputStream_CDR)
END_INTERFACE_MAP()

protected:
	InputStream_CDR(const ACE_InputCDR& in);
	virtual ~InputStream_CDR() {};

private:
	template <class T>
	int ReadVar(T& val)
	{
		return ((*this >> val) ? 0 : -1);
	}

// OOObject::InputStream members
public:
	int ReadBoolean(OOObject::bool_t& val);
	int ReadChar(OOObject::char_t& val);
	int ReadByte(OOObject::byte_t& val);
	int ReadShort(OOObject::int16_t& val);
	int ReadUShort(OOObject::uint16_t& val);
	int ReadLong(OOObject::int32_t& val);
	int ReadULong(OOObject::uint32_t& val);
	int ReadLongLong(OOObject::int64_t& val);
	int ReadULongLong(OOObject::uint64_t& val);
	int ReadFloat(OOObject::real4_t& val);
	int ReadDouble(OOObject::real8_t& val);
};

};
};

namespace OOUtil
{
	// Specialisation so we can use the constructor
	template <>
	class Object<OOCore::Impl::InputStream_CDR> : public OOCore::Impl::InputStream_CDR
	{
	public:
		static int CreateObject(const ACE_InputCDR& in, OOUtil::Object<OOCore::Impl::InputStream_CDR>*& pObject)
		{
			ACE_NEW_RETURN(pObject,OOUtil::Object<OOCore::Impl::InputStream_CDR>(in),-1);

			int res = pObject->FinalConstruct();
			if (res != 0)
			{
				delete pObject;
				pObject = 0;
			}
			return res;
		}

	private:
		Object(const ACE_InputCDR& in) : OOCore::Impl::InputStream_CDR(in)
		{ }

	// OOObject::Object members
	public:
		virtual OOObject::int32_t AddRef()
		{
			return Internal_AddRef();
		}

		virtual OOObject::int32_t Release()
		{
			return Internal_Release();
		}

		virtual OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			return Internal_QueryInterface(iid,ppVal);
		}
	};

};

#endif // OOCORE_INPUTSTREAM_CDR_H_INCLUDED_

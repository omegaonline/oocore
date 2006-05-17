#ifndef OOCORE_UTIL_H_INCLUDED_
#define OOCORE_UTIL_H_INCLUDED_

#include <ace/Atomic_Op.h>
#include <ace/SString.h>

#include "./OOObject.h"

namespace OOUtil
{
	OOObject::int32_t GetTypeInfo(const OOObject::guid_t& iid, OOObject::TypeInfo** ppTI)
	{
		return OOObject::GetMetaInfo(iid,OOObject::TypeInfo::IID,reinterpret_cast<OOObject::Object**>(ppTI));
	}

	template <class OBJECT>
	class Object_Ptr
	{
	public:
		Object_Ptr(OBJECT* obj = 0) :
			m_ptr(obj)
		{
			if (m_ptr)
				m_ptr->AddRef();
		}

		Object_Ptr(const Object_Ptr<OBJECT>& rhs) :
			m_ptr(rhs.m_ptr)
		{
			if (m_ptr)
				m_ptr->AddRef();
		}

		virtual ~Object_Ptr()
		{
			if (m_ptr)
				m_ptr->Release();
		}

		Object_Ptr& operator = (const Object_Ptr<OBJECT>& rhs)
		{
			return (*this) = rhs.m_ptr;
		}

		OOObject::int32_t CreateObject(const OOObject::guid_t& oid, OOObject::ObjectFactory::Flags_t flags = OOObject::ObjectFactory::ANY, OOObject::Object* pOuter = 0)
		{
			if (m_ptr)
				m_ptr->Release();

			return OOObject::CreateObject(oid,flags,pOuter,T::IID,reinterpret_cast<OOObject::Object**>(&m_ptr));
		}

		Object_Ptr clear()
		{
			Object_Ptr<OBJECT> old;

#if defined(WIN32)
			old = reinterpret_cast<OBJECT*>(static_cast<LONG_PTR>(::InterlockedExchange(reinterpret_cast<LONG*>(&m_ptr), 0)));
#else
			m_lock.acquire();
			old = m_ptr;
			m_ptr = 0;
			m_lock.release();
#endif
			if (old) 
				old->Release();
			return old;
			
		}

		Object_Ptr& operator = (OBJECT* ptr)
		{
			if (ptr)
				ptr->AddRef();

#if defined(WIN32)
			OBJECT* old = reinterpret_cast<OBJECT*>(static_cast<LONG_PTR>(::InterlockedExchange(reinterpret_cast<LONG*>(&m_ptr), static_cast<LONG>(reinterpret_cast<LONG_PTR>(ptr)))));
#else
			m_lock.acquire();
			OBJECT* old = m_ptr;
			m_ptr = ptr;
			m_lock.release();
#endif

			if (old)
				old->Release();

			return *this;
		}

		OBJECT** const operator&()
		{
			return &m_ptr;
		}

		OBJECT* operator ->()
		{
			return m_ptr;
		}

		operator OBJECT*()
		{
			return m_ptr;
		}

		template <class NEW>
		OOObject::int32_t QueryInterface(NEW** ppVal)
		{
			return m_ptr->QueryInterface(NEW::IID,reinterpret_cast<OOObject::Object**>(ppVal));
		}
			
	protected:
		OBJECT* m_ptr;

#if !defined(WIN32)
		ACE_Thread_Mutex m_lock;
#endif
	};

	template <class OBJECT>
	class Object_Impl : public OBJECT
	{
	public:
		Object_Impl() : m_refcount(0) 
		{ }

		virtual OOObject::int32_t AddRef()
		{
			++m_refcount;
			return 0;
		}

		virtual OOObject::int32_t Release()
		{
			if (--m_refcount==0)
				delete this;
			
			return 0;
		}

		virtual OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			if (!ppVal)
				return -1;

			if (iid==OOObject::Object::IID || 
				iid==OBJECT::IID)
			{
				++m_refcount;
				*ppVal = this;
				return 0;
			}
	
			return -1;
		}

	protected:
		virtual ~Object_Impl()
		{}

		long RefCount()
		{
			return m_refcount.value();
		}

	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};

	template <class DERIVED>
	class Object_Root
	{
	protected:
		Object_Root() : m_refcount(0)
		{ }

		virtual ~Object_Root()
		{}

		virtual int FinalConstruct()
		{
			return 0;
		}

		OOObject::int32_t Internal_AddRef()
		{
			++m_refcount;
			return 0;
		}

		OOObject::int32_t Internal_Release()
		{
			if (--m_refcount==0)
				delete this;
			
			return 0;
		}

		struct QIEntry
		{
			const OOObject::guid_t* pGuid;
			size_t cast_offset;
		};

		virtual const QIEntry* getQIEntries() const = 0;
		virtual OOObject::Object* GetControllingObject() = 0;
		
		OOObject::int32_t Internal_QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			DERIVED* pT = static_cast<DERIVED*>(this);

			if (!ppVal)
				return -1;

			const QIEntry* g=getQIEntries();
			for (int i=0;g[i].pGuid!=0;++i)
			{
				if (iid == OOObject::Object::IID ||
					*(g[i].pGuid) == iid)
				{
					*ppVal = reinterpret_cast<OOObject::Object*>(reinterpret_cast<size_t>(pT) + g[i].cast_offset);
					(*ppVal)->AddRef();
					return 0;
				}
			}
			
			return -1;
		}
		
	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};

	template <class ROOT>
	class Object : public ROOT
	{
	public:
		static int CreateObject(OOUtil::Object<ROOT>*& pObject)
		{
			ACE_NEW_RETURN(pObject,OOUtil::Object<ROOT>(),-1);

			int res = pObject->FinalConstruct();
			if (res != 0)
			{
				delete pObject;
				pObject = 0;
			}
			return res;
		}

	private:
		Object() : ROOT()
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

	template <class ROOT>
	class SingletonObject : public ROOT
	{
	public:
		SingletonObject() : ROOT()
		{ 
			FinalConstruct();
		}
		
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

	template <class ROOT>
	class AggObject : public Object_Impl<OOObject::Object>
	{
		class ContainedObject : public ROOT
		{
			friend class AggObject<ROOT>;

			ContainedObject(OOObject::Object* pOuter) : 
				m_pOuter(pOuter)
			{ }

			OOObject::Object* m_pOuter;

		protected:
			virtual OOObject::Object* GetControllingObject()
			{
				return m_pOuter;
			}

		// OOObject::Object members
		public:
			OOObject::int32_t AddRef()
			{
				return m_pOuter->AddRef();
			}

			OOObject::int32_t Release()
			{
				return m_pOuter->Release();
			}

			OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal)
			{
				return m_pOuter->QueryInterface(iid,ppVal);
			}
		};
		
		AggObject(OOObject::Object* pOuter) : m_contained(pOuter)
		{ }

		ContainedObject m_contained;

	public:
		static int CreateObject(OOObject::Object* pOuter, AggObject<ROOT>*& pObject)
		{
			ACE_NEW_RETURN(pObject,AggObject<ROOT>(pOuter),-1);

			int res = pObject->m_contained.FinalConstruct();
			if (res != 0)
			{
				delete pObject;
				pObject = 0;
			}
			return res;
		}

		ROOT* Instance()
		{
			return &m_contained;
		}

	// OOObject::Object members
	public:
		OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			if (Object_Impl<OOObject::Object>::QueryInterface(iid,ppVal) == 0)
				return 0;
			
			return m_contained.QueryInterface(iid,ppVal);
		}
	};
	
	// IO helpers
	class InputStream_Ptr : public Object_Ptr<OOObject::InputStream>
	{
		typedef Object_Ptr<OOObject::InputStream> baseClass;

	public:
		InputStream_Ptr() :
			baseClass()
		{ }

		template <class T>
		InputStream_Ptr(T* t) :
			baseClass(t)
		{ }

		template <class T>
		InputStream_Ptr& operator = (T* t)
		{
			baseClass::operator =(t);
			return (*this);
		}

		int read(OOObject::bool_t& in) { return (m_ptr ? m_ptr->ReadBoolean(in) : -1); }
		int read(OOObject::char_t& in) { return (m_ptr ? m_ptr->ReadChar(in) : -1); }
		int read(OOObject::int16_t& in) { return (m_ptr ? m_ptr->ReadShort(in) : -1); }
		int read(OOObject::uint16_t& in) { return (m_ptr ? m_ptr->ReadUShort(in) : -1); }
		int read(OOObject::int32_t& in) { return (m_ptr ? m_ptr->ReadLong(in) : -1); }
		int read(OOObject::uint32_t& in) { return (m_ptr ? m_ptr->ReadULong(in) : -1); }
		int read(OOObject::int64_t& in) { return (m_ptr ? m_ptr->ReadLongLong(in) : -1); }
		int read(OOObject::uint64_t& in) { return (m_ptr ? m_ptr->ReadULongLong(in) : -1); }
		int read(OOObject::real4_t& in) { return (m_ptr ? m_ptr->ReadFloat(in) : -1); }
		int read(OOObject::real8_t& in) { return (m_ptr ? m_ptr->ReadDouble(in) : -1); }
		
		int read(OOObject::guid_t& val) 
		{
			if (m_ptr == 0)
				return -1;

			if (m_ptr->ReadULong(val.Data1) != 0) return -1;
			if (m_ptr->ReadUShort(val.Data2) != 0) return -1;
			if (m_ptr->ReadUShort(val.Data3) != 0) return -1;
			return read_bytes(val.Data4,8);
		}

		// Work around for the fact that on some platforms ACE_CDR::Boolean is typedef'd as unsigned char
		int read(ACE::If_Then_Else<(sizeof(bool)==1),OOObject::byte_t,OOObject::byte_t[2]>::result_type& in)
		{ 
			return read_byte_workaround(in); 
		}

	private:
		int read_byte_workaround(OOObject::byte_t& in)
		{
			if (m_ptr == 0)
				return -1;

			return m_ptr->ReadByte(in);
		}

		int read_byte_workaround(OOObject::byte_t in[2])
		{
			if (m_ptr == 0)
				return -1;

			return read_bytes(in,2);
		}

		int read_bytes(OOObject::byte_t* b, size_t c)
		{
			if (m_ptr == 0)
				return -1;

			for (size_t i=0;i<c;++i)
			{
				if (m_ptr->ReadByte(b[i]) != 0) return -1;
			}
			return 0;
		}
	};

	class OutputStream_Ptr : public Object_Ptr<OOObject::OutputStream>
	{
		typedef Object_Ptr<OOObject::OutputStream> baseClass;

	public:
		OutputStream_Ptr() :
			baseClass()
		{ }

		template <class T>
		OutputStream_Ptr(T* t) :
			baseClass(t)
		{ }

		template <class T>
		OutputStream_Ptr& operator = (T* t)
		{
			baseClass::operator =(t);
			return (*this);
		}

		int write(const OOObject::bool_t& out) { return (m_ptr ? m_ptr->WriteBoolean(out) : -1); }
		int write(const OOObject::char_t& out) { return (m_ptr ? m_ptr->WriteChar(out) : -1); }
		int write(const OOObject::int16_t& out) { return (m_ptr ? m_ptr->WriteShort(out) : -1); }
		int write(const OOObject::uint16_t& out) { return (m_ptr ? m_ptr->WriteUShort(out) : -1); }
		int write(const OOObject::int32_t& out) { return (m_ptr ? m_ptr->WriteLong(out) : -1); }
		int write(const OOObject::uint32_t& out) { return (m_ptr ? m_ptr->WriteULong(out) : -1); }
		int write(const OOObject::int64_t& out) { return (m_ptr ? m_ptr->WriteLongLong(out) : -1); }
		int write(const OOObject::uint64_t& out) { return (m_ptr ? m_ptr->WriteULongLong(out) : -1); }
		int write(const OOObject::real4_t& out) { return (m_ptr ? m_ptr->WriteFloat(out) : -1); }
		int write(const OOObject::real8_t& out) { return (m_ptr ? m_ptr->WriteDouble(out) : -1); }
		
		int write(const OOObject::guid_t& val)
		{
			if (!m_ptr) 
				return -1;

			if (m_ptr->WriteULong(val.Data1) != 0) return -1;
			if (m_ptr->WriteUShort(val.Data2) != 0) return -1;
			if (m_ptr->WriteUShort(val.Data3) != 0) return -1;

			return write_bytes(val.Data4,8);
		}

		// Work around for the fact that on some platforms ACE_CDR::Boolean is typedef'd as unsigned char
		int write(const ACE::If_Then_Else<(sizeof(bool)==1),OOObject::byte_t,OOObject::byte_t[2]>::result_type& out)
		{ 
			return write_byte_workaround(out); 
		}

	private:
		int write_byte_workaround(const OOObject::byte_t& out)
		{
			if (!m_ptr) return -1;
			return m_ptr->WriteByte(out);
		}

		int write_byte_workaround(const OOObject::byte_t out[2])
		{
			return write_bytes(out,2);
		}

		int write_bytes(const OOObject::byte_t* p, size_t c)
		{
			if (!m_ptr) return -1;
			for (size_t i=0;i<c;++i)
			{
				if (m_ptr->WriteByte(p[i]) != 0) return -1;
			}
			return 0;
		}
	};

	class RegistryKey_Ptr : public Object_Ptr<OOObject::RegistryKey>
	{
		typedef Object_Ptr<OOObject::RegistryKey> baseClass;

	public:
		RegistryKey_Ptr() :
			baseClass()
		{ }

		template <class T>
		RegistryKey_Ptr(T* t) :
			baseClass(t)
		{ }

		template <class T>
		RegistryKey_Ptr& operator = (T* t)
		{
			baseClass::operator =(t);
			return (*this);
		}

		int open(const OOObject::char_t* key, OOObject::bool_t create = false)
		{
			RegistryKey_Ptr ptr;
			if (OOObject::OpenRegistryKey(key,create,&ptr) != 0)
				return -1;

			(*this) = ptr;
			return 0;
		}

		int QueryValue(const OOObject::char_t* name, ACE_CString& value)
		{
			if (!m_ptr) return -1;

			size_t size = 128;
			for (;;)
			{
				OOObject::char_t* pszBuf = new OOObject::char_t[128];
				int ret = m_ptr->QueryValue(name,pszBuf,&size);
				if (ret >= 0)
					value.set(pszBuf);
				delete [] pszBuf;
				if (ret == 0)
				{		
					return 0;
				}
				else if (ret != 1)
				{
					return -1;
				}
			}
		}

		int SetValue(const OOObject::char_t* name, int value)
		{
			OOObject::char_t szBuf[24];
			ACE_OS::sprintf(szBuf,"%ld",value);
			return m_ptr->SetValue(name,szBuf);
		}
	};
};

#define BEGIN_INTERFACE_MAP(cls) \
	private: \
	typedef cls RootClass; \
	const QIEntry* getQIEntries() const {static const QIEntry QIEntries[] = { 

#ifdef _MSC_VER
#define CLASS_CAST_OFFSET 0x1000
#else
#error You need to define CLASS_CAST_OFFSET, try 0x1000
#endif

#define INTERFACE_ENTRY(object) \
	{ &object::IID, reinterpret_cast<size_t>(static_cast<object*>(reinterpret_cast<RootClass*>(CLASS_CAST_OFFSET))) - CLASS_CAST_OFFSET}, 

#define END_INTERFACE_MAP() \
	{ 0 } }; return QIEntries; } \
	protected: \
	virtual OOObject::Object* GetControllingObject() { return reinterpret_cast<OOObject::Object*>(reinterpret_cast<size_t>(this) + getQIEntries()->cast_offset); }
	

#endif // OOCORE_UTIL_H_INCLUDED_

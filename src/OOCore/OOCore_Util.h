#ifndef OOCORE_OOCORE_UTIL_H_INCLUDED_
#define OOCORE_OOCORE_UTIL_H_INCLUDED_

#include <ace/Atomic_Op.h>

#include "./OOCore.h"

namespace OOCore
{
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
			
	private:
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

		OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal)
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

	template <class thisClass>
	class Object_Root
	{
	public:
		Object_Root() : m_refcount(0) 
		{ }

	protected:
		virtual ~Object_Root()
		{}

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

		virtual const QIEntry* getQIEntries() = 0;

		OOObject::int32_t Internal_QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			if (!ppVal)
				return -1;

			const QIEntry* g=getQIEntries();
			for (int i=0;g[i].pGuid!=0;++i)
			{
				if (iid == OOObject::Object::IID ||
					*(g[i].pGuid) == iid)
				{
					Internal_AddRef();
					*ppVal = reinterpret_cast<OOObject::Object*>(reinterpret_cast<size_t>(static_cast<thisClass*>(this)) + g[i].cast_offset);
					return 0;
				}
			}
			
			return -1;
		}
		
	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};
};

#define BEGIN_INTERFACE_MAP(cls) \
	private: \
	typedef cls RootClass; \
	public: \
	OOObject::int32_t AddRef() {return Internal_AddRef();} \
	OOObject::int32_t Release() {return Internal_Release();} \
	OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal) {return Internal_QueryInterface(iid,ppVal);} \
	private: \
	const QIEntry* getQIEntries() {static const QIEntry QIEntries[] = { 

#ifdef _MSC_VER
#define CLASS_CAST_OFFSET 0x1000
#else
#error You need to define CLASS_CAST_OFFSET, try 0x1000
#endif

#define INTERFACE_ENTRY(object) \
	{ &object::IID, reinterpret_cast<size_t>(static_cast<object*>(reinterpret_cast<RootClass*>(CLASS_CAST_OFFSET))) - CLASS_CAST_OFFSET}, 

#define END_INTERFACE_MAP() \
	{ 0 } }; return QIEntries; }

#endif // OOCORE_OOCORE_UTIL_H_INCLUDED_

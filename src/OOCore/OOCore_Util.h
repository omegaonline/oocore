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

		Object_Ptr& operator = (OBJECT* ptr)
		{
			if (ptr)
				ptr->AddRef();

#ifdef ACE_WIN32
			OBJECT* old = reinterpret_cast<OBJECT*>(static_cast<LONG_PTR>(::InterlockedExchange(reinterpret_cast<LONG*>(&m_ptr), static_cast<LONG>(reinterpret_cast<LONG_PTR>(ptr)))));
#else
			static ACE_Thread_Mutex m_lock;
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
			
	private:
		OBJECT* m_ptr;
	};

	template <class OBJECT>
	class Object_Impl : public OBJECT
	{
	public:
		Object_Impl() : m_refcount(0) 
		{ }

		OOObject::int32_t AddRef()
		{
			++m_refcount;
			return 0;
		}

		OOObject::int32_t Release()
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

	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};

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

		OOObject::int32_t Internal_QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal)
		{
			if (!ppVal)
				return -1;

			for (const OOObject::guid_t* g=getQIEntries();g!=0;++g)
			{
				if (*g == iid)
				{
					Internal_AddRef();
					*ppVal = reinterpret_cast<OOObject::Object*>(this);
					return 0;
				}
			}
			
			return -1;
		}

		virtual const OOObject::guid_t* getQIEntries() = 0;
		
	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};
};

#define BEGIN_INTERFACE_MAP(cls) \
	public: \
	OOObject::int32_t AddRef() {return Internal_AddRef();} \
	OOObject::int32_t Release() {return Internal_Release();} \
	OOObject::int32_t QueryInterface(const OOObject::guid_t& iid, OOObject::Object** ppVal) {return Internal_QueryInterface(iid,ppVal);} \
	private: \
	const OOObject::guid_t* getQIEntries() {static const OOObject::guid_t* QIEntries[] = { &OOObject::Object::IID,

#define INTERFACE_ENTRY(object) \
	&object::IID,

#define END_INTERFACE_MAP() \
	0 }; return QIEntries[0]; }

#endif // OOCORE_OOCORE_UTIL_H_INCLUDED_

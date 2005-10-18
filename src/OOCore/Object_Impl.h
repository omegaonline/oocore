//////////////////////////////////////////////////////
//
// This header file is for internal use only
//
// #include "Object.h" instead
//
//////////////////////////////////////////////////////

#pragma once

#include <ace/Atomic_Op.h>
#include <ace/Active_Map_Manager.h>

#include "./Guid.h"
#include "./Object_Types.h"

#define DECLARE_IID(export)			static const export OOObj::GUID IID;
#define DEFINE_IID(type,val)		const OOObj::GUID type::IID(#val);

namespace OOObj
{
	class Object
	{
	public:
		virtual int AddRef() = 0;
		virtual int Release() = 0;
		virtual int QueryInterface(const OOObj::GUID& iid, Object** ppVal) = 0;

		DECLARE_IID(OOCore_Export);
	};
	
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
			if (m_ptr)
				m_ptr->Release();

			m_ptr = rhs.m_ptr;
			
			if (m_ptr)
				m_ptr->AddRef();

			return *this;
		}

		Object_Ptr& operator = (OBJECT* ptr)
		{
			if (m_ptr)
				m_ptr->Release();

			m_ptr = ptr;
			
			if (m_ptr)
				m_ptr->AddRef();

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

		int AddRef()
		{
			++m_refcount;
			return 0;
		}

		int Release()
		{
			if (--m_refcount==0)
				delete this;
			
			return 0;
		}

	protected:
		virtual ~Object_Impl()
		{}

	private:
		ACE_Atomic_Op<ACE_Thread_Mutex,long> m_refcount;
	};
};
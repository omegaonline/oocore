///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "TLSSingleton.h"
#include "Destructor.h"

#include <map>

namespace 
{
	class TLSMap
	{
	public:
		static TLSMap* instance(bool create = true);

		static void destroy(void* pThis);
		
		struct tls_val
		{
			void* val;
			void (*destructor)(void*);
		};
		std::map<const void*,tls_val> m_mapVals;

	private:
		TLSMap() {}
		TLSMap(const TLSMap&) {}
		TLSMap& operator = (const TLSMap&) { return *this; }
		~TLSMap() {}
	};

	struct TLSGlobal
	{
		TLSGlobal();
		~TLSGlobal();

#if defined(_WIN32)
		DWORD m_key;
#else
#error Fix me!
#endif
	};	
}

void OOBase::TLS::Add(const void* key, void (*destructor)(void*))
{
	TLSMap* inst = TLSMap::instance();

	try
	{
		std::map<const void*,TLSMap::tls_val>::iterator i=inst->m_mapVals.find(key);
		if (i != inst->m_mapVals.end())
		{
			i->second.val = 0;
			i->second.destructor = destructor;
		}
		else
		{
			TLSMap::tls_val v;
			v.val = 0;
			v.destructor = destructor;

			inst->m_mapVals.insert(std::map<const void*,TLSMap::tls_val>::value_type(key,v));
		}
	}
	catch(std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
	}
}

bool OOBase::TLS::Get(const void* key, void** val)
{
	TLSMap* inst = TLSMap::instance();

	try
	{
		std::map<const void*,TLSMap::tls_val>::iterator i=inst->m_mapVals.find(key);
		if (i == inst->m_mapVals.end())
			return false;
	
		*val = i->second.val;
	}
	catch(std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
	}

	return true;
}

void OOBase::TLS::Set(const void* key, void* val)
{
	TLSMap* inst = TLSMap::instance();

	try
	{
		std::map<const void*,TLSMap::tls_val>::iterator i=inst->m_mapVals.find(key);
		if (i != inst->m_mapVals.end())
			i->second.val = val;
		else
		{
			TLSMap::tls_val v;
			v.val = val;
			v.destructor = 0;

			inst->m_mapVals.insert(std::map<const void*,TLSMap::tls_val>::value_type(key,v));
		}
	}
	catch(std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
	}
}

void OOBase::TLS::Remove(const void* key)
{
	try
	{
		TLSMap::instance()->m_mapVals.erase(key);
	}
	catch(std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
	}
}

void OOBase::TLS::ThreadExit()
{
	TLSMap* inst = TLSMap::instance(false);
	if (inst)
	{
		OOBase::Destructor::remove_destructor(TLSMap::destroy,inst);
		TLSMap::destroy(inst);
	}
}

namespace 
{
	static TLSGlobal s_tls_cons;
}

#if defined(_WIN32)
TLSGlobal::TLSGlobal() :
	m_key(TLS_OUT_OF_INDEXES)
{
	m_key = TlsAlloc();
	if (m_key == TLS_OUT_OF_INDEXES)
		OOBase_CallCriticalFailure(GetLastError());
	
	if (!TlsSetValue(m_key,0))
		OOBase_CallCriticalFailure(GetLastError());
}

TLSGlobal::~TLSGlobal()
{
	if (m_key != TLS_OUT_OF_INDEXES)
		TlsFree(m_key);
}

#else
#error Fix TLS alloc stuff
#endif

TLSMap* TLSMap::instance(bool create)
{
#if defined(_WIN32)
	TLSMap* inst = static_cast<TLSMap*>(TlsGetValue(s_tls_cons.m_key));
	if (inst == 0 && create)
	{
		OOBASE_NEW(inst,TLSMap());
		if (!inst)
			OOBase_OutOfMemory();

		OOBase::Destructor::add_destructor(destroy,inst);

		if (!TlsSetValue(s_tls_cons.m_key,inst))
			OOBase_CallCriticalFailure(GetLastError());
	}
	return inst;
#else
#error Fix TLS alloc stuff
#endif
}

void TLSMap::destroy(void* pThis)
{
	TLSMap* inst = static_cast<TLSMap*>(pThis);
	try
	{
		for (std::map<const void*,tls_val>::iterator i=inst->m_mapVals.begin();i!=inst->m_mapVals.end();++i)
		{
			if (i->second.destructor)
				(*(i->second.destructor))(i->second.val);
		}
		inst->m_mapVals.clear();
	}
	catch(std::exception& e)
	{
		OOBase_CallCriticalFailure(e.what());
	}

	delete inst;
}

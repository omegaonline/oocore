///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOSvrLite, the Omega Online Standalone plugin.
//
// OOSvrLite is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOSvrLite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOSvrLite.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_Lite.h"
#include "IPSLite.h"

using namespace Omega;
using namespace OTL;

// Our library map
BEGIN_LIBRARY_OBJECT_MAP()
END_LIBRARY_OBJECT_MAP_NO_ENTRYPOINT()

#if defined(_WIN32)

extern "C" BOOL WINAPI DllMain(HANDLE /*instance*/, DWORD /*reason*/, LPVOID /*lpreserved*/)
{
	return TRUE;
}

#endif

void InterProcessService::Load(const string_t& str)
{
	// Split out individual args
	for (size_t start = 0;;)
	{
		// Skip leading whitespace
		while (start < str.Length() && (str[start] == L'\t' || str[start] == L' '))
			++start;

		if (start == str.Length())
			return;

		// Find the next linefeed
		size_t end = str.Find(L',',start);

		// Trim trailing whitespace
		size_t valend = (end == string_t::npos ? str.Length() : end);
		while (valend > start && (str[valend-1] == L'\t' || str[valend-1] == L' '))
			--valend;

		if (valend > start)
		{
			string_t strKey, strValue;

			// Split on first =
			size_t eq = str.Find(L'=',start);
			if (eq != string_t::npos)
			{
				// Trim trailing whitespace before =
				size_t keyend = eq;
				while (keyend > start && (str[keyend-1] == L'\t' || str[keyend-1] == L' '))
					--keyend;

				if (keyend > start)
				{
					strKey = str.Mid(start,keyend-start);

					// Skip leading whitespace after =
					size_t valpos = eq+1;
					while (valpos < valend && (str[valpos] == L'\t' || str[valpos] == L' '))
						++valpos;

					if (valpos < valend)
						strValue = str.Mid(valpos,valend-valpos);
				}
			}
			else
			{
				strKey = str.Mid(start,valend-start);
				strValue = L"true";
			}

			if (!strKey.IsEmpty())
			{
				string_t* pv = m_args.find(strKey);
				if (pv)
					*pv = strValue;
				else
				{
					int err = m_args.insert(strKey,strValue);
					if (err != 0)
						OMEGA_THROW(err);
				}
			}
		}

		if (end == string_t::npos)
			return;

		start = end + 1;
	}
}

string_t InterProcessService::GetArg(const string_t& arg)
{
	string_t ret;
	m_args.find(arg,ret);
	return ret;
}

Activation::IRunningObjectTable* InterProcessService::GetRunningObjectTable()
{
	return NULL;
}

void InterProcessService::LaunchObjectApp(const guid_t& oid, const guid_t&, Activation::Flags_t, uint32_t envc, const byte_t* envp, IObject*& pObject)
{
	pObject = NULL;
	throw Activation::IOidNotFoundException::Create(oid);
}

bool_t InterProcessService::HandleRequest(uint32_t timeout)
{
	return Omega::HandleRequest(timeout);
}

Remoting::IChannel* InterProcessService::OpenRemoteChannel(const string_t&)
{
	throw Remoting::IChannelClosedException::Create();
}

Remoting::IChannelSink* InterProcessService::OpenServerSink(const guid_t&, Remoting::IChannelSink*)
{
	throw Remoting::IChannelClosedException::Create();
}

OMEGA_DEFINE_EXPORTED_FUNCTION(OOCore::IInterProcessService*,OOSvrLite_GetIPS,1,((in),const string_t&,args))
{
	ObjectPtr<SingletonObjectImpl<InterProcessService> > ptrIPS = SingletonObjectImpl<InterProcessService>::CreateInstance();
	ptrIPS->Load(args);
	return ptrIPS.AddRef();
}

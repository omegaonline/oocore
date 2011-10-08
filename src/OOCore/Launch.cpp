///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
//
// This file is part of OOCore, the Omega Online Core library.
//
// OOCore is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOCore is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOCore.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOCore_precomp.h"

#include "UserSession.h"
#include "Activation.h"

using namespace Omega;
using namespace OTL;

#if !defined(ECONNREFUSED) && defined(_WIN32)
#define ECONNREFUSED WSAECONNREFUSED
#endif

namespace
{
	void parse_args(const string_t& str, OOBase::Table<string_t,string_t>& args)
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
					int err = args.replace(strKey,strValue);
					if (err != 0)
						OMEGA_THROW(err);
				}
			}

			if (end == string_t::npos)
				return;

			start = end + 1;
		}
	}
	
	void discover_server_port(bool& bStandalone, OOBase::LocalString& strPipe)
	{
		int err = strPipe.getenv("OMEGA_SESSION_ADDRESS");
		if (err != 0)
			OMEGA_THROW(err);
			
		if (!strPipe.empty())
			return;
			
 	#if defined(_WIN32)
 		const char* name = "OmegaOnline";
	#else
		const char* name = "/tmp/omegaonline";
	#endif

		OOBase::SmartPtr<OOBase::Socket> root_socket = OOBase::Socket::connect_local(name,err);
		if (!root_socket)
		{
			if (err == ENOENT && bStandalone)
 				return;
 
			ObjectPtr<IException> ptrE = ISystemException::Create(err);
			throw IInternalException::Create("Failed to connect to network daemon","Omega::Initialize",size_t(-1),NULL,ptrE);
		}

		bStandalone = false;

		// Send version information
		OOBase::CDRStream stream;

		uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;
		if (!stream.write(version))
			OMEGA_THROW(stream.last_error());

		err = root_socket->send(stream.buffer());
		if (err)
			OMEGA_THROW(err);

		// We know a CDRStream writes strings as a 4 byte length followed by the character data
		stream.reset();
		err = root_socket->recv(stream.buffer(),sizeof(uint32_t));
		if (err != 0)
			OMEGA_THROW(err);

		uint32_t len = 0;
		if (!stream.read(len))
			OMEGA_THROW(stream.last_error());

		err = root_socket->recv(stream.buffer(),len);
		if (err != 0)
			OMEGA_THROW(err);

		// Now reset rd_ptr and read the string
		stream.buffer()->mark_rd_ptr(0);

		if (!stream.read(strPipe))
			OMEGA_THROW(stream.last_error());
	}
}

void OOCore::UserSession::start(const string_t& strArgs)
{
	OOBase::Table<string_t,string_t> args;
	parse_args(strArgs,args);

	bool bStandalone = false;
	bool bStandaloneAlways = false;
	size_t i = args.find(L"standalone");
	if (i != args.npos)
	{
		if (*args.at(i) == L"true")
			bStandalone = true;
		else if (*args.at(i) == L"always")
		{
			bStandalone = true;
			bStandaloneAlways = true;
		}
	}

	OOBase::LocalString strPipe;
	if (!bStandaloneAlways)
		discover_server_port(bStandalone,strPipe);

	int err = 0;
	if (!bStandalone)
	{
		// Connect up to the user process...
		OOBase::timeval_t wait(15);
		OOBase::Countdown countdown(&wait);
		do
		{
			m_stream = OOBase::Socket::connect_local(strPipe.c_str(),err,&wait);
			if (!err || (err != ENOENT && err != ECONNREFUSED))
				break;

			// We ignore the error, and try again until we timeout
			countdown.update();
		}
		while (wait != OOBase::timeval_t::Zero);

		if (err)
			OMEGA_THROW(err);

		// Send version information
		uint32_t version = (OOCORE_MAJOR_VERSION << 24) | (OOCORE_MINOR_VERSION << 16) | OOCORE_PATCH_VERSION;
		if ((err = m_stream->send(version)) != 0)
			OMEGA_THROW(err);

		// Read our channel id
		if ((err = m_stream->recv(m_channel_id)) != 0)
			OMEGA_THROW(err);

		// Spawn off the io worker thread
		m_worker_thread.run(io_worker_fn,this);
	}

	// Create the zero compartment
	OOBase::SmartPtr<Compartment> ptrZeroCompt = new (OOCore::throwing) Compartment(this);
	
	{
		OOBase::Guard<OOBase::RWMutex> guard(m_lock);

		if ((err = m_mapCompartments.replace(0,ptrZeroCompt)) != 0)
			OMEGA_THROW(err);

		ptrZeroCompt->set_id(0);
	}

	// Remove standalone support eventually...
	void* ISSUE_4;

	ObjectPtr<IInterProcessService> ptrIPS;
	if (!bStandalone)
	{
		// Create a new object manager for the user channel on the zero compartment
		ObjectPtr<Remoting::IObjectManager> ptrOM = ptrZeroCompt->get_channel_om(m_channel_id & 0xFF000000);

		// Create a proxy to the server interface
		IObject* pIPS = NULL;
		ptrOM->GetRemoteInstance(OID_InterProcessService,Activation::Library | Activation::DontLaunch,OMEGA_GUIDOF(IInterProcessService),pIPS);
		ptrIPS = static_cast<IInterProcessService*>(pIPS);
	}
	else
	{
		// Load up OOSvrLite and get the IPS from there...
		int err = m_lite_dll.load("oosvrlite");
		if (err != 0)
			OMEGA_THROW(err);

		typedef const System::Internal::SafeShim* (OMEGA_CALL *pfnOOSvrLite_GetIPS_Safe)(System::Internal::marshal_info<IInterProcessService*&>::safe_type::type OOSvrLite_GetIPS_RetVal, System::Internal::marshal_info<const string_t&>::safe_type::type args);

		pfnOOSvrLite_GetIPS_Safe pfn = (pfnOOSvrLite_GetIPS_Safe)(m_lite_dll.symbol("OOSvrLite_GetIPS_Safe"));
		if (!pfn)
			OMEGA_THROW("Corrupt OOSvrLite");

		const System::Internal::SafeShim* pSE = (*pfn)(System::Internal::marshal_info<IInterProcessService*&>::safe_type::coerce(ptrIPS),System::Internal::marshal_info<const string_t&>::safe_type::coerce(strArgs));
		if (pSE)
			System::Internal::throw_correct_exception(pSE);
	}

	// Register locally...
	m_nIPSCookie = OOCore_RegisterIPS(ptrIPS);
	
	// Now set our pipe name as an env var
	if (!strPipe.empty())
	{
#if defined(_WIN32)
		SetEnvironmentVariableA("OMEGA_SESSION_ADDRESS",strPipe.c_str());
#else
		setenv("OMEGA_SESSION_ADDRESS",strPipe.c_str(),1);
#endif
	}
}
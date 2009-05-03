///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
//
// This file is part of OOServer, the Omega Online Server application.
//
// OOServer is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOServer is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with OOServer.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#include "OOServer_User.h"
#include "InterProcessService.h"
#include "UserManager.h"

using namespace Omega;
using namespace OTL;

void User::InterProcessService::Init(OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMSB, OTL::ObjectPtr<Omega::Remoting::IObjectManager> ptrOMUser, Manager* pManager)
{
	m_pManager = pManager;

	// Create a proxy to the server interface
	if (ptrOMSB)
	{
		IObject* pIPS = 0;
		ptrOMSB->GetRemoteInstance(System::OID_InterProcessService.ToString(),Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(System::IInterProcessService),pIPS);
		m_ptrSBIPS.Attach(static_cast<System::IInterProcessService*>(pIPS));
	}

	if (ptrOMUser)
	{
		// Create a proxy to the server interface
		IObject* pIPS = 0;
		ptrOMUser->GetRemoteInstance(System::OID_InterProcessService.ToString(),Activation::InProcess | Activation::DontLaunch,OMEGA_GUIDOF(System::IInterProcessService),pIPS);
		ObjectPtr<System::IInterProcessService> ptrIPS;
		ptrIPS.Attach(static_cast<System::IInterProcessService*>(pIPS));

		// Get the running object table
		m_ptrReg.Attach(ptrIPS->GetRegistry());
	}
	else
	{
		// Create a local registry impl
		ObjectPtr<ObjectImpl<Registry::Key> > ptrKey = ObjectImpl<User::Registry::Key>::CreateInstancePtr();
		ptrKey->Init(m_pManager,L"",0);

		m_ptrReg = static_cast<Omega::Registry::IKey*>(ptrKey);
	}

	// Create the ROT
	m_ptrROT = ObjectImpl<User::RunningObjectTable>::CreateInstancePtr();
	try
	{
		m_ptrROT->Init(ptrOMSB);
	}
	catch (...)
	{
		m_ptrROT.Release();
		throw;
	}
}

Registry::IKey* User::InterProcessService::GetRegistry()
{
	return m_ptrReg.AddRef();
}

Activation::IRunningObjectTable* User::InterProcessService::GetRunningObjectTable()
{
	return m_ptrROT.AddRef();
}

void User::InterProcessService::GetObject(const string_t& strProcess, bool_t bPublic, const guid_t& oid, const guid_t& iid, IObject*& pObject)
{
	if (bPublic && m_ptrSBIPS)
		return m_ptrSBIPS->GetObject(strProcess,false,oid,iid,pObject);

	// The timeout needs to be related to the request timeout...
	OOBase::timeval_t wait(15);
	ObjectPtr<Remoting::ICallContext> ptrCC;
	ptrCC.Attach(Remoting::GetCallContext());
	if (ptrCC)
	{
		uint32_t msecs = ptrCC->Timeout();
		if (msecs != (uint32_t)-1)
			wait = OOBase::timeval_t(msecs / 1000,(msecs % 1000) * 1000);
	}

	OOBase::SmartPtr<User::Process> ptrProcess;

	bool bStarted = false;
	do
	{
		OOBase::Guard<OOBase::Mutex> guard(m_lock);

		std::map<string_t,OOBase::SmartPtr<User::Process> >::iterator i = m_mapInProgress.find(strProcess);
		if (i != m_mapInProgress.end())
		{
			ptrProcess = i->second;

			if (!ptrProcess->running())
			{
				m_mapInProgress.erase(strProcess);
				ptrProcess = 0;
			}			
		}

		if (!ptrProcess)
		{
			// Create a new process
			ptrProcess = User::Process::exec(strProcess.c_str());
						
			m_mapInProgress.insert(std::map<string_t,OOBase::SmartPtr<User::Process> >::value_type(strProcess,ptrProcess));
			bStarted = true;
		}

		guard.release();

		// Wait for the process to start and register its parts...
		OOBase::Countdown timeout(&wait);
		while (wait != OOBase::timeval_t::zero)
		{
			ObjectPtr<IObject> ptrObject;
			ptrObject.Attach(m_ptrROT->GetObject(oid));
			if (ptrObject)
			{
				pObject = ptrObject->QueryInterface(iid);
				if (!pObject)
					throw INoInterfaceException::Create(iid);
				return;
			}

			// Check the process is still alive
			OOBase::timeval_t short_wait(0,100000);
			int ec = 0;
			if (ptrProcess->wait_for_exit(&short_wait,&ec))
				break;

			// Update our countdown
			timeout.update();
		}

		if (wait == OOBase::timeval_t::zero)
			throw ITimeoutException::Create();

		// Remove from the map
		guard.acquire();
		m_mapInProgress.erase(strProcess);
		guard.release();

	} while (!bStarted);

	OMEGA_THROW(ENOENT);
}

IO::IStream* User::InterProcessService::OpenStream(const string_t& strEndpoint, IO::IAsyncStreamNotify* pNotify)
{
	// First try to determine the protocol...
	size_t pos = strEndpoint.Find(L':');
	if (pos == string_t::npos)
		OMEGA_THROW(L"No protocol specified!");

	// Look up handler in registry
	string_t strProtocol = strEndpoint.Left(pos).ToLower();

	string_t strHandler;
	ObjectPtr<Omega::Registry::IKey> ptrKey(L"\\Local User");
	if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
	{
		ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
		if (ptrKey->IsValue(L"Handler"))
			strHandler = ptrKey->GetStringValue(L"Handler");
	}

	if (strHandler.IsEmpty())
	{
		ptrKey = ObjectPtr<Omega::Registry::IKey>(L"\\");
		if (ptrKey->IsSubKey(L"Networking\\Protocols\\" + strProtocol))
		{
			ptrKey = ptrKey.OpenSubKey(L"Networking\\Protocols\\" + strProtocol);
			if (ptrKey->IsValue(L"Handler"))
				strHandler = ptrKey->GetStringValue(L"Handler");
		}
	}

	guid_t oid = guid_t::Null();
	if (!strHandler.IsEmpty())
	{
		oid = guid_t::FromString(strHandler);
		if (oid == guid_t::Null())
			oid = Activation::NameToOid(strHandler);
	}

	if (oid == guid_t::Null())
	{
		// We have some built-ins
/*		if (strProtocol == L"tcp")
			oid = OID_TcpProtocolHandler;
		else if (strProtocol == L"http" || strProtocol == L"https")
			oid = OID_HttpProtocolHandler;
		else*/
			OMEGA_THROW(L"No handler for protocol " + strProtocol);
	}

	// Create the handler...
	ObjectPtr<Net::IProtocolHandler> ptrHandler(oid);

	// Open the stream...
	return ptrHandler->OpenStream(strEndpoint,pNotify);
}

bool_t User::InterProcessService::HandleRequest(uint32_t timeout)
{
	OOBase::timeval_t wait(timeout/1000,(timeout % 1000) * 1000);
	
	int ret = m_pManager->pump_requests((timeout ? &wait : 0),true);
	if (ret == -1)
		OMEGA_THROW(L"Request processing failed");
	else
		return (ret == 0 ? false : true);
}

Remoting::IChannel* User::InterProcessService::OpenRemoteChannel(const string_t& strEndpoint)
{
	return Manager::open_remote_channel(strEndpoint);
}

Remoting::IChannelSink* User::InterProcessService::OpenServerSink(const guid_t& message_oid, Remoting::IChannelSink* pSink)
{
	return Manager::open_server_sink(message_oid,pSink);
}

OMEGA_DEFINE_OID(System,OID_InterProcessService,"{7E9E22E8-C0B0-43f9-9575-BFB1665CAE4A}");

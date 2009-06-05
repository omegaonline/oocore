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

#ifndef OOSERVER_IPSLITE_H_INCLUDED_
#define OOSERVER_IPSLITE_H_INCLUDED_

class InterProcessService :
	public OTL::ObjectBase,
	public Omega::System::IInterProcessService,
	public Omega::System::IProxy
{
public:
	void Init() {}

	BEGIN_INTERFACE_MAP(InterProcessService)
		INTERFACE_ENTRY(Omega::System::IInterProcessService)
		INTERFACE_ENTRY(Omega::System::IProxy)
	END_INTERFACE_MAP()

// System::IProxy members
public:
	void WriteKey(Omega::Remoting::IMessage*)
	{
		OMEGA_THROW(L"Invalid operation");
	}

	void UnpackKey(Omega::Remoting::IMessage*)
	{
		OMEGA_THROW(L"Invalid operation");
	}

	Omega::System::IMarshaller* GetMarshaller()
	{
		OMEGA_THROW(L"Invalid operation");
	}

	Omega::bool_t IsAlive()
	{
		return true;
	}

// System::IInterProcessService members
public:
	Omega::Registry::IKey* GetRegistry();
	Omega::Activation::IRunningObjectTable* GetRunningObjectTable();
	void LaunchObjectApp(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject);
	Omega::bool_t HandleRequest(Omega::uint32_t timeout);
	Omega::Remoting::IChannel* OpenRemoteChannel(const Omega::string_t& strEndpoint);
	Omega::Remoting::IChannelSink* OpenServerSink(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink);
};

#endif // OOSERVER_IPSLITE_H_INCLUDED_

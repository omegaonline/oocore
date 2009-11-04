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
	public OOCore::IInterProcessService
{
public:
	void Init() {}

	BEGIN_INTERFACE_MAP(InterProcessService)
		INTERFACE_ENTRY(OOCore::IInterProcessService)
	END_INTERFACE_MAP()

// OOCore::IInterProcessService members
public:
	Omega::bool_t IsStandalone() { return true; }
	Omega::Registry::IKey* GetRegistry();
	Omega::Activation::IRunningObjectTable* GetRunningObjectTable();
	void LaunchObjectApp(const Omega::guid_t& oid, const Omega::guid_t& iid, Omega::IObject*& pObject);
	Omega::bool_t HandleRequest(Omega::uint32_t timeout);
	Omega::Remoting::IChannel* OpenRemoteChannel(const Omega::string_t& strEndpoint);
	Omega::Remoting::IChannelSink* OpenServerSink(const Omega::guid_t& message_oid, Omega::Remoting::IChannelSink* pSink);
};

#endif // OOSERVER_IPSLITE_H_INCLUDED_

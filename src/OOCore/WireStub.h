///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2007 Rick Taylor
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

#ifndef OOCORE_WIRESTUB_H_INCLUDED_
#define OOCORE_WIRESTUB_H_INCLUDED_

namespace OOCore
{
	class StdObjectManager;

	class Stub : public Omega::System::MetaInfo::IStubController_Safe
	{
	public:
		Stub(Omega::System::MetaInfo::IObject_Safe* pObjS, Omega::uint32_t stub_id, StdObjectManager* pManager);
		virtual ~Stub();

		Omega::System::MetaInfo::IException_Safe* MarshalInterface(Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t& iid);
		Omega::System::MetaInfo::IException_Safe* ReleaseMarshalData(Omega::System::MetaInfo::IMessage_Safe* pMessage, const Omega::guid_t&);
		Omega::System::MetaInfo::IStub_Safe* LookupStub(Omega::Remoting::IMessage* pMessage);

	// IObject_Safe methods
	public:
		void OMEGA_CALL AddRef_Safe()
		{
			++m_refcount;
		}

		void OMEGA_CALL Release_Safe()
		{
			if (--m_refcount==0)
				delete this;
		}

		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL QueryInterface_Safe(const Omega::guid_t* piid, Omega::System::MetaInfo::IObject_Safe** ppS)
		{
			*ppS = 0;
			if (*piid == OMEGA_GUIDOF(Omega::IObject) ||
				*piid == OMEGA_GUIDOF(Omega::System::IStubController))
			{
				*ppS = this;
				(*ppS)->AddRef_Safe();
			}
			return 0;
		}

		// These can/will not be called
		void OMEGA_CALL Pin() {}
		void OMEGA_CALL Unpin() {}

	// IStubController members
	public:
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL RemoteRelease_Safe(Omega::uint32_t release_count);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL SupportsInterface_Safe(Omega::bool_t* pbSupports, const Omega::guid_t* piid);
		Omega::System::MetaInfo::IException_Safe* OMEGA_CALL MarshalStub_Safe(Omega::System::MetaInfo::IMessage_Safe* pParamsIn, Omega::System::MetaInfo::IMessage_Safe* pParamsOut);

	private:
		Stub(const Stub&) : Omega::System::MetaInfo::IStubController_Safe() {}
		Stub& operator = (const Stub&) { return *this; }

		OOBase::AtomicInt<size_t>              m_refcount;
		OOBase::AtomicInt<Omega::uint32_t>     m_marshal_count;
		OOBase::RWMutex                        m_lock;
		Omega::uint32_t                        m_stub_id;
		Omega::System::MetaInfo::IObject_Safe* m_pObjS;
		StdObjectManager*                      m_pManager;

		std::map<const Omega::guid_t,Omega::System::MetaInfo::IStub_Safe*> m_iid_map;

		Omega::System::MetaInfo::IStub_Safe* FindStub(const Omega::guid_t& iid);
	};
}

#endif // OOCORE_WIRESTUB_H_INCLUDED_

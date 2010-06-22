///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2008 Rick Taylor
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

#ifndef OOCORE_ACTIVATION_H_INCLUDED_
#define OOCORE_ACTIVATION_H_INCLUDED_

namespace OOCore
{
	// {60B09DE7-609E-4b82-BA35-270A9544BE29}
	extern "C" const Omega::guid_t OID_ServiceManager;

	Omega::IObject* GetInstance(const Omega::any_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid);

	// The instance wide ServiceManager instance
	class ServiceManager : 
		public OTL::ObjectBase,
		public Omega::Activation::IRunningObjectTable
	{
	protected:
		ServiceManager();
		virtual ~ServiceManager();

		void InitOnce() {}

		BEGIN_INTERFACE_MAP(ServiceManager)
			INTERFACE_ENTRY(Omega::Activation::IRunningObjectTable)
		END_INTERFACE_MAP()

	private:
		ServiceManager(const ServiceManager&);
		ServiceManager& operator = (const ServiceManager&);

		OOBase::RWMutex m_lock;
		Omega::uint32_t m_nNextCookie;

		struct Info
		{
			Omega::string_t                    m_oid;
			OTL::ObjectPtr<Omega::IObject>     m_ptrObject;
			Omega::Activation::RegisterFlags_t m_flags;
			Omega::uint32_t                    m_rot_cookie;
		};
		std::map<Omega::uint32_t,Info>                                          m_mapServicesByCookie;
		std::multimap<Omega::string_t,std::map<Omega::uint32_t,Info>::iterator> m_mapServicesByOid;

	// IRunningObjectTable members
	public:
		Omega::uint32_t RegisterObject(const Omega::any_t& oid, Omega::IObject* pObject, Omega::Activation::RegisterFlags_t flags);
		void GetObject(const Omega::any_t& oid, Omega::Activation::RegisterFlags_t flags, const Omega::guid_t& iid, Omega::IObject*& pObject);
		void RevokeObject(Omega::uint32_t cookie);
	};
}

#endif // OOCORE_ACTIVATION_H_INCLUDED_

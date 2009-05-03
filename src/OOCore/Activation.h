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
	// The instance wide ServiceManager instance
	class ServiceManager
	{
	public:
		Omega::uint32_t RegisterObject(const Omega::guid_t& oid, Omega::IObject* pObject, Omega::Activation::Flags_t flags, Omega::Activation::RegisterFlags_t reg_flags);
		Omega::IObject* GetObject(const Omega::guid_t& oid, Omega::Activation::Flags_t flags, const Omega::guid_t& iid);
		void RevokeObject(Omega::uint32_t cookie);

	private:
		friend class OOBase::SingletonNoDestroy<ServiceManager>;
		
		ServiceManager();
		ServiceManager(const ServiceManager&) {}
		ServiceManager& operator = (const ServiceManager&) { return *this; }

		OOBase::RWMutex m_lock;
		Omega::uint32_t m_nNextCookie;

		struct Info
		{
			Omega::guid_t                      m_oid;
			OTL::ObjectPtr<Omega::IObject>     m_ptrObject;
			Omega::Activation::Flags_t         m_flags;
			Omega::Activation::RegisterFlags_t m_reg_flags;
			Omega::uint32_t                    m_rot_cookie;
		};
		std::map<Omega::uint32_t,Info>                                        m_mapServicesByCookie;
		std::multimap<Omega::guid_t,std::map<Omega::uint32_t,Info>::iterator> m_mapServicesByOid;
	};
	typedef OOBase::SingletonNoDestroy<ServiceManager> SERVICE_MANAGER;
}

#endif // OOCORE_ACTIVATION_H_INCLUDED_

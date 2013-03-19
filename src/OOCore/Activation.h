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

#include "RunningObjectTable.h"
#include "Server.h"

namespace OTL
{
	// The following is an expansion of BEGIN_LIBRARY_OBJECT_MAP
	// We don't use the macro as we override some behaviours
	namespace Module
	{
		class OOCore_ModuleImpl : public ProcessModule
		{
			friend class OOBase::Singleton<Module::OOCore_ModuleImpl,OOCore::DLL>;

		public:
			ObjectPtr<OOCore::IInterProcessService> GetIPS();
			bool IsHosted() const;

			void RegisterIPS(ObjectPtr<OOCore::IInterProcessService> ptrIPS, bool bHosted);
			void RevokeIPS();

			Omega::IObject* GetROTObject(const Omega::any_t& oid, const Omega::guid_t& iid);

		private:
			OOCore_ModuleImpl();

			OOBase::SpinLock                               m_lock;
			ObjectPtr<NoLockObjectImpl<OOCore::LocalROT> > m_ptrROT;
			ObjectPtr<OOCore::IInterProcessService>        m_ptrIPS;
			bool                                          m_hosted_by_ooserver;

			ModuleBase::CreatorEntry* getCreatorEntries()
			{
				return NULL;
			}
			static const CreatorEntry* getCoreEntries();
		};
	}

	OMEGA_PRIVATE_FN_DECL(Module::OOCore_ModuleImpl*,GetModule());

	namespace Module
	{
		OMEGA_PRIVATE_FN_DECL(ModuleBase*,GetModuleBase)();
	}
}

#endif // OOCORE_ACTIVATION_H_INCLUDED_

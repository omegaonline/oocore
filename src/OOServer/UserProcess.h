///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
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

#ifndef OOSERVER_USER_PROCESS_H_INCLUDED_
#define OOSERVER_USER_PROCESS_H_INCLUDED_

namespace User
{
	class Process : public OOBase::NonCopyable
	{
	public:
		virtual ~Process() {}

		virtual bool is_running(int& exit_code) = 0;
		virtual void kill() = 0;

		static bool is_invalid_path(const Omega::string_t& strPath);

	protected:
		Process() {}
	};
}

#endif // OOSERVER_USER_PROCESS_H_INCLUDED_

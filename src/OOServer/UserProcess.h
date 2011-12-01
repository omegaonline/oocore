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
	class Process
	{
	public:
		virtual ~Process() {}

		virtual bool running() = 0;
		virtual bool wait_for_exit(const OOBase::timeval_t* wait, int& exit_code) = 0;

		static Process* exec(const wchar_t* pszProcess, Omega::uint32_t envc, const Omega::byte_t* envp);
		static bool is_relative_path(const wchar_t* pszPath);

	protected:
		Process() {}

	private:
		Process(const Process&);
		Process& operator = (const Process&);
	};
}

#endif // OOSERVER_USER_PROCESS_H_INCLUDED_

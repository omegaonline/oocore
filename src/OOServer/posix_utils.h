///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2010 Rick Taylor
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
#ifndef OOSERVER_POSIX_UTILS_INCLUDED_
#define OOSERVER_POSIX_UTILS_INCLUDED_

#if defined(HAVE_UNISTD_H)
uid_t get_directory_user(void);
gid_t get_directory_group(void);
mode_t get_directory_permissions(void);

bool create_unless_existing_directory(  std::string& dir, 
                                        mode_t perm = get_directory_permissions(),
                                        uid_t   uid = get_directory_user(),
                                        gid_t   gid = get_directory_group());
#endif /*HAVE_UNISTD_H */
#endif /*OOSERVER_POSIX_UTILS_INCLUDED_ */

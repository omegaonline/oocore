///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2011 Rick Taylor
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

#ifndef OMEGA_MEMORY_H_INCLUDED_
#define OMEGA_MEMORY_H_INCLUDED_

namespace Omega
{
	namespace System
	{
		void* Allocate(size_t bytes);
		void Free(void* mem);

		namespace Internal
		{
			class ThrowingNew
			{
			public:
				// Custom new and delete
				void* operator new(size_t size)
				{
					return Omega::System::Allocate(size);
				}

				void* operator new[](size_t size)
				{
					return Omega::System::Allocate(size);
				}

				void operator delete(void* p)
				{
					Omega::System::Free(p);
				}

				void operator delete[](void* p)
				{
					Omega::System::Free(p);
				}
			};
		}
	}
}

#endif // OMEGA_MEMORY_H_INCLUDED_

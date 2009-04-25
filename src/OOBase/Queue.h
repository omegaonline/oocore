///////////////////////////////////////////////////////////////////////////////////
//
// Copyright (C) 2009 Rick Taylor
//
// This file is part of OOBase, the Omega Online Base library.
//
// OOBase is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// OOBase is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with OOBase.  If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////////

#ifndef OOBASE_QUEUE_H_INCLUDED_
#define OOBASE_QUEUE_H_INCLUDED_

#include "Condition.h"

#include <queue>

namespace OOBase
{
	template <class T>
	class BoundedQueue
	{
	public:
		enum result_t
		{
			success = 0,
			timedout,
			closed,
			pulsed
		};

		BoundedQueue(size_t bound = 10) :
			m_bound(bound),
			m_closed(false),
			m_pulsed(false)
		{}
		
		result_t push(const T& val, const timeval_t* wait = 0)
		{
			timeval_t wait2;
			if (wait)
				wait2 = *wait;
			Countdown countdown(&wait2);

			m_lock.acquire();

			if (m_closed)
			{
				m_lock.release();
				return closed;
			}
			
			while (m_queue.size() >= m_bound)
			{
				if (!m_space.wait(m_lock,(wait ? &wait2 : 0)))
					return timedout;

				if (m_closed)
				{
					m_lock.release();
					return closed;
				}
			}

			m_queue.push(val);

			m_lock.release();

			m_available.signal(); //broadcast();

			return success;
		}

		result_t pop(T& val, const timeval_t* wait = 0)
		{
			timeval_t wait2;
			if (wait)
				wait2 = *wait;
			Countdown countdown(&wait2);

			m_lock.acquire();

			if (m_pulsed)
			{
				m_lock.release();
				return pulsed;
			}

			while (m_queue.empty())
			{
				if (m_closed)
				{
					m_lock.release();
					return closed;
				}
				
				if (!m_available.wait(m_lock,(wait ? &wait2 : 0)))
					return timedout;

				if (m_pulsed)
				{
					m_lock.release();
					return pulsed;
				}
			}
			
			val=m_queue.front();
			m_queue.pop();
			
			m_lock.release();

			m_space.broadcast();

			return success;
		}

		void close()
		{
			Guard<Condition::Mutex> guard(m_lock);
			m_closed = true;

			guard.release();

			m_available.broadcast();
		}

		void pulse()
		{
			Guard<Condition::Mutex> guard(m_lock);
			m_pulsed = true;

			guard.release();

			m_available.broadcast();
			m_space.broadcast();

			guard.acquire();

			m_pulsed = false;
		}

	private:
		size_t           m_bound;
		bool             m_closed;
		bool             m_pulsed;
		Condition::Mutex m_lock;
		Condition        m_available;
		Condition        m_space;
		std::queue<T>    m_queue;
	};	
}

#endif // OOBASE_QUEUE_H_INCLUDED_

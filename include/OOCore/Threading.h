#ifndef OMEGA_THREADING_H_INCLUDED_
#define OMEGA_THREADING_H_INCLUDED_

namespace Omega
{
	class CriticalSection
	{
	public:
		inline CriticalSection();
		inline ~CriticalSection();

		inline void Lock();
		inline void Unlock();

	private:
		typedef struct tag_handle_t
		{
			int unused;
		}* handle_t;

		handle_t m_handle;
	};

	class Guard
	{
	public:
		Guard(CriticalSection& lock) : m_cs(lock)
		{
			m_cs.Lock();
		}

		~Guard()
		{
			m_cs.Unlock();
		}

	private:
		Guard& operator = (const Guard&) { return *this; }

		CriticalSection& m_cs;
	};

	class ReaderWriterLock
	{
	public:
		inline ReaderWriterLock();
		inline ~ReaderWriterLock();

		inline void LockRead();
		inline void LockWrite();
		inline void Unlock();

	private:
		typedef struct tag_handle_t
		{
			int unused;
		}* handle_t;

		handle_t m_handle;
	};

	class ReadGuard
	{
	public:
		ReadGuard(ReaderWriterLock& lock) : m_lock(lock)
		{
			m_lock.LockRead();
		}

		~ReadGuard()
		{
			m_lock.Unlock();
		}

	private:
		ReadGuard& operator = (const ReadGuard&) { return *this; }

		ReaderWriterLock& m_lock;
	};

	class WriteGuard
	{
	public:
		WriteGuard(ReaderWriterLock& lock) : m_lock(lock)
		{
			m_lock.LockWrite();
		}

		~WriteGuard()
		{
			m_lock.Unlock();
		}

	private:
		WriteGuard& operator = (const WriteGuard&) { return *this; }

		ReaderWriterLock& m_lock;
	};

	template <class T> class AtomicOp
	{
	public:
		AtomicOp() {};
		inline AtomicOp(const T& v);
		inline AtomicOp(const AtomicOp& rhs);

		inline T operator ++();
		inline T operator ++(int);
		inline T operator --();
		inline T operator --(int);
		inline volatile T* operator &();

		inline AtomicOp& operator = (const AtomicOp& rhs);
		inline AtomicOp& operator = (const T& rhs);

		inline T value() const;
		inline volatile T& value();
		inline T exchange(const T& v);

	private:
		mutable CriticalSection m_cs;
		T                       m_value;
	};

	#ifdef OMEGA_HAS_ATOMIC_OP

	template <> class AtomicOp<int32_t>
	{
	public:
		AtomicOp() {}
		inline AtomicOp(const int32_t& v);
		inline AtomicOp(const AtomicOp& rhs);

		inline AtomicOp& operator = (const AtomicOp& rhs);
		inline AtomicOp& operator = (const int32_t& rhs);

		inline int32_t operator ++();
		inline int32_t operator ++(int) { return ++*this - 1; }
		inline int32_t operator --();
		inline int32_t operator --(int) { return --*this + 1; }
		inline volatile int32_t* operator &() { return &m_value; }

		inline int32_t value() const { return m_value; }
		inline volatile int32_t& value()  { return m_value; }
		inline int32_t exchange(const int32_t& v);

	private:
		int32_t m_value;
	};

	template <> class AtomicOp<uint32_t>
	{
	public:
		AtomicOp() {};
		inline AtomicOp(const uint32_t& v);
		inline AtomicOp(const AtomicOp& rhs);

		inline AtomicOp& operator = (const AtomicOp& rhs);
		inline AtomicOp& operator = (const uint32_t& rhs);

		inline uint32_t operator ++();
		inline uint32_t operator ++(int) { return ++*this - 1; }
		inline uint32_t operator --();
		inline uint32_t operator --(int) { return --*this + 1; }
		inline volatile uint32_t* operator &()  { return &m_value; }

		inline uint32_t value() const  { return m_value; }
		inline volatile uint32_t& value()  { return m_value; }
		inline uint32_t exchange(const uint32_t& v);

	private:
		uint32_t	m_value;
	};

	#endif // OMEGA_HAS_ATOMIC_OP
}

#endif // OMEGA_THREADING_H_INCLUDED_

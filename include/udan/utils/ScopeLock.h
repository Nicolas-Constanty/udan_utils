#pragma once

namespace udan
{
	namespace utils
	{
		/**
		 * \brief This lock come from Jayson Gregory's book p320
		 * \tparam LOCK Lock type
		 */
		template<class LOCK>
		class ScopeLock
		{
			typedef LOCK lock_t;
			lock_t* m_plock;
		public:
			explicit ScopeLock(lock_t& lock) : m_plock(&lock)
			{
				m_plock->Lock();
			}

			~ScopeLock()
			{
				m_plock->Unlock();
			}
		};
	}
}

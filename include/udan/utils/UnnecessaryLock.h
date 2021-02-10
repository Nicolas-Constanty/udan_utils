#pragma once

#include <cassert>
#include "ScopeLock.h"

namespace udan
{
	namespace utils
	{
		/**
		 * \brief This class come from Jason Gregory's book p326
		 */
		class UnnecessaryLock
		{
			volatile bool m_locked = false;
		public:
			void Lock()
			{
				assert(!m_locked);
				m_locked = true;
			}

			void Unlock()
			{
				assert(m_locked);
				m_locked = false;
			}
		};

#if DEBUG
#define ASSERT_LOCK_NOT_NECESSARY(L) udan::utils::ScopeLock<decltype(L)>(L)
#else
#define ASSERT_LOCK_NOT_NECESSARY(L)
#endif
	}
}

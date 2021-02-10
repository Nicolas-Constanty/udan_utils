#pragma once


#include "CriticalSectionLock.h"
#include <functional>

namespace udan
{
	namespace utils
	{
		class ConditionVariable
		{
			CONDITION_VARIABLE m_conditionVariable;

		public:
			explicit ConditionVariable()
			{
				InitializeConditionVariable(&m_conditionVariable);
			}

			void Wait(CriticalSectionLock& lock, const std::function<bool()>& predicate)
			{
				while (!predicate())
				{
					SleepConditionVariableCS(
						&m_conditionVariable,
						lock.Handle(),
						INFINITE);
				}
			}

			void NotifyOne()
			{
				WakeConditionVariable(&m_conditionVariable);
			}

			void NotifyAll()
			{
				WakeAllConditionVariable(&m_conditionVariable);
			}

			PCONDITION_VARIABLE Handle()
			{
				return &m_conditionVariable;
			}
		};
	}
}

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
			DWORD m_waitTime;

		public:
			explicit ConditionVariable() : m_waitTime(0)
			{
				InitializeConditionVariable(&m_conditionVariable);
			}

			explicit ConditionVariable(DWORD waitTime) : m_waitTime(waitTime)
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
						m_waitTime);
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

#pragma once

#include <timeapi.h>

#include "udan/debug/uLogger.h"

namespace udan
{
	namespace utils
	{
		template<typename Timer>
		class TimedScope
		{
		public:
			~TimedScope()
			{
				LOG_DEBUG("TimedScope: {} s", m_timer->GetDeltaTime());
			}

		private:
			Timer m_timer;
		};
	}
}

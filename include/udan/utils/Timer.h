#pragma once

#include <timeapi.h>
#include <windows.h>

namespace udan
{
	namespace utils
	{
		class Timer
		{
		public:
			explicit Timer()
			{
				timeBeginPeriod(1);
				LARGE_INTEGER frequency;
				QueryPerformanceFrequency(&frequency);
				m_frequency = static_cast<double>(frequency.QuadPart);
				m_time = GetCurrentTimeSeconds();
			}

			[[nodiscard]] double GetDeltaTime() const
			{
				return GetCurrentTimeSeconds() - m_time;
			}

			void Reset()
			{
				m_time = GetCurrentTimeSeconds();
			}

		private:
			[[nodiscard]] double GetCurrentTimeSeconds() const
			{
				LARGE_INTEGER time;
				QueryPerformanceCounter(&time);
				return  static_cast<double>(time.QuadPart) / m_frequency;
			}
			double m_frequency;
			double m_time;
		};
	}
}

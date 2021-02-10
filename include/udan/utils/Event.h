#pragma once
#include <functional>

namespace udan
{
	namespace utils
	{
		template<typename ... Args>
		class Event
		{
		public:
			void Invoke(Args&... args)
			{
				for (auto& func : m_observers)
					func(std::forward<Args>(args)...);
			}

			void Register(const std::function<void(Args...)>& func)
			{
				m_observers.emplace_back(func);
			}

			Event& operator+=(const std::function<void(Args...)>& func)
			{
				m_observers.emplace_back(func);
				return *this;
			}

			~Event()
			{
				m_observers.clear();
			}

		private:
			std::vector<std::function<void(Args...)>> m_observers;
		};
	}
}

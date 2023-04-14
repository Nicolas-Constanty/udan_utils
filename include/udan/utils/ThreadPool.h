#pragma once

#include <map>
#include <queue>
#include <set>
#include <thread>
#include <vector>


#include "ConditionVariable.h"
#include "Task.h"

namespace udan
{
	namespace utils
	{
		class ThreadPool
		{
		public:
			__declspec(dllexport) ThreadPool(size_t capacity);
			/**
			 * \brief This function finish running, then stop threads and join
			 */
			__declspec(dllexport) void Stop();
			__declspec(dllexport) void StopWhenQueueEmpty();
			__declspec(dllexport) void WaitUntilQueueEmpty();

			/**
			 * \brief This function may lead to UB since thread are directly killed prefer Stop over Interrupt
			 * (https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-terminatethread)
			 */
#if DEBUG
			void Interrupt();
#endif
			__declspec(dllexport) void BulkSchedule(const std::vector<std::shared_ptr<ATask>>& task);
			__declspec(dllexport) void Schedule(const std::shared_ptr<ATask>& task);
			__declspec(dllexport) void ResetTaskCount();
			__declspec(dllexport) size_t GetThreadCount() const;

			//void ThreadPool::Print();
		private:
			void ScheduleCompletedDependency(const std::shared_ptr<ATask>& task);
			void Run();
			std::vector<std::thread> m_threads;
			ConditionVariable m_cv;
			ConditionVariable m_queueEmpty;
			CriticalSectionLock m_mtx;
			CriticalSectionLock m_mtx_remaining;
			bool m_shouldRun;

			std::priority_queue<std::shared_ptr<ATask>, std::vector<std::shared_ptr<ATask>>, std::greater<>> m_tasks;
			std::set<size_t> m_remainingTasks;
		};
	}
}

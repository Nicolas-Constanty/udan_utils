#include "udan/utils/ThreadPool.h"


#include "udan/utils/ScopeLock.h"
#include "udan/debug/uLogger.h"

#include <iostream>

namespace udan
{
	namespace utils
	{
		ThreadPool::ThreadPool(size_t capacity) : m_cv(INFINITE), m_queueEmpty(INFINITE)
		{
			m_shouldRun = true;
			m_threads.reserve(capacity);
			for (size_t i = 0; i < capacity; ++i)
			{
				m_threads.emplace_back([this] { Run(); });
			}
			LOG_INFO("Threadpool launching {} threads...", capacity);
		}

		void ThreadPool::Stop()
		{
			{
				ScopeLock<decltype(m_mtx)> lck(m_mtx);
				m_shouldRun = false;
			}
			m_cv.NotifyAll();
			for (auto& thread : m_threads)
			{
				thread.join();
			}
		}

		void ThreadPool::StopWhenQueueEmpty()
		{
			WaitUntilQueueEmpty();
			Stop();
		}

		void ThreadPool::WaitUntilQueueEmpty()
		{
			ScopeLock<decltype(m_mtx_remaining)> lck(m_mtx_remaining);
			m_queueEmpty.Wait(m_mtx_remaining, [this]() { return m_remainingTasks.empty(); });
			LOG_INFO("Exit wait");

		}
#if DEBUG
		void ThreadPool::Interrupt()
		{
			ScopeLock<decltype(m_mtx)> lck(m_mtx);
			m_shouldRun = false;
			for (auto& thread : m_threads)
			{
				if (TerminateThread(thread.native_handle(), 0) != 0)
				{
					LOG_ERR(GetErrorString());
				}
			}
		}
#endif 
		/*void ThreadPool::Print()
		{
			while (!m_tasks.empty())
			{
				LOG_DEBUG("{}", m_tasks.top()->GetPriority());
				m_tasks.pop();
			}
		}*/

		void ThreadPool::BulkSchedule(const std::vector<std::shared_ptr<ATask>>& tasks)
		{
			ScopeLock<decltype(m_mtx)> lck(m_mtx);
			for (const auto& task : tasks)
			{
				auto dt = std::dynamic_pointer_cast<DependencyTask>(task);
				if (dt != nullptr && !dt->Dependencies().empty())
				{
					for (const auto& dependency : dt->Dependencies())
					{
						if (!dependency->Completed())
						{
							dependency->onCompleted += [this, dt, dependency, task]()
							{
								if (dt->RemoveDependency(dependency))
									ScheduleCompletedDependency(task);
							};
							break;
						}
					}
				}
#if DEBUG
				//LOG_DEBUG("Schedule task {}: ", task->GetId());
				auto debugTask = std::make_shared<DebugTaskDecorator>(task);
				m_tasks.push(debugTask);
				{
					ScopeLock<decltype(m_mtx_remaining)> lck(m_mtx_remaining);
					m_remainingTasks.insert(debugTask->GetId());
					LOG_INFO("Remnaining size: {}", m_remainingTasks.size());
				}
#else
				m_tasks.push(task);
				{
					ScopeLock<decltype(m_mtx_remaining)> lck(m_mtx_remaining);
					m_remainingTasks.insert(task->GetId());
					LOG_INFO("Remnaining size: {}", m_remainingTasks.size());
				}
#endif
			}
			m_cv.NotifyAll();
		}

		void ThreadPool::Schedule(const std::shared_ptr<ATask>& task)
		{
			ScopeLock<decltype(m_mtx)> lck(m_mtx);
			auto dt = std::dynamic_pointer_cast<DependencyTask>(task);
			if (dt != nullptr && !dt->Dependencies().empty())
			{
				auto ready = true;
				for (const auto& dependency : dt->Dependencies())
				{
					if (!dependency->Completed())
					{
						ready = false;
						dependency->onCompleted += [this, dt, dependency, task]()
						{
							if (dt->RemoveDependency(dependency))
							{
								Schedule(task);
							}
						};
					}
				}
				if (ready)
				{
					ScheduleCompletedDependency(task);
				}
			}
			else
				ScheduleCompletedDependency(task);
		}

		void ThreadPool::ResetTaskCount()
		{
			ATask::ResetId();
		}

		size_t ThreadPool::GetThreadCount() const
		{
			return m_threads.size();
		}

		void ThreadPool::ScheduleCompletedDependency(const std::shared_ptr<ATask>& task)
		{
#if DEBUG
			//LOG_DEBUG("Schedule task {}: ", task->GetId());
			auto debugTask = std::make_shared<DebugTaskDecorator>(task);
			{
				ScopeLock<decltype(m_mtx)> lck(m_mtx);
				m_tasks.push(debugTask);
			}
			m_cv.NotifyOne();
			{
				ScopeLock<decltype(m_mtx_remaining)> lck(m_mtx_remaining);
				m_remainingTasks.insert(debugTask->GetId());
				LOG_INFO("Remnaining size: {}", m_remainingTasks.size());
			}
#else
			{
				ScopeLock<decltype(m_mtx)> lck(m_mtx);
				m_tasks.push(task);
			}
			m_cv.NotifyOne();
			{
				ScopeLock<decltype(m_mtx_remaining)> lck(m_mtx_remaining);
				m_remainingTasks.insert(task->GetId());
				LOG_INFO("Remnaining size: {}", m_remainingTasks.size());
			}
#endif	
		}

		void ThreadPool::Run()
		{
			LOG_INFO("Start thread {}", GetCurrentThreadId());
			while (m_shouldRun)
			{
				bool notify = false;
				{
					std::shared_ptr<ATask> task;
					{
						ScopeLock<decltype(m_mtx)> lck(m_mtx);
						m_cv.Wait(m_mtx, [&]()
							{
								return !m_tasks.empty() || !m_shouldRun;
							});
						if (!m_shouldRun)
							break;
						task = m_tasks.top();
						m_tasks.pop();
					}
					task->Exec();
					{
						ScopeLock<decltype(m_mtx_remaining)> lck(m_mtx_remaining);
						m_remainingTasks.erase(task->GetId());
						LOG_INFO("Check if empty");
						if (m_remainingTasks.empty())
						{
							LOG_INFO("No remaining tasks {}", GetCurrentThreadId());
							notify = true;
						}
					}
				}
				if (notify)
					m_queueEmpty.NotifyOne();
			}
			LOG_INFO("Exit thread {}", GetCurrentThreadId());
		}
	}
}
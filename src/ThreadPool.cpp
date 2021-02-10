#include "udan/utils/ThreadPool.h"

#include <iostream>


#include "udan/utils/ScopeLock.h"
#include "udan/utils/WindowsApi.h"
#include "udan/debug/uLogger.h"

namespace udan
{
	namespace utils
	{
		ThreadPool::ThreadPool(size_t capacity)
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
				m_cv.NotifyOne();
			}
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
			ScopeLock<decltype(m_mtx)> lck(m_mtx);
			m_queueEmpty.Wait(m_mtx, [this]() { return m_remainingTasks.empty(); });
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

		void ThreadPool::Schedule(const std::shared_ptr<ATask>& task)
		{
			ScopeLock<decltype(m_mtx)> lck(m_mtx);
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
			m_remainingTasks.insert(debugTask->GetId());
#else
			m_tasks.push(task);
			m_remainingTasks.insert(task->GetId());
#endif
			LOG_INFO("Remnaining size: {}", m_remainingTasks.size());
			m_cv.NotifyOne();
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
			ScopeLock<decltype(m_mtx)> lck(m_mtx);
#if DEBUG
			//LOG_DEBUG("Schedule task {}: ", task->GetId());
			auto debugTask = std::make_shared<DebugTaskDecorator>(task);
			m_tasks.push(debugTask);
			m_remainingTasks.insert(debugTask->GetId());
#else
			m_tasks.push(task);
			m_remainingTasks.insert(task->GetId());
#endif	
			m_cv.NotifyOne();
		}

		void ThreadPool::Run()
		{
			LOG_INFO("Start thread {}", GetCurrentThreadId());
			while (m_shouldRun)
			{
				std::shared_ptr<ATask> task;
				{
					ScopeLock<decltype(m_mtx)> lck(m_mtx);
					m_cv.Wait(m_mtx, [this]()
						{
							return !m_tasks.empty() || !m_shouldRun;
						});
					if (!m_shouldRun)
						break;
					task = m_tasks.top();
					m_tasks.pop();
				}
				m_cv.NotifyOne();
				task->Exec();
				{
					ScopeLock<decltype(m_mtx)> lck(m_mtx);
					m_remainingTasks.erase(task->GetId());
					LOG_INFO("Check if empty");
					if (m_remainingTasks.empty())
					{
						LOG_INFO("No remaining tasks {}", GetCurrentThreadId());
						m_queueEmpty.NotifyOne();
					}
				}
			}
			LOG_INFO("Exit thread {}", GetCurrentThreadId());
		}
	}
}
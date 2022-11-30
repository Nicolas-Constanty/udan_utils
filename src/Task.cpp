#include "udan/utils/Task.h"
#include "udan/utils/ScopeLock.h"
#include "udan/debug/uLogger.h"

namespace udan
{
	namespace utils
	{
		uint64_t ATask::m_taskId = 1;
		ATask::ATask(TaskPriority priority, size_t task_id) :
			m_priority(priority),
			m_id(task_id == 0 ? m_taskId++ : task_id),
			m_completed(false)
		{
		}

		ATask::~ATask() = default;

		Task::Task(std::function<void()> task_function, TaskPriority priority) :
			ATask(priority),
			m_task(std::move(task_function))
		{
		}

		Task::~Task()
		{
		}

		void Task::Exec()
		{
			m_task();
			Done();
		}

		DependencyTask::DependencyTask(
			std::function<void()> task_function,
			const DependencyVector& tasks,
			TaskPriority priority) :
			Task(std::move(task_function), priority)
		{
			for (const auto& task : tasks)
			{
				if (task.get() == this)
					continue;
				m_dependencies.emplace_back(task);
			}
		}

		DependencyTask::~DependencyTask()
		{
			m_dependencies.clear();
		}

		bool DependencyTask::RemoveDependency(const std::shared_ptr<ATask>& task)
		{
			ScopeLock<decltype(m_mtx)> lck(m_mtx);
			m_dependencies.remove(task);
			return m_dependencies.empty();
		}

		const std::list<std::shared_ptr<ATask>>& DependencyTask::Dependencies() const
		{
			return m_dependencies;
		}

		DebugTaskDecorator::DebugTaskDecorator(const std::shared_ptr<ATask>& task) :
			ATask(task->GetPriority(), task->GetId()),
			m_task(task)
		{
		}

		DebugTaskDecorator::~DebugTaskDecorator()
		{
		}

		void DebugTaskDecorator::Exec()
		{
			LOG_DEBUG("Starting task {}", m_task->GetId());
			m_task->Exec();
			LOG_DEBUG("Task {} has been executed", m_task->GetId());
		}
	}
}

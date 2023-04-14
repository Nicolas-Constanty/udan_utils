#pragma once
#include <condition_variable>
#include <functional>
#include <list>
#include <vector>


#include "CriticalSectionLock.h"
#include "Event.h"

namespace udan
{
	namespace utils
	{
		enum class TaskPriority : uint16_t
		{
			LOW = 0,
			NORMAL = 1,
			HIGH = 2,
			CRITICAL = 3
		};

		class ATask
		{
		public:
			Event<> onCompleted;

			__declspec(dllexport) explicit ATask(TaskPriority priority = TaskPriority::NORMAL, size_t task_id = 0);
			__declspec(dllexport) virtual ~ATask();
			__declspec(dllexport) virtual void Exec() = 0;
			[[nodiscard]] TaskPriority GetPriority() const
			{
				return m_priority;
			}
			[[nodiscard]] uint64_t GetId() const
			{
				return m_id;
			}
			[[nodiscard]] bool Completed() const
			{
				return m_completed;
			}
			static void ResetId()
			{
				m_taskId = 0;
			}

			void Done()
			{
				m_completed = true;
				onCompleted.Invoke();
			}
		private:
			TaskPriority m_priority;
			uint64_t m_id;
			bool m_completed;

			static uint64_t m_taskId;
		};

		inline bool operator<(const std::shared_ptr<ATask>& lhs, const std::shared_ptr <ATask>& rhs)
		{
			return lhs->GetPriority() > rhs->GetPriority();
		}

		inline bool operator>(const std::shared_ptr<ATask>& lhs, const std::shared_ptr < ATask>& rhs)
		{
			return lhs->GetPriority() < rhs->GetPriority();
		}

		class Task : public ATask
		{
		public:
			__declspec(dllexport) explicit Task(std::function<void()> task_function, TaskPriority priority = TaskPriority::NORMAL);
			__declspec(dllexport) ~Task() override;
			__declspec(dllexport) void Exec() override;

		private:
			std::function<void()> m_task;
		};

		typedef std::vector<std::shared_ptr<ATask>> DependencyVector;
		/// <summary>
		/// This task can specify some other tasks dependencies
		/// </summary>
		class DependencyTask final : public Task
		{
		public:
			__declspec(dllexport) explicit DependencyTask(
				std::function<void()> task_function,
				const DependencyVector& tasks = {},
				TaskPriority priority = TaskPriority::NORMAL);
			__declspec(dllexport) ~DependencyTask()  override;
			__declspec(dllexport) bool RemoveDependency(const std::shared_ptr<ATask>& task);
			[[nodiscard]] __declspec(dllexport)  const std::list<std::shared_ptr<ATask>>& Dependencies() const;

		private:
			std::list <std::shared_ptr<ATask>> m_dependencies;
			CriticalSectionLock m_mtx;
		};

		class DebugTaskDecorator : public ATask
		{
		public:
			__declspec(dllexport) explicit DebugTaskDecorator(const std::shared_ptr<ATask>& task);
			__declspec(dllexport) ~DebugTaskDecorator() override;
			__declspec(dllexport) void Exec() override;

		private:
			std::shared_ptr<ATask> m_task;
		};
	}
}
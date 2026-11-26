#pragma once
#include <FoundationEngine/JobSystem/JobDeclaretions.h>
#include <FoundationEngine/JobSystem/JobConcept.h>
#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobNode.h>

namespace SeedCore
{
	enum class JobTaskType :Int
	{
		PLACEHOLDER = 0,

		STATIC,

		RUNTIME,

		SUBFLOW,

		CONDITION,

		MODULE,

		UNDEFINED
	};

	inline constexpr auto JOB_TASK_TYPES = std::to_array<JobTaskType>
		({
			JobTaskType::PLACEHOLDER,
			JobTaskType::STATIC,
			JobTaskType::RUNTIME,
			JobTaskType::SUBFLOW,
			JobTaskType::CONDITION,
			JobTaskType::MODULE,
		});

	SEEDCORE_API const Char* ToString(JobTaskType type);

	template<typename C>
	concept StaticTaskLike = std::invocable<C> && std::same_as<std::invoke_result_t<C>, void>;

	template<typename C>
	constexpr Bool IsStaticTaskValue = StaticTaskLike<C>;

	template<typename C>
	concept SubflowTaskLike = std::invocable<C, Subflow&>&& std::same_as<std::invoke_result_t<C, Subflow&>, void>;

	template<typename C>
	constexpr Bool IsSubflowTaskValue = SubflowTaskLike<C>;

	template<typename C>
	concept RuntimeTaskLike =
		(std::invocable<C, JobPreemptiveRuntime&> && std::same_as<std::invoke_result_t<C, JobPreemptiveRuntime&>, void>) ||
		(std::invocable<C, JobNonpreemptiveRuntime&> && std::same_as<std::invoke_result_t<C, JobNonpreemptiveRuntime&>, void>);

	template<typename C>
	constexpr Bool IsRuntimeTaskValue = RuntimeTaskLike<C>;

	template<typename C>
	concept SingleConditionTaskLike = std::invocable<C> && std::convertible_to<std::invoke_result_t<C>, Int>;

	template<typename C>
	constexpr Bool IsSingleConditionTaskValue = SingleConditionTaskLike<C>;

	template<typename C>
	concept MultiConditionTaskLike = std::invocable<C> && std::same_as<std::invoke_result_t<C>, HybridArray<Int>>;

	template<typename C>
	constexpr Bool IsMultiConditionTaskValue = MultiConditionTaskLike<C>;

	class SEEDCORE_API JobTask
	{
	private:
		friend class FlowBuilder;
		friend class JobPreemptiveRuntime;
		friend class JobNonpreemptiveRuntime;
		friend class JobTaskflow;
		friend class JobTaskView;
		friend class JobExecutor;

	public:
		JobTask() = default;

		JobTask(const JobTask& rhs);

		JobTask& operator=(const JobTask& rhs);

		JobTask& operator=(std::nullptr_t null);

		Bool operator==(const JobTask& rhs)const;

		Bool operator!=(const JobTask& rhs)const;

		const String& Name()const;

		Size NumberSuccessors()const;

		Size NumberPredecessors()const;

		Size NumberStrongDependencies()const;

		Size NumberWeakDependencies()const;

		JobTask& Name(const String& name);

		template<typename C>
		JobTask& Work(C&& callable)
		{
			if constexpr (IsStaticTaskValue<C>)
			{
				node_->handle_.emplace<JobNode::Static>(std::forward<C>(callable));
			}
			else if constexpr (IsRuntimeTaskValue<C>)
			{
				if constexpr (std::is_invocable_v<C, JobPreemptiveRuntime&>)
				{
					node_->handle_.emplace<JobNode::PreemptiveRuntime>(std::forward<C>(callable));
				}
				else
				{
					node_->handle_.emplace<JobNode::NonpreemptiveRuntime>(std::forward<C>(callable));
				}
			}
			else if constexpr (IsSubflowTaskValue<C>)
			{
				node_->handle_.emplace<JobNode::Subflow>(std::forward<C>(callable));
			}
			else if constexpr (IsSingleConditionTaskValue<C>)
			{
				node_->handle_.emplace<JobNode::SingleCondition>(std::forward<C>(callable));
			}
			else if constexpr (IsMultiConditionTaskValue<C>)
			{
				node_->handle_.emplace<JobNode::MultiCondition>(std::forward<C>(callable));
			}
			else
			{

			}
			return *this;
		}

		template<GraphLike T>
		JobTask& Composed(T& target)
		{
			node_->handle_.emplace<JobNode::OwnedModule>(RetrieveGraph(target));
			return *this;
		}

		JobTask& Adopt(JobGraph&& graph);

		template<typename... Ts>
		JobTask& Precede(Ts&&... tasks)
		{
			(node_->Precede(tasks.node_), ...);
			return *this;
		}

		template<typename... Ts>
		JobTask& Succeed(Ts&&... tasks)
		{
			(tasks.node_->Precede(node_), ...);
			return *this;
		}

		template<typename... Ts>
		JobTask& RemovePredecessors(Ts&&... tasks)
		{
			(tasks.node_->RemoveSuccessors(node_), ...);
			(node_->RemovePredecessors(tasks.node_), ...);
			return *this;
		}

		template<typename... Ts>
		JobTask& RemoveSuccessors(Ts&&... tasks)
		{
			(node_->RemoveSuccessors(tasks.node_), ...);
			(tasks.node_->RemovePredecessors(node_), ...);
			return *this;
		}

		JobTask& Release(Semaphore& semaphore);

		template<typename I>
		JobTask& Release(I first, I last)
		{
			if (!node_->semaphores_)
			{
				node_->semaphores_ = std::make_unique<JobNode::Semaphores>();
			}
			node_->semaphores_->release_.reserve(node_->semaphores_->release_.size() + std::distance(first, last));
			for (auto s = first;s != last;++s)
			{
				node_->semaphores_->release_.push_back(&(*s));
			}
			return *this;
		}

		JobTask& Acquire(Semaphore& semaphore);

		template<typename I>
		JobTask& Acquire(I first, I last)
		{
			if (!node_->semaphores_)
			{
				node_->semaphores_ = std::make_unique<JobNode::Semaphores>();
			}

			node_->semaphores_->acquire_.reserve(node_->semaphores_->acquire_.size() + std::distance(first, last));
			for (auto s = first;s != last;++s)
			{
				node_->semaphores_->acquire_.push_back(&(*s));
			}
			return *this;
		}

		JobTask& Data(void* data);

		void Reset();

		void ResetWork();

		Bool Empty()const;

		Bool HasWork()const;

		template<typename V>
		void EachSuccessor(V&& visitor)const
		{
			for (Size index = 0;index < node_->numberSuccessors_;++index)
			{
				visitor(JobTask(node_->edges_[index]));
			}
		}

		template<typename V>
		void EachPredecessor(V&& visitor)const
		{
			for (Size index = node_->numberSuccessors_;index < node_->edges_.size();++index)
			{
				visitor(JobTask(node_->edges_[index]));
			}
		}

		template<typename V>
		void EachSubflowTask(V&& visitor)const
		{
			if (auto ptr = std::get_if<JobNode::Subflow>(&node_->handle_);ptr)
			{
				for (auto iterator = ptr->subgraph_.begin();iterator != ptr->subgraph_.end();++iterator)
				{
					visitor(JobTask(*iterator));
				}
			}
		}

		Size HashValue()const;

		JobTaskType Type()const;

		void* Data()const;

		std::exception_ptr ExceptionPtr()const;

		Bool HasExceptionPtr()const;

	private:
		JobTask(JobNode* node);

		JobNode* node_ = nullptr;
	};

	class JobTaskView
	{
	private:
		friend class JobExecutor;

	public:
		const String& Name()const;

		Size NumberSuccessors()const;

		Size NumberPredecessors()const;

		Size NumberStrongDependencies()const;

		Size NumberWeakDependencies()const;

		template<typename V>
		void EachSuccessor(V&& visitor)const
		{
			for (Size index = 0;index < node_.numberSuccessors_;++index)
			{
				visitor(JobTaskView(*node_.edges_[index]));
			}
		}

		template<typename V>
		void EachPredecessor(V&& visitor)const
		{
			for (Size index = node_.numberSuccessors_;index < node_.edges_.size();++index)
			{
				visitor(JobTaskView(*node_.edges_[index]));
			}
		}

		JobTaskType Type()const;

		Size HashValue()const;

	private:
		JobTaskView(const JobNode& node);

		JobTaskView(const JobTaskView&) = default;

		const JobNode& node_;
	};
}

namespace std
{
	template<>
	struct hash<SeedCore::JobTask>
	{
		auto operator()(const SeedCore::JobTask& task)const noexcept
		{
			return task.HashValue();
		}
	};

	template<>
	struct hash<SeedCore::JobTaskView>
	{
		auto operator()(const SeedCore::JobTaskView& taskView)const noexcept
		{
			return taskView.HashValue();
		}
	};
}

#pragma once
#include <FoundationEngine/JobSystem/JobDeclaretions.h>
#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobTask.h>

namespace SeedCore
{
	class SEEDCORE_API FlowBuilder
	{
	private:
		friend class JobExecutor;

	public:
		FlowBuilder(JobGraph& graph);

		template<StaticTaskLike C>
		JobTask emplace(C&& callable)
		{
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::Static>{}, std::forward<C>(callable)));
		}

		template<RuntimeTaskLike C>
		JobTask emplace(C&& callable)
		{
			if constexpr (std::is_invocable_v<C, JobPreemptiveRuntime&>)
			{
				return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::PreemptiveRuntime>{}, std::forward<C>(callable)));
			}
			else if constexpr (std::is_invocable_v<C, JobNonpreemptiveRuntime&>)
			{
				return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::NonpreemptiveRuntime>{}, std::forward<C>(callable)));
			}
		}

		template<SubflowTaskLike C>
		JobTask emplace(C&& callable)
		{
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::Subflow>{}, std::forward<C>(callable)));
		}

		template<SingleConditionTaskLike C>
		JobTask emplace(C&& callable)
		{
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::SingleCondition>{}, std::forward<C>(callable)));
		}

		template<MultiConditionTaskLike C>
		JobTask emplace(C&& callable)
		{
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::MultiCondition>{}, std::forward<C>(callable)));
		}

		template<typename... C>
			requires(sizeof...(C) > 1)
		auto emplace(C&&... callables)
		{
			return std::make_tuple(emplace(std::forward<C>(callables))...);
		}

		void Erase(JobTask task);

		template<GraphLike C>
		JobTask Composed(C& callable)
		{
			return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::OwnedModule>{}, RetrieveGraph(callable)));
		}

		JobTask Adopt(JobGraph&& graph);

		JobTask Placeholder();

		void Linearize(DynamicArray<JobTask>& keys);

		void Linearize(std::initializer_list<JobTask>& keys);

	protected:
		JobGraph& graph_;

	private:
		template<typename L>
		void Linearize(L& keys)
		{
			auto iterator = keys.begin();
			auto end = keys.end();

			if (iterator == end)
			{
				return;
			}

			auto next = iterator;

			for (++next;next != end;++next, ++iterator)
			{
				iterator->node_->Precede(next->node_);
			}
		}
	};

	class JobSubflow :public FlowBuilder
	{
	private:
		friend class JobExecutor;
		friend class FlowBuilder;

	public:
		void Join();

		Bool Joinable()const noexcept;

		JobExecutor& Executor()noexcept;

		JobGraph& Graph();

		void Retain(Bool flag)noexcept;

		Bool Retain()const;

	private:
		JobSubflow(JobExecutor& executor, JobWorker& worker, JobNode* node, JobGraph& graph);

		JobSubflow() = delete;

		JobSubflow(const JobSubflow&) = delete;

		JobSubflow(JobSubflow&&) = delete;

		JobExecutor& executor_;

		JobWorker& worker_;

		JobNode* node_;
	};
}

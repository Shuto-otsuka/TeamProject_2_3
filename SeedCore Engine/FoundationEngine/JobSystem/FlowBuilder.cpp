#include <FoundationEngine/JobSystem/FlowBuilder.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/Log/Exeption.h>

namespace SeedCore
{
	FlowBuilder::FlowBuilder(JobGraph& graph) :graph_(graph)
	{
		/// No Code
	}

	void FlowBuilder::Erase(JobTask task)
	{
		if (!task.node_)
		{
			return;
		}

		for (Size index = 0;index < task.node_->numberSuccessors_;++index)
		{
			task.node_->edges_[index]->RemovePredecessors(task.node_);
		}

		for (Size index = task.node_->numberSuccessors_;index < task.node_->edges_.size();++index)
		{
			task.node_->edges_[index]->RemoveSuccessors(task.node_);
		}

		graph_.erase(task.node_);
	}

	JobTask FlowBuilder::Adopt(JobGraph&& graph)
	{
		return JobTask(graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::AdoptedModule>{}, std::move(graph)));
	}

	JobTask FlowBuilder::Placeholder()
	{
		auto node = graph_.emplace_back(JobNodeState::NONE, JobExceptionState::NONE, DefaultTaskParams{}, nullptr, nullptr, 0, std::in_place_type_t<JobNode::Placeholder>{});
		return JobTask(node);
	}

	void FlowBuilder::Linearize(DynamicArray<JobTask>& keys)
	{
		this->Linearize<DynamicArray<JobTask>>(keys);
	}

	void FlowBuilder::Linearize(std::initializer_list<JobTask>& keys)
	{
		this->Linearize<std::initializer_list<JobTask>>(keys);
	}
}

namespace SeedCore
{
	JobSubflow::JobSubflow(JobExecutor& executor, JobWorker& worker, JobNode* node, JobGraph& graph) :FlowBuilder(graph), executor_(executor), worker_(worker), node_(node)
	{
		node_->nstate_ &= ~(JobNodeState::JOINED_SUBFLOW | JobNodeState::RETAIN_SUBFLOW);

		graph.clear();
	}

	void JobSubflow::Join()
	{
		if (!Joinable())
		{
			SC_THROW("サブフローは既に合流済みです。");
		}

		executor_.CorunGraph(worker_, graph_, node_->topology_, node_);

		node_->nstate_ |= JobNodeState::JOINED_SUBFLOW;
	}

	Bool JobSubflow::Joinable()const noexcept
	{
		return !(node_->nstate_ & JobNodeState::JOINED_SUBFLOW);
	}

	JobExecutor& JobSubflow::Executor()noexcept
	{
		return executor_;
	}

	JobGraph& JobSubflow::Graph()
	{
		return graph_;
	}

	void JobSubflow::Retain(Bool flag)noexcept
	{
		if (flag)
		{
			node_->nstate_ |= JobNodeState::RETAIN_SUBFLOW;
		}
		else
		{
			node_->nstate_ &= ~JobNodeState::RETAIN_SUBFLOW;
		}
	}

	Bool JobSubflow::Retain()const
	{
		return node_->nstate_ & JobNodeState::RETAIN_SUBFLOW;
	}
}

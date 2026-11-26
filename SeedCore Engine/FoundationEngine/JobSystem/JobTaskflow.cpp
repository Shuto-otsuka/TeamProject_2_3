#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	JobTaskflow::JobTaskflow(const String& name) :FlowBuilder(graph_), name_(name)
	{
		/// No Code
	}

	JobTaskflow::JobTaskflow() :FlowBuilder(graph_)
	{
		/// No Code
	}

	JobTaskflow::JobTaskflow(JobTaskflow&& rhs) :FlowBuilder(graph_)
	{
		std::scoped_lock<std::mutex> lock(rhs.mutex_);
		name_ = std::move(rhs.name_);
		graph_ = std::move(rhs.graph_);
		topologies_ = std::move(rhs.topologies_);
	}

	JobTaskflow& JobTaskflow::operator=(JobTaskflow&& rhs)
	{
		if (this != &rhs)
		{
			std::scoped_lock<std::mutex, std::mutex> lock(mutex_, rhs.mutex_);
			name_ = std::move(rhs.name_);
			graph_ = std::move(rhs.graph_);
			topologies_ = std::move(rhs.topologies_);
		}
		return *this;
	}

	Size JobTaskflow::NumberTasks()const
	{
		return graph_.size();
	}

	Bool JobTaskflow::Empty()const
	{
		return graph_.empty();
	}

	void JobTaskflow::Name(const String& name)
	{
		name_ = name;
	}

	const String& JobTaskflow::Name()const
	{
		return name_;
	}

	void JobTaskflow::Clear()
	{
		graph_.clear();
	}

	void JobTaskflow::RemoveDependency(JobTask from, JobTask to)
	{
		from.node_->RemoveSuccessors(to.node_);

		to.node_->RemovePredecessors(from.node_);
	}

	JobGraph& JobTaskflow::Graph()
	{
		return graph_;
	}

	Size JobTaskflow::FetchEnqueue(ResourceRef<JobTopology> topologies)
	{
		std::lock_guard<std::mutex> lock(mutex_);

		auto preSize = topologies_.size();
		topologies_.emplace(std::move(topologies));
		return preSize;
	}
}

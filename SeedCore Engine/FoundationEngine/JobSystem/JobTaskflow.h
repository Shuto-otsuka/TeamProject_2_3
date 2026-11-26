#pragma once
#include <FoundationEngine/JobSystem/FlowBuilder.h>

namespace SeedCore
{
	class SEEDCORE_API JobTaskflow :public FlowBuilder
	{
	private:
		friend class JobTopology;
		friend class JobExecutor;
		friend class FlowBuilder;
		friend class Subflow;

	public:
		JobTaskflow(const String& name);

		JobTaskflow();

		JobTaskflow(JobTaskflow&& rhs);

		JobTaskflow& operator=(JobTaskflow&& rhs);

		~JobTaskflow() = default;

		Size NumberTasks()const;

		Bool Empty()const;

		void Name(const String& name);

		const String& Name()const;

		void Clear();

		void RemoveDependency(JobTask from, JobTask to);

		JobGraph& Graph();

	private:
		mutable std::mutex mutex_;

		String name_;

		JobGraph graph_;

		std::queue<ResourceRef<JobTopology>> topologies_;

		Size FetchEnqueue(ResourceRef<JobTopology> topologies);
	};

	template<typename T>
	class JobFuture :public std::future<T>
	{
	private:
		friend class JobExecutor;
		friend class Subflow;
		friend class JobPreemptiveRuntime;

	public:
		JobFuture() = default;

		JobFuture(const JobFuture&) = delete;

		JobFuture(JobFuture&&) = default;

		JobFuture(std::future<T>&& future) :std::future<T>(std::move(future))
		{
			/// No Code
		}

		JobFuture& operator=(const JobFuture&) = delete;

		JobFuture& operator=(JobFuture&&) = default;

		Bool Cancel()
		{
			if (topology_)
			{
				topology_->estate_.fetch_or(JobExceptionState::CANCELLED, std::memory_order_relaxed);
				return true;
			}
			return false;
		}

	private:
		ResourceRef<JobTopology> topology_;

		JobFuture(std::future<T>&& future, ResourceRef<JobTopology> pointer) :std::future<T>(std::move(future)), topology_(std::move(pointer))
		{
			/// No Code
		}
	};
}

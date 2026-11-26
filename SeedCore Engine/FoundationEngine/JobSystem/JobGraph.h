#pragma once
#include <FoundationEngine/JobSystem/JobConcept.h>

namespace SeedCore
{
	class TaskParams
	{
	public:
		String name_;

		void* data_ = nullptr;
	};

	class DefaultTaskParams {};

	template<typename P>
	concept TaskParamsLike = std::same_as<std::decay_t<P>, TaskParams> || std::same_as<std::decay_t<P>, DefaultTaskParams> || StringLike<P>;

	template<typename P>
	constexpr Bool IsTaskParamsValue = TaskParamsLike<P>;

	class SEEDCORE_API JobGraph
	{
	private:
		friend class JobNode;
		friend class FlowBuilder;
		friend class Subflow;
		friend class JobTaskflow;
		friend class JobExecutor;

	private:
		using Iterator = DynamicArray<JobNode*>::iterator;

		using ConstIterator = DynamicArray<JobNode*>::const_iterator;

		DynamicArray<JobNode*> nodes_;

	public:
		JobGraph() = default;

		~JobGraph();

		JobGraph(const JobGraph&) = delete;

		JobGraph(JobGraph&& other);

		JobGraph& operator=(const JobGraph&) = delete;

		JobGraph& operator=(JobGraph&& other);

		void clear();

		Size size()const;

		Bool empty()const;

		Iterator begin();

		Iterator end();

		ConstIterator begin()const;

		ConstIterator end()const;

	private:
		void erase(JobNode* node);

		template <typename ...Args>
		JobNode* emplace_back(Args&&... args)
		{
			nodes_.push_back(animate(std::forward<Args>(args)...));
			return nodes_.back();
		}
	};

	template<typename T>
	concept GraphLike = std::derived_from<T, JobGraph> || requires(T & t)
	{
		{ t.graph() }->std::convertible_to<JobGraph&>;
	};

	template<GraphLike T>
	JobGraph& RetrieveGraph(T& target)
	{
		if constexpr (requires{target.graph();})
		{
			return target.graph();
		}
		else
		{
			return static_cast<JobGraph&>(target);
		}
	}
}
#include <FoundationEngine/JobSystem/JobGraph.h>
#include <FoundationEngine/JobSystem/JobNode.h>

namespace SeedCore
{
	JobGraph::~JobGraph()
	{
		clear();
	}

	JobGraph::JobGraph(JobGraph&& other) :nodes_(std::move(other.nodes_))
	{
		/// No Code
	}

	JobGraph& JobGraph::operator=(JobGraph&& other)
	{
		clear();
		nodes_ = std::move(other.nodes_);
		return *this;
	}

	void JobGraph::clear()
	{
		for (JobNode* node : nodes_)
		{
			recycle(node);
		}
		nodes_.clear();
	}

	Size JobGraph::size()const
	{
		return nodes_.size();
	}

	Bool JobGraph::empty()const
	{
		return nodes_.empty();
	}

	JobGraph::Iterator JobGraph::begin()
	{
		return nodes_.begin();
	}

	JobGraph::Iterator JobGraph::end()
	{
		return nodes_.end();
	}

	JobGraph::ConstIterator JobGraph::begin()const
	{
		return nodes_.begin();
	}

	JobGraph::ConstIterator JobGraph::end()const
	{
		return nodes_.end();
	}

	void JobGraph::erase(JobNode* node)
	{
		SeedCore::erase_if(nodes_, [&](auto& p)
			{
				if (p == node)
				{
					recycle(p);
					return true;
				}
				return false;
			});
	}
}

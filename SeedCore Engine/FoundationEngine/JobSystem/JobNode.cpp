#include <FoundationEngine/JobSystem/JobNode.h>

namespace SeedCore
{
	JobNode::OwnedModule::OwnedModule(JobGraph& graph) :graph_(graph)
	{
		/// No Code
	}

	JobNode::AdoptedModule::AdoptedModule(JobGraph&& graph) :graph_(std::move(graph))
	{
		/// No Code
	}
}

namespace SeedCore
{
	Size JobNode::NumberSuccessors()const
	{
		return numberSuccessors_;
	}

	Size JobNode::NumberPredecessors()const
	{
		return edges_.size() - numberSuccessors_;
	}

	Size JobNode::NumberStrongDependencies()const
	{
		Size n = 0;
		for (Size index = numberSuccessors_;index < edges_.size();index++)
		{
			n += !edges_[index]->Conditioner();
		}
		return n;
	}

	Size JobNode::NumberWeakDependencies()const
	{
		Size n = 0;
		for (Size index = numberSuccessors_;index < edges_.size();index++)
		{
			n += edges_[index]->Conditioner();
		}
		return n;
	}

	const String& JobNode::Name()const
	{
		return name_;
	}

	Bool JobNode::ParentCancelled()const
	{
		return (topology_ && topology_->estate_.load(std::memory_order_relaxed) & (JobExceptionState::CANCELLED | JobExceptionState::EXCEPTION)) || (parent_ && (parent_->estate_.load(std::memory_order_relaxed) & (JobExceptionState::CANCELLED | JobExceptionState::EXCEPTION)));
	}

	Bool JobNode::Conditioner()const
	{
		return handle_.index() == JobNode::SINGLE_CONDITION || handle_.index() == JobNode::MULTI_CONDITION;
	}

	Bool JobNode::AcquireAll(HybridArray<JobNode*>& nodes)
	{
		auto& acquire = semaphores_->acquire_;
		for (Size index = 0;index < acquire.size();++index)
		{
			if (!acquire[index]->try_acquire_or_wait(this))
			{
				for (Size rollbackIndex = 1;rollbackIndex <= index;++rollbackIndex)
				{
					acquire[index - rollbackIndex]->release(nodes);
				}
				return false;
			}
		}
		return true;
	}

	void JobNode::ReleaseAll(HybridArray<JobNode*>& nodes)
	{
		auto& release = semaphores_->release_;
		for (Semaphore* semaphore : release)
		{
			semaphore->release(nodes);
		}
	}

	void JobNode::Precede(JobNode* node)
	{
		edges_.push_back(node);
		std::swap(edges_[numberSuccessors_++], edges_[edges_.size() - 1]);

		node->edges_.push_back(this);
	}

	void JobNode::SetUpJoinCounter()
	{
		for (Size index = numberSuccessors_;index < edges_.size();index++)
		{
			nstate_ += !edges_[index]->Conditioner();
		}

		joinCounter_.store(nstate_ & JobNodeState::STRONG_DEPENDENCIES_MASK, std::memory_order_relaxed);
	}

	void JobNode::RemoveSuccessors(JobNode* node)
	{
		auto sit = std::remove(edges_.begin(), edges_.begin() + numberSuccessors_, node);
		Size newNumberSuccessor = std::distance(edges_.begin(), sit);

		std::move(edges_.begin() + numberSuccessors_, edges_.end(), sit);
		edges_.resize(edges_.size() - (numberSuccessors_ - newNumberSuccessor));
		numberSuccessors_ = newNumberSuccessor;
	}

	void JobNode::RemovePredecessors(JobNode* node)
	{
		edges_.erase(std::remove(edges_.begin() + numberSuccessors_, edges_.end(), node), edges_.end());
	}
}

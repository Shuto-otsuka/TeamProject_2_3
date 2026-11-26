#include <FoundationEngine/JobSystem/JobTask.h>
#include <FoundationEngine/JobSystem/Semaphore.h>

namespace SeedCore
{
	JobTask::JobTask(const JobTask& rhs) :node_(rhs.node_)
	{
		/// No Code
	}

	JobTask::JobTask(JobNode* node) :node_(node)
	{
		/// No Code
	}

	JobTask& JobTask::operator=(const JobTask& rhs)
	{
		node_ = rhs.node_;
		return *this;
	}

	JobTask& JobTask::operator=(std::nullptr_t null)
	{
		node_ = null;
		return *this;
	}

	Bool JobTask::operator==(const JobTask& rhs)const
	{
		return node_ == rhs.node_;
	}

	Bool JobTask::operator!=(const JobTask& rhs)const
	{
		return node_ != rhs.node_;
	}

	const String& JobTask::Name()const
	{
		return node_->name_;
	}

	Size JobTask::NumberSuccessors()const
	{
		return node_->NumberSuccessors();
	}

	Size JobTask::NumberPredecessors()const
	{
		return node_->NumberPredecessors();
	}

	Size JobTask::NumberStrongDependencies()const
	{
		return node_->NumberStrongDependencies();
	}

	Size JobTask::NumberWeakDependencies()const
	{
		return node_->NumberWeakDependencies();
	}

	JobTask& JobTask::Name(const String& name)
	{
		node_->name_ = name;
		return *this;
	}

	JobTask& JobTask::Adopt(JobGraph&& graph)
	{
		node_->handle_.emplace<JobNode::AdoptedModule>(std::move(graph));
		return *this;
	}

	JobTask& JobTask::Data(void* data)
	{
		node_->data_ = data;
		return *this;
	}

	void JobTask::Reset()
	{
		node_ = nullptr;
	}

	void JobTask::ResetWork()
	{
		node_->handle_.emplace<std::monostate>();
	}

	Bool JobTask::Empty()const
	{
		return node_ == nullptr;
	}

	Bool JobTask::HasWork()const
	{
		return node_ ? node_->handle_.index() != 0 : false;
	}

	Size JobTask::HashValue()const
	{
		return std::hash<JobNode*>{}(node_);
	}

	JobTaskType JobTask::Type()const
	{
		switch (node_->handle_.index())
		{
		case JobNode::PLACEHOLDER:
			return JobTaskType::PLACEHOLDER;
		case JobNode::STATIC:
			return JobTaskType::STATIC;
		case JobNode::PREEMPTIVE_RUNTIME:
			FALLTHROUGH;
		case JobNode::NONPREEMPTIVE_RUNTIME:
			return JobTaskType::RUNTIME;
		case JobNode::SUBFLOW:
			return JobTaskType::SUBFLOW;
		case JobNode::SINGLE_CONDITION:
			FALLTHROUGH;
		case JobNode::MULTI_CONDITION:
			return JobTaskType::CONDITION;
		case JobNode::OWNED_MODULE:
			FALLTHROUGH;
		case JobNode::ADOPTED_MODULE:
			return JobTaskType::MODULE;
		default:
			return JobTaskType::UNDEFINED;
		}
	}

	JobTask& JobTask::Release(Semaphore& semaphore)
	{
		if (!node_->semaphores_)
		{
			node_->semaphores_ = std::make_unique<JobNode::Semaphores>();
		}
		node_->semaphores_->release_.push_back(&semaphore);
		return *this;
	}

	JobTask& JobTask::Acquire(Semaphore& semaphore)
	{
		if (!node_->semaphores_)
		{
			node_->semaphores_ = std::make_unique<JobNode::Semaphores>();
		}
		node_->semaphores_->acquire_.push_back(&semaphore);
		return *this;
	}

	void* JobTask::Data()const
	{
		return node_->data_;
	}

	std::exception_ptr JobTask::ExceptionPtr()const
	{
		return node_ ? node_->exceptionPtr_ : nullptr;
	}

	Bool JobTask::HasExceptionPtr()const
	{
		return node_ ? (node_->exceptionPtr_ != nullptr) : false;
	}
}

namespace SeedCore
{
	JobTaskView::JobTaskView(const JobNode& node) :node_(node)
	{
		/// No Code
	}

	const String& JobTaskView::Name()const
	{
		return node_.name_;
	}

	Size JobTaskView::NumberSuccessors()const
	{
		return node_.NumberSuccessors();
	}

	Size JobTaskView::NumberPredecessors()const
	{
		return node_.NumberPredecessors();
	}

	Size JobTaskView::NumberStrongDependencies()const
	{
		return node_.NumberStrongDependencies();
	}

	Size JobTaskView::NumberWeakDependencies()const
	{
		return node_.NumberWeakDependencies();
	}

	JobTaskType JobTaskView::Type()const
	{
		switch (node_.handle_.index())
		{
		case JobNode::PLACEHOLDER:
			return JobTaskType::PLACEHOLDER;
		case JobNode::STATIC:
			return JobTaskType::STATIC;
		case JobNode::PREEMPTIVE_RUNTIME:
			FALLTHROUGH;
		case JobNode::NONPREEMPTIVE_RUNTIME:
			return JobTaskType::RUNTIME;
		case JobNode::SUBFLOW:
			return JobTaskType::SUBFLOW;
		case JobNode::SINGLE_CONDITION:
			FALLTHROUGH;
		case JobNode::MULTI_CONDITION:
			return JobTaskType::CONDITION;
		case JobNode::OWNED_MODULE:
			FALLTHROUGH;
		case JobNode::ADOPTED_MODULE:
			return JobTaskType::MODULE;
		default:
			return JobTaskType::UNDEFINED;
		}
	}

	Size JobTaskView::HashValue()const
	{
		return std::hash<const JobNode*>{}(&node_);
	}
}

namespace SeedCore
{
	const Char* ToString(JobTaskType type)
	{
		switch (type)
		{
		case JobTaskType::PLACEHOLDER:
			return "Placeholder";
		case JobTaskType::STATIC:
			return "Static";
		case JobTaskType::RUNTIME:
			return "Runtime";
		case JobTaskType::SUBFLOW:
			return "Subflow";
		case JobTaskType::CONDITION:
			return "Condition";
		case JobTaskType::MODULE:
			return "Module";
		default:
			return "Undefined";
		}
	}
}

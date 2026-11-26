#include <FoundationEngine/World/ECS/System/SystemGraph.h>
#include <FoundationEngine/JobSystem/JobExecutor.h>
#include <FoundationEngine/JobSystem/JobTaskflow.h>

namespace SeedCore
{
	void SystemGraph::Add(const Bitset& readSignature, const Bitset& writeSignature, std::function<void()> task)
	{
		Entry entry;
		entry.readSignature_ = readSignature;
		entry.writeSignature_ = writeSignature;
		entry.task_ = std::move(task);
		entries_.push_back(std::move(entry));
	}

	void SystemGraph::Run(JobExecutor& executor)
	{
		if (entries_.empty())
		{
			return;
		}

		JobTaskflow taskflow;

		DynamicArray<JobTask> tasks;
		tasks.reserve(entries_.size());
		for (Size index = 0; index < entries_.size(); ++index)
		{
			tasks.push_back(taskflow.emplace([this, index]()
				{
					entries_[index].task_();
				}));
		}

		for (Size later = 1; later < entries_.size(); ++later)
		{
			for (Size earlier = 0; earlier < later; ++earlier)
			{
				if (Conflicts(entries_[earlier], entries_[later]))
				{
					tasks[earlier].Precede(tasks[later]);
				}
			}
		}

		executor.Run(taskflow).wait();
	}

	void SystemGraph::Clear()
	{
		entries_.clear();
	}

	Bool SystemGraph::Conflicts(const Entry& first, const Entry& second)
	{
		if ((first.writeSignature_ & second.writeSignature_).any())
		{
			return true;
		}

		if ((first.writeSignature_ & second.readSignature_).any())
		{
			return true;
		}

		if ((first.readSignature_ & second.writeSignature_).any())
		{
			return true;
		}

		return false;
	}
}

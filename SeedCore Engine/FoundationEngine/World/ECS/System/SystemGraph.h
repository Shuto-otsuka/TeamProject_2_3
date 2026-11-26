#pragma once
#include <FoundationEngine/Utility/Bitset.h>
#include <functional>

namespace SeedCore
{
	class JobExecutor;

	class SEEDCORE_API SystemGraph
	{
	public:
		SystemGraph() = default;

		~SystemGraph() = default;

		void Add(const Bitset& readSignature, const Bitset& writeSignature, std::function<void()> task);

		void Run(JobExecutor& executor);

		void Clear();

	private:
		struct Entry;

		static Bool Conflicts(const Entry& first, const Entry& second);

	private:
		struct Entry
		{
			Bitset readSignature_;

			Bitset writeSignature_;

			std::function<void()> task_;
		};

		DynamicArray<Entry> entries_;
	};
}

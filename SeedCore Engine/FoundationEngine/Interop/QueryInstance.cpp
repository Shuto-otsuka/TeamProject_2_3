#include <FoundationEngine/Interop/QueryInstance.h>

namespace SeedCore
{
	void QueryInstance::Add(const QueryDesc& query)
	{
		queries_.push_back(query);
	}

	std::span<const QueryDesc> QueryInstance::Queries()const
	{
		return queries_;
	}

	void QueryInstance::Clear()
	{
		queries_.clear();
	}
}

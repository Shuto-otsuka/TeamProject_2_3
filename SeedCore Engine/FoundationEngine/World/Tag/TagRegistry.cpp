#include <FoundationEngine/World/Tag/TagRegistry.h>

namespace SeedCore
{
	Size TagRegistry::GetOrCreate(String tag)
	{
		if (tag == String{})
		{
			return static_cast<Size>(-1);
		}

		auto it = indices_.find(tag);
		if (it != indices_.end())
		{
			return it->second;
		}

		Size index = names_.size();
		names_.push_back(tag);
		indices_[tag] = index;
		return index;
	}

	Size TagRegistry::Find(String tag)
	{
		auto it = indices_.find(tag);
		if (it != indices_.end())
		{
			return it->second;
		}
		return static_cast<Size>(-1);
	}

	void TagRegistry::Remove(String tag)
	{
		auto it = indices_.find(tag);
		if (it == indices_.end())
		{
			return;
		}

		names_[it->second] = String{};
		indices_.erase(it);
	}

	std::span<const String> TagRegistry::Names()
	{
		return std::span<const String>(names_.data(), names_.size());
	}
}

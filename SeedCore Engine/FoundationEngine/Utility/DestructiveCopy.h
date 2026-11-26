#pragma once

namespace SeedCore
{
	template<typename T>
	struct DestructiveCopy
	{
		DestructiveCopy(T&& rhs) :object_(std::move(rhs))
		{
			/// No Code
		}

		DestructiveCopy(const T& other) :object_(std::move(other.object_))
		{
			/// No Code
		}

		T& Get()
		{
			return object_;
		}

		mutable T object_;
	};
}
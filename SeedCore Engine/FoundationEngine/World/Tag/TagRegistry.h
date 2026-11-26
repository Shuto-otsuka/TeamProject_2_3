#pragma once
#include <FoundationEngine/World/Actor/ActorRegistry.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class SEEDCORE_API TagRegistry
	{
	public:
		static Size GetOrCreate(String tag);

		static Size Find(String tag);

		static void Remove(String tag);

		static std::span<const String> Names();

		template<typename Function>
		static void ForEach(const ActorRecord& record, Function function)
		{
			std::span<const String> names = Names();

			for (Uint64 bits = record.tags_;bits != 0;bits &= bits - 1)
			{
				Size index = static_cast<Size>(std::countr_zero(bits));
				if (names[index] != String{})
				{
					function(names[index]);
				}
			}

			if (record.extraTags_)
			{
				for (Size bit = record.extraTags_->find_first_set();bit != Bitset::InvalidIndex;bit = record.extraTags_->find_next_set(bit + 1))
				{
					Size index = bit + 64;
					if (names[index] != String{})
					{
						function(names[index]);
					}
				}
			}
		}

	private:
		static inline FlatMap<String, Size> indices_;

		static inline DynamicArray<String> names_;
	};
}

#include <FoundationEngine/World/ECS/Archetype/ArchetypeRegistry.h>
#include <FoundationEngine/World/ECS/Archetype/Archetype.h>

namespace SeedCore
{
	ArchetypeRegistry::RegistryCleaner::~RegistryCleaner()
	{
		for (Archetype* archetype : archetypes_)
		{
			delete archetype;
		}
	}
}

namespace SeedCore
{
	Archetype* ArchetypeRegistry::GetOrCreate(const DynamicArray<ComponentID>& layout)
	{
		DynamicArray<ComponentID> sortedLayout = layout;
		std::ranges::sort(sortedLayout);

		Size hash = 0;
		for (ComponentID id : sortedLayout)
		{
			hash ^= reinterpret_cast<uintptr_t>(id) + 0x9e3779b9 + (hash << 6) + (hash >> 2);
		}

		DynamicArray<Archetype*>& bucket = registry_[hash];
		for (Archetype* existing : bucket)
		{
			if (existing->Layout() == sortedLayout)
			{
				return existing;
			}
		}

		Archetype* newArchetype = new Archetype(sortedLayout, static_cast<Uint32>(archetypes_.size()));
		archetypes_.push_back(newArchetype);
		bucket.push_back(newArchetype);
		return newArchetype;
	}

	std::span<Archetype* const> ArchetypeRegistry::Archetypes()
	{
		return std::span<Archetype* const>(archetypes_.data(), archetypes_.size());
	}
}

#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class Archetype;

	class SEEDCORE_API ArchetypeRegistry
	{
	private:
		struct RegistryCleaner
		{
			~RegistryCleaner();
		};

	public:
		static Archetype* GetOrCreate(const DynamicArray<ComponentID>& layout);

		static std::span<Archetype* const> Archetypes();

	private:
		static inline DynamicArray<Archetype*> archetypes_;

		static inline FlatMap<Size, DynamicArray<Archetype*>> registry_;

		static inline RegistryCleaner cleaner_;
	};
}

#pragma once
#include <FoundationEngine/SeedScript.h>
#include <FoundationEngine/Resource/Blueprint/BlueprintComponent.h>

namespace SeedCore
{
	class World;

	struct SEEDCORE_API UnknownComponent
	{
		DynamicArray<BlueprintComponent> components_;

		static void Keep(Actor actor, const BlueprintComponent& component);

		static void Resolve(World& world);
	};
	REGISTER_COMPONENT(UnknownComponent, "Core", ComponentStorage::SparseSet);
}

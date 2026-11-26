#pragma once
#include <FoundationEngine/World/ECS/Entity/Entity.h>
#include <FoundationEngine/Utility/FlatMap.h>

namespace SeedCore
{
	class World;
	class CommandBuffer;

	class LifetimeSystem
	{
	public:
		void Execute(CommandBuffer& cmd, World& world, Float deltaTime);

		void Reset();

	private:
		FlatMap<EntityID, Float> remaining_;
	};
}

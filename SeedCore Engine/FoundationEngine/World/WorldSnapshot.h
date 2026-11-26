#pragma once
#include <FoundationEngine/World/ECS/Component/Component.h>
#include <FoundationEngine/World/ECS/Entity/Entity.h>

namespace SeedCore
{
	class World;

	class SEEDCORE_API WorldSnapshot
	{
	public:
		void Capture(World& world);

		void Restore(World& world);

		Bool HasData()const;

		void Clear();

	private:
		struct ComponentData
		{
			ComponentID id_;

			DynamicArray<Uint8> data_;
		};

		struct ActorData
		{
			EntityID entity_;

			DynamicArray<ComponentData> components_;

			DynamicArray<ComponentID> componentBaseIDs_;
		};

		DynamicArray<ActorData> actors_;

		Bool hasData_ = false;
	};
}

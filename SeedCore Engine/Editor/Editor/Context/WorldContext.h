#pragma once

namespace SeedCore
{
	class GameTimer;
	class ResourceCache;
	class SceneSystem;
	class SystemScheduler;
	class World;

	struct WorldContext
	{
		World* world_ = nullptr;

		ResourceCache* resource_ = nullptr;

		SceneSystem* sceneSystem_ = nullptr;

		GameTimer* timer_ = nullptr;

		SystemScheduler* system_ = nullptr;
	};
}

#include <GraphicsEngine/System/ReflectionSystem.h>
#include <GraphicsEngine/Illumination/Reflection/Reflection.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void ReflectionSystem::Execute(World& world)
	{
		constantBuffer_ = RaytracingReflectionConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<Reflection>> query(world);
		query.ForEach([&](const Active& active, const Reflection& reflection)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = reflection.enable_ && reflection.technique_ == ReflectionTechnique::Raytracing;
				constantBuffer_.rayTMax_ = reflection.raytracing_.rayTMax_;
				constantBuffer_.normalBias_ = reflection.raytracing_.normalBias_;
				constantBuffer_.strength_ = reflection.strength_;
				constantBuffer_.rayCount_ = reflection.raytracing_.rayCount_;
				constantBuffer_.bounceCount_ = reflection.raytracing_.bounceCount_;
				constantBuffer_.bounceRoughnessThreshold_ = reflection.raytracing_.bounceRoughnessThreshold_;
			});
	}

	const RaytracingReflectionConstantBuffer& ReflectionSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool ReflectionSystem::Enabled()const
	{
		return enabled_;
	}
}

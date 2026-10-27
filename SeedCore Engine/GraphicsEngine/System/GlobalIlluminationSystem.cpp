#include <GraphicsEngine/System/GlobalIlluminationSystem.h>
#include <GraphicsEngine/Illumination/GlobalIllumination/GlobalIllumination.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void GlobalIlluminationSystem::Execute(World& world)
	{
		constantBuffer_ = RaytracingGlobalIlluminationConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<GlobalIllumination>> query(world);
		query.ForEach([&](const Active& active, const GlobalIllumination& globalIllumination)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = globalIllumination.enable_ && globalIllumination.technique_ == GlobalIlluminationTechnique::Raytracing;
				constantBuffer_.rayTMax_ = globalIllumination.raytracing_.rayTMax_;
				constantBuffer_.normalBias_ = globalIllumination.raytracing_.normalBias_;
				constantBuffer_.intensity_ = globalIllumination.intensity_;
				constantBuffer_.rayCount_ = globalIllumination.raytracing_.rayCount_;
				constantBuffer_.bounceCount_ = globalIllumination.raytracing_.bounceCount_;
			});
	}

	const RaytracingGlobalIlluminationConstantBuffer& GlobalIlluminationSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool GlobalIlluminationSystem::Enabled()const
	{
		return enabled_;
	}
}

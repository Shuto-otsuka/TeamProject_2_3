#include <GraphicsEngine/System/AmbientOcclusionSystem.h>
#include <GraphicsEngine/Illumination/AmbientOcclusion/AmbientOcclusion.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void AmbientOcclusionSystem::Execute(World& world)
	{
		constantBuffer_ = RaytracingAmbientOcclusionConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<AmbientOcclusion>> query(world);
		query.ForEach([&](const Active& active, const AmbientOcclusion& ambientOcclusion)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = ambientOcclusion.enable_ && ambientOcclusion.technique_ == AmbientOcclusionTechnique::Raytracing;
				constantBuffer_.rayLength_ = ambientOcclusion.radius_;
				constantBuffer_.normalBias_ = ambientOcclusion.raytracing_.normalBias_;
				constantBuffer_.power_ = ambientOcclusion.power_;
				constantBuffer_.rayCount_ = ambientOcclusion.raytracing_.rayCount_;
			});
	}

	const RaytracingAmbientOcclusionConstantBuffer& AmbientOcclusionSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool AmbientOcclusionSystem::Enabled()const
	{
		return enabled_;
	}
}

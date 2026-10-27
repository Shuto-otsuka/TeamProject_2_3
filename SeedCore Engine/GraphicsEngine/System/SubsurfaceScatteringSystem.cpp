#include <GraphicsEngine/System/SubsurfaceScatteringSystem.h>
#include <GraphicsEngine/Illumination/SubsurfaceScattering/SubsurfaceScattering.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void SubsurfaceScatteringSystem::Execute(World& world)
	{
		constantBuffer_ = SubsurfaceScatteringRayConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<SubsurfaceScattering>> query(world);
		query.ForEach([&](const Active& active, const SubsurfaceScattering& subsurfaceScattering)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = subsurfaceScattering.enable_ && subsurfaceScattering.technique_ == SubsurfaceScatteringTechnique::Raytracing;
				constantBuffer_.scatterDistance_ = subsurfaceScattering.scatteringDistance_;
				constantBuffer_.thicknessBias_ = subsurfaceScattering.raytracing_.thicknessBias_;
				constantBuffer_.rayTMax_ = subsurfaceScattering.raytracing_.rayTMax_;
				constantBuffer_.strength_ = subsurfaceScattering.strength_;
				constantBuffer_.subsurfaceColor_[0] = subsurfaceScattering.subsurfaceColor_.x;
				constantBuffer_.subsurfaceColor_[1] = subsurfaceScattering.subsurfaceColor_.y;
				constantBuffer_.subsurfaceColor_[2] = subsurfaceScattering.subsurfaceColor_.z;
			});
	}

	const SubsurfaceScatteringRayConstantBuffer& SubsurfaceScatteringSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool SubsurfaceScatteringSystem::Enabled()const
	{
		return enabled_;
	}
}

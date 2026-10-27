#include <GraphicsEngine/System/RefractionSystem.h>
#include <GraphicsEngine/Illumination/Refraction/Refraction.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void RefractionSystem::Execute(World& world)
	{
		constantBuffer_ = RefractionRayConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<Refraction>> query(world);
		query.ForEach([&](const Active& active, const Refraction& refraction)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = refraction.enable_ && refraction.technique_ == RefractionTechnique::Raytracing;
				constantBuffer_.rayTMax_ = refraction.raytracing_.rayTMax_;
				constantBuffer_.normalBias_ = refraction.raytracing_.normalBias_;
				constantBuffer_.strength_ = refraction.strength_;
			});
	}

	const RefractionRayConstantBuffer& RefractionSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool RefractionSystem::Enabled()const
	{
		return enabled_;
	}
}

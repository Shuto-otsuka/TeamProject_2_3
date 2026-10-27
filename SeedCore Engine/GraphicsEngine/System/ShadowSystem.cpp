#include <GraphicsEngine/System/ShadowSystem.h>
#include <GraphicsEngine/Illumination/Shadow/Shadow.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void ShadowSystem::Execute(World& world)
	{
		constantBuffer_ = RaytracingShadowConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<Shadow>> query(world);
		query.ForEach([&](const Active& active, const Shadow& shadow)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = shadow.enable_ && shadow.technique_ == ShadowTechnique::Raytracing;
				constantBuffer_.rayTMax_ = shadow.raytracing_.rayTMax_;
				constantBuffer_.normalBias_ = shadow.raytracing_.normalBias_;
				constantBuffer_.shadowStrength_ = shadow.raytracing_.shadowStrength_;
				constantBuffer_.sunAngularRadius_ = shadow.raytracing_.sunAngularRadius_;
				constantBuffer_.punctualLightRadius_ = shadow.raytracing_.punctualLightRadius_;
				constantBuffer_.rayCount_ = shadow.raytracing_.rayCount_;
			});
	}

	const RaytracingShadowConstantBuffer& ShadowSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool ShadowSystem::Enabled()const
	{
		return enabled_;
	}
}

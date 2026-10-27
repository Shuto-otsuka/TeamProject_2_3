#include <GraphicsEngine/System/VolumetricStarSystem.h>
#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStar.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void VolumetricStarSystem::Execute(World& world)
	{
		constantBuffer_ = VolumetricStarRayConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<VolumetricStar>> query(world);
		query.ForEach([&](const Active& active, const VolumetricStar& star)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = star.enable_;
				constantBuffer_.cellSize_ = star.cellSize_;
				constantBuffer_.density_ = star.density_;
				constantBuffer_.brightness_ = star.brightness_;
				constantBuffer_.twinkleSpeed_ = star.twinkleSpeed_;
				constantBuffer_.color_[0] = star.color_.x;
				constantBuffer_.color_[1] = star.color_.y;
				constantBuffer_.color_[2] = star.color_.z;
				constantBuffer_.sizeMin_ = star.sizeMin_;
				constantBuffer_.sizeMax_ = star.sizeMax_;
				constantBuffer_.shootingStarChancePerSecond_ = star.shootingStarChancePerSecond_;
				constantBuffer_.shootingStarBrightness_ = star.shootingStarBrightness_;
				constantBuffer_.shootingStarWidth_ = star.shootingStarWidth_;
				constantBuffer_.glowIntensity_ = star.glowIntensity_;
				constantBuffer_.glowFalloff_ = star.glowFalloff_;
			});
	}

	const VolumetricStarRayConstantBuffer& VolumetricStarSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool VolumetricStarSystem::Enabled()const
	{
		return enabled_;
	}
}

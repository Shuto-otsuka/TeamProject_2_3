#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStarFeature.h>
#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStarStage.h>
#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStar.h>
#include <GraphicsEngine/System/CelestialSystem.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(VolumetricStarFeature, TraceStage);

	void VolumetricStarFeature::Create(RenderDevice& device)
	{
		tracePipeline_.Create(device);
	}

	void VolumetricStarFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		constants_ = VolumetricStarConstantBuffer{};
		Bool enabled = false;
		Bool found = false;
		Query<Read<Active>, Read<VolumetricStar>> query(*scene.world_);
		query.ForEach([&](const Active& active, const VolumetricStar& star)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled = star.enable_;
				constants_.cellSize_ = star.cellSize_;
				constants_.density_ = star.density_;
				constants_.brightness_ = star.brightness_;
				constants_.twinkleSpeed_ = star.twinkleSpeed_;
				constants_.color_ = Vector3(star.color_.r, star.color_.g, star.color_.b);
				constants_.sizeMin_ = star.sizeMin_;
				constants_.sizeMax_ = star.sizeMax_;
				constants_.shootingStarChancePerSecond_ = star.shootingStarChancePerSecond_;
				constants_.shootingStarBrightness_ = star.shootingStarBrightness_;
				constants_.shootingStarWidth_ = star.shootingStarWidth_;
				constants_.glowIntensity_ = star.glowIntensity_;
				constants_.glowFalloff_ = star.glowFalloff_;
			});

		const CelestialResult* celestial = scene.Find<CelestialResult>();
		Float nightFactor = celestial ? celestial->nightFactor_ : 0.0f;
		Float deltaTime = scene.views_.empty() ? 0.0f : scene.views_[0].scene_.deltaTime_;

		for (Size slot = 0;slot < meteors_.size();++slot)
		{
			if (meteors_[slot].progress_ <= 0.0f)
			{
				continue;
			}
			meteors_[slot].progress_ += deltaTime / Max(durations_[slot], 0.01f);
			if (meteors_[slot].progress_ >= 1.0f)
			{
				meteors_[slot].progress_ = 0.0f;
			}
		}

		if (enabled && nightFactor > 0.0f)
		{
			for (Size slot = 0;slot < meteors_.size();++slot)
			{
				if (meteors_[slot].progress_ > 0.0f)
				{
					continue;
				}

				Float chance = constants_.shootingStarChancePerSecond_ * nightFactor * deltaTime;
				if (std::uniform_real_distribution<Float>(0.0f, 1.0f)(randomEngine_) < chance)
				{
					Vector3 start = Vector3(std::uniform_real_distribution<Float>(-1.0f, 1.0f)(randomEngine_), std::uniform_real_distribution<Float>(0.2f, 1.0f)(randomEngine_), std::uniform_real_distribution<Float>(-1.0f, 1.0f)(randomEngine_));
					start.Normalize();

					Vector3 randomVector = Vector3(std::uniform_real_distribution<Float>(-1.0f, 1.0f)(randomEngine_), std::uniform_real_distribution<Float>(-1.0f, 1.0f)(randomEngine_), std::uniform_real_distribution<Float>(-1.0f, 1.0f)(randomEngine_));
					Vector3 axis = start.Cross(randomVector);
					if (axis.LengthSquared() < 0.0001f)
					{
						axis = Vector3(1.0f, 0.0f, 0.0f);
					}
					axis.Normalize();

					Float travelAngle = DirectX::XMConvertToRadians(std::uniform_real_distribution<Float>(50.0f, 110.0f)(randomEngine_));
					Vector3 end = Vector3::Transform(start, Matrix::CreateFromAxisAngle(axis, travelAngle));
					if (end.y < 0.1f)
					{
						end.y = 0.1f;
					}
					end.Normalize();

					meteors_[slot].startDirection_ = start;
					meteors_[slot].endDirection_ = end;
					meteors_[slot].progress_ = 0.0001f;
					meteors_[slot].brightness_ = std::uniform_real_distribution<Float>(0.6f, 1.0f)(randomEngine_);
					durations_[slot] = std::uniform_real_distribution<Float>(1.0f, 2.0f)(randomEngine_);
				}
				break;
			}
		}

		constants_.enabled_ = enabled ? 1 : 0;
		constants_.activeShootingStars_ = meteors_;
		tracing_ = enabled;
	}

	void VolumetricStarFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		upload.Write(Constants, RenderViewKind::Game, std::span<const VolumetricStarConstantBuffer>(&constants_, 1));
	}
}

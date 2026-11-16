#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudFeature.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudStage.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudScapes.h>
#include <GraphicsEngine/System/CelestialSystem.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(VolumetricCloudFeature, BuildStage, TraceStage);

	void VolumetricCloudFeature::Create(RenderDevice& device)
	{
		shapePipeline_.Create(device);
		detailPipeline_.Create(device);
		tracePipeline_.Create(device);
		resolvePipeline_.Create(device);
	}

	void VolumetricCloudFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		constants_ = VolumetricCloudConstantBuffer{};
		Bool traced = tracing_;
		Bool enabled = false;
		Bool found = false;
		Query<Read<Active>, Read<VolumetricCloudScapes>> query(*scene.world_);
		query.ForEach([&](const Active& active, const VolumetricCloudScapes& cloud)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled = cloud.enable_;
				Float horizontalFrequency = 0.25f / cloud.shape_.size_;
				Float windAngle = ToRadians(cloud.windDirection_);
				constants_.cloudBottom_ = cloud.cloudBottom_;
				constants_.cloudTop_ = cloud.cloudBottom_ + cloud.cloudThickness_;
				constants_.coverage_ = cloud.coverage_;
				constants_.densityScale_ = 0.002f * Pow(100.0f, cloud.lighting_.density_);
				constants_.cloudAlbedo_ = Vector3(cloud.lighting_.albedo_.r, cloud.lighting_.albedo_.g, cloud.lighting_.albedo_.b);
				constants_.scatteringG_ = cloud.lighting_.forwardScattering_;
				constants_.noiseScale_ = horizontalFrequency;
				constants_.windX_ = Cos(windAngle) * cloud.windSpeed_ * horizontalFrequency;
				constants_.windZ_ = Sin(windAngle) * cloud.windSpeed_ * horizontalFrequency;
				constants_.cloudType_ = cloud.cloudType_;
				constants_.rain_ = cloud.rain_;
				constants_.detailScale_ = cloud.shape_.detailScale_;
				constants_.skyZenithColor_ = Vector3(cloud.sky_.zenithColor_.r, cloud.sky_.zenithColor_.g, cloud.sky_.zenithColor_.b);
				constants_.sunSize_ = cloud.sky_.sunSize_;
				constants_.skyHorizonColor_ = Vector3(cloud.sky_.horizonColor_.r, cloud.sky_.horizonColor_.g, cloud.sky_.horizonColor_.b);
				constants_.skyBrightness_ = cloud.sky_.brightness_;
				constants_.groundColor_ = Vector3(cloud.sky_.groundColor_.r, cloud.sky_.groundColor_.g, cloud.sky_.groundColor_.b);
				constants_.planetRadius_ = cloud.advanced_.planetRadius_;
				switch (cloud.quality_)
				{
				case VolumetricCloudQuality::Low:
					constants_.stepCount_ = 32;
					constants_.lightStepCount_ = 4;
					constants_.maxMarchDistance_ = 60000.0f;
					constants_.lodDistance_ = 15000.0f;
					break;

				case VolumetricCloudQuality::Medium:
					constants_.stepCount_ = 48;
					constants_.lightStepCount_ = 5;
					constants_.maxMarchDistance_ = 90000.0f;
					constants_.lodDistance_ = 20000.0f;
					break;

				case VolumetricCloudQuality::High:
					constants_.stepCount_ = 64;
					constants_.lightStepCount_ = 6;
					constants_.maxMarchDistance_ = 120000.0f;
					constants_.lodDistance_ = 25000.0f;
					break;

				case VolumetricCloudQuality::Ultra:
					constants_.stepCount_ = 128;
					constants_.lightStepCount_ = 8;
					constants_.maxMarchDistance_ = 200000.0f;
					constants_.lodDistance_ = 40000.0f;
					break;
				}
				constants_.aerialDensity_ = cloud.advanced_.aerialDensity_;
				constants_.weatherScale_ = 0.25f / (cloud.shape_.weatherSize_ * 1000.0f);
				constants_.weatherAmount_ = cloud.shape_.weatherAmount_;
				constants_.detailStrength_ = cloud.shape_.detailStrength_;
				constants_.detailWindSpeed_ = cloud.shape_.detailWindSpeed_;
				constants_.phaseBackward_ = cloud.advanced_.phaseBackward_;
				constants_.phaseBlend_ = cloud.advanced_.phaseBlend_;
				constants_.powderStrength_ = cloud.lighting_.powderStrength_;
				constants_.ambientStrength_ = cloud.lighting_.ambientStrength_;
				constants_.multiScatterOctaves_ = cloud.advanced_.multiScatterOctaves_;
				constants_.multiScatterAttenuation_ = cloud.advanced_.multiScatterAttenuation_;
				constants_.multiScatterContribution_ = cloud.advanced_.multiScatterContribution_;
				constants_.multiScatterEccentricity_ = cloud.advanced_.multiScatterEccentricity_;
				constants_.iblIntensity_ = cloud.advanced_.iblIntensity_;
				constants_.lightDensity_ = 1.0f - 0.9f * cloud.lighting_.lightPenetration_;
				constants_.sunStrength_ = cloud.lighting_.sunStrength_;
			});

		const CelestialResult* celestial = scene.Find<CelestialResult>();
		if (celestial && celestial->sunEnabled_)
		{
			constants_.skyZenithColor_ = Vector3(celestial->skyZenithColor_[0], celestial->skyZenithColor_[1], celestial->skyZenithColor_[2]);
			constants_.skyHorizonColor_ = Vector3(celestial->skyHorizonColor_[0], celestial->skyHorizonColor_[1], celestial->skyHorizonColor_[2]);
			constants_.sunSize_ = celestial->sunAngularRadius_;
		}

		constants_.frameIndex_ = frameIndex_;
		constants_.proceduralSkyEnabled_ = enabled ? 1 : 0;
		++frameIndex_;

		baking_ = !baked_;
		baked_ = true;
		tracing_ = enabled;
		historyValid_ = traced && tracing_;
	}

	void VolumetricCloudFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		upload.Write(Constants, RenderViewKind::Game, std::span<const VolumetricCloudConstantBuffer>(&constants_, 1));
	}
}

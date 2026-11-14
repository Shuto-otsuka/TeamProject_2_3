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
	}

	void VolumetricCloudFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		constants_ = VolumetricCloudConstantBuffer{};
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
				constants_.cloudBottom_ = cloud.cloudBottom_;
				constants_.cloudTop_ = cloud.cloudTop_;
				constants_.coverage_ = cloud.coverage_;
				constants_.densityScale_ = cloud.densityScale_;
				constants_.cloudAlbedo_ = Vector3(cloud.cloudAlbedo_.r, cloud.cloudAlbedo_.g, cloud.cloudAlbedo_.b);
				constants_.scatteringG_ = cloud.scatteringG_;
				constants_.stepCount_ = cloud.stepCount_;
				constants_.lightStepCount_ = cloud.lightStepCount_;
				constants_.noiseScale_ = cloud.noiseScale_;
				constants_.windSpeed_ = cloud.windSpeed_;
				constants_.cloudType_ = cloud.cloudType_;
				constants_.rain_ = cloud.rain_;
				constants_.detailScale_ = cloud.detailScale_;
				constants_.skyZenithColor_ = Vector3(cloud.skyZenithColor_.r, cloud.skyZenithColor_.g, cloud.skyZenithColor_.b);
				constants_.sunSize_ = cloud.sunSize_;
				constants_.skyHorizonColor_ = Vector3(cloud.skyHorizonColor_.r, cloud.skyHorizonColor_.g, cloud.skyHorizonColor_.b);
				constants_.skyBrightness_ = cloud.skyBrightness_;
				constants_.groundColor_ = Vector3(cloud.groundColor_.r, cloud.groundColor_.g, cloud.groundColor_.b);
				constants_.planetRadius_ = cloud.planetRadius_;
				constants_.maxMarchDistance_ = cloud.maxMarchDistance_;
				constants_.lodDistance_ = cloud.lodDistance_;
				constants_.aerialDensity_ = cloud.aerialDensity_;
				constants_.weatherScale_ = cloud.weatherScale_;
				constants_.weatherAmount_ = cloud.weatherAmount_;
				constants_.detailStrength_ = cloud.detailStrength_;
				constants_.detailWindSpeed_ = cloud.detailWindSpeed_;
				constants_.phaseBackward_ = cloud.phaseBackward_;
				constants_.phaseBlend_ = cloud.phaseBlend_;
				constants_.powderStrength_ = cloud.powderStrength_;
				constants_.ambientStrength_ = cloud.ambientStrength_;
				constants_.multiScatterOctaves_ = cloud.multiScatterOctaves_;
				constants_.multiScatterAttenuation_ = cloud.multiScatterAttenuation_;
				constants_.multiScatterContribution_ = cloud.multiScatterContribution_;
				constants_.multiScatterEccentricity_ = cloud.multiScatterEccentricity_;
				constants_.temporalJitter_ = cloud.temporalJitter_;
				constants_.iblIntensity_ = cloud.iblIntensity_;
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
	}

	void VolumetricCloudFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		upload.Write(Constants, RenderViewKind::Game, std::span<const VolumetricCloudConstantBuffer>(&constants_, 1));
	}
}

#include <GraphicsEngine/System/VolumetricCloudScapesSystem.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudScapes.h>
#include <GraphicsEngine/System/CelestialSystem.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void VolumetricCloudScapesSystem::Execute(World& world, const CelestialSystem& celestial)
	{
		constantBuffer_ = VolumetricCloudScapesRayConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<VolumetricCloudScapes>> query(world);
		query.ForEach([&](const Active& active, const VolumetricCloudScapes& cloud)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = cloud.enable_;
				constantBuffer_.cloudBottom_ = cloud.cloudBottom_;
				constantBuffer_.cloudTop_ = cloud.cloudTop_;
				constantBuffer_.coverage_ = cloud.coverage_;
				constantBuffer_.densityScale_ = cloud.densityScale_;
				constantBuffer_.cloudAlbedo_[0] = cloud.cloudAlbedo_.x;
				constantBuffer_.cloudAlbedo_[1] = cloud.cloudAlbedo_.y;
				constantBuffer_.cloudAlbedo_[2] = cloud.cloudAlbedo_.z;
				constantBuffer_.scatteringG_ = cloud.scatteringG_;
				constantBuffer_.stepCount_ = cloud.stepCount_;
				constantBuffer_.lightStepCount_ = cloud.lightStepCount_;
				constantBuffer_.noiseScale_ = cloud.noiseScale_;
				constantBuffer_.windSpeed_ = cloud.windSpeed_;
				constantBuffer_.cloudType_ = cloud.cloudType_;
				constantBuffer_.rain_ = cloud.rain_;
				constantBuffer_.detailScale_ = cloud.detailScale_;
				constantBuffer_.skyZenithColor_[0] = cloud.skyZenithColor_.x;
				constantBuffer_.skyZenithColor_[1] = cloud.skyZenithColor_.y;
				constantBuffer_.skyZenithColor_[2] = cloud.skyZenithColor_.z;
				constantBuffer_.sunSize_ = cloud.sunSize_;
				constantBuffer_.skyHorizonColor_[0] = cloud.skyHorizonColor_.x;
				constantBuffer_.skyHorizonColor_[1] = cloud.skyHorizonColor_.y;
				constantBuffer_.skyHorizonColor_[2] = cloud.skyHorizonColor_.z;
				constantBuffer_.skyBrightness_ = cloud.skyBrightness_;
				constantBuffer_.groundColor_[0] = cloud.groundColor_.x;
				constantBuffer_.groundColor_[1] = cloud.groundColor_.y;
				constantBuffer_.groundColor_[2] = cloud.groundColor_.z;
				constantBuffer_.planetRadius_ = cloud.planetRadius_;
				constantBuffer_.maxMarchDistance_ = cloud.maxMarchDistance_;
				constantBuffer_.lodDistance_ = cloud.lodDistance_;
				constantBuffer_.aerialDensity_ = cloud.aerialDensity_;
				constantBuffer_.weatherScale_ = cloud.weatherScale_;
				constantBuffer_.weatherAmount_ = cloud.weatherAmount_;
				constantBuffer_.detailStrength_ = cloud.detailStrength_;
				constantBuffer_.detailWindSpeed_ = cloud.detailWindSpeed_;
				constantBuffer_.phaseBackward_ = cloud.phaseBackward_;
				constantBuffer_.phaseBlend_ = cloud.phaseBlend_;
				constantBuffer_.powderStrength_ = cloud.powderStrength_;
				constantBuffer_.ambientStrength_ = cloud.ambientStrength_;
				constantBuffer_.multiScatterOctaves_ = cloud.multiScatterOctaves_;
				constantBuffer_.multiScatterAttenuation_ = cloud.multiScatterAttenuation_;
				constantBuffer_.multiScatterContribution_ = cloud.multiScatterContribution_;
				constantBuffer_.multiScatterEccentricity_ = cloud.multiScatterEccentricity_;
				constantBuffer_.temporalJitter_ = cloud.temporalJitter_;
				constantBuffer_.iblIntensity_ = cloud.iblIntensity_;
			});

		if (!celestial.SunEnabled())
		{
			return;
		}

		for (Int index = 0; index < 3; index++)
		{
			constantBuffer_.skyZenithColor_[index] = celestial.Result().skyZenithColor_[index];
			constantBuffer_.skyHorizonColor_[index] = celestial.Result().skyHorizonColor_[index];
		}
		constantBuffer_.sunSize_ = celestial.SunAngularRadius();
	}

	const VolumetricCloudScapesRayConstantBuffer& VolumetricCloudScapesSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool VolumetricCloudScapesSystem::Enabled()const
	{
		return enabled_;
	}
}

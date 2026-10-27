#include <GraphicsEngine/System/VolumetricLightSystem.h>
#include <GraphicsEngine/Illumination/VolumetricLight/VolumetricLight.h>
#include <FoundationEngine/World/World.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	void VolumetricLightSystem::Execute(World& world)
	{
		constantBuffer_ = VolumetricLightRayConstantBuffer();
		enabled_ = false;

		Bool found = false;
		Query<Read<Active>, Read<VolumetricLight>> query(world);
		query.ForEach([&](const Active& active, const VolumetricLight& volumetricLight)
			{
				if (!active.active_ || found)
				{
					return;
				}
				found = true;

				enabled_ = volumetricLight.enable_ && volumetricLight.technique_ == VolumetricLightTechnique::Raytracing;
				constantBuffer_.density_ = volumetricLight.density_;
				constantBuffer_.absorption_ = volumetricLight.absorption_;
				constantBuffer_.heightFalloff_ = volumetricLight.heightFalloff_;
				constantBuffer_.heightReference_ = volumetricLight.heightReference_;
				constantBuffer_.fogAlbedo_[0] = volumetricLight.fogAlbedo_.x;
				constantBuffer_.fogAlbedo_[1] = volumetricLight.fogAlbedo_.y;
				constantBuffer_.fogAlbedo_[2] = volumetricLight.fogAlbedo_.z;
				constantBuffer_.scatteringG_ = volumetricLight.raytracing_.scatteringG_;
				constantBuffer_.rayTMax_ = volumetricLight.raytracing_.rayTMax_;
				constantBuffer_.godrayStrength_ = volumetricLight.raytracing_.godrayStrength_;
				constantBuffer_.cloudShadowEnabled_ = volumetricLight.raytracing_.cloudShadowEnabled_ ? 1 : 0;
			});
	}

	const VolumetricLightRayConstantBuffer& VolumetricLightSystem::ConstantBuffer()const
	{
		return constantBuffer_;
	}

	Bool VolumetricLightSystem::Enabled()const
	{
		return enabled_;
	}
}

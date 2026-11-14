#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightFeature.h>
#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLightStage.h>
#include <GraphicsEngine/Illumination/VolumetricLight/VolumetricLight.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/Math/Halton.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(RaytracingVolumetricLightFeature, BuildStage, TraceStage, ResolveStage);

	void RaytracingVolumetricLightFeature::Create(RenderDevice& device)
	{
		injectPipeline_.Create(device);
		scatterPipeline_.Create(device);
		integratePipeline_.Create(device);
	}

	void RaytracingVolumetricLightFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		Bool traced = tracing_;
		tracing_ = false;
		Bool found = false;
		Query<Read<Active>, Read<VolumetricLight>> query(*scene.world_);
		query.ForEach([&](const Active& active, const VolumetricLight& volumetricLight)
			{
				if (found || !active.active_)
				{
					return;
				}
				found = true;
				tracing_ = volumetricLight.enable_ && volumetricLight.technique_ == VolumetricLightTechnique::Raytracing;
				constants_.density_ = volumetricLight.density_;
				constants_.absorption_ = volumetricLight.absorption_;
				constants_.heightFalloff_ = volumetricLight.heightFalloff_;
				constants_.heightReference_ = volumetricLight.heightReference_;
				constants_.fogAlbedo_ = { volumetricLight.fogAlbedo_.r, volumetricLight.fogAlbedo_.g, volumetricLight.fogAlbedo_.b };
				constants_.scatteringG_ = volumetricLight.raytracing_.scatteringG_;
				constants_.rayTMax_ = volumetricLight.raytracing_.rayTMax_;
				constants_.godrayStrength_ = volumetricLight.raytracing_.godrayStrength_;
			});
		constants_.historyValid_ = traced && tracing_ ? 1u : 0u;
		++constants_.frame_;
		Uint32 sampleIndex = constants_.frame_ % 16 + 1;
		constants_.jitter_ = { HaltonRadicalInverse(sampleIndex, 2) - 0.5f, HaltonRadicalInverse(sampleIndex, 3) - 0.5f, HaltonRadicalInverse(sampleIndex, 5) - 0.5f };
	}

	void RaytracingVolumetricLightFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		if (tracing_)
		{
			upload.Write(Constants, RenderViewKind::Game, std::span<const RaytracingVolumetricLightConstantBuffer>(&constants_, 1));
		}
	}
}

#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Sky/SkyStage.h>
#include <GraphicsEngine/Sky/Skymap.h>
#include <GraphicsEngine/Sky/SkymapResource.h>
#include <GraphicsEngine/Light/SkyLight.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(SkyFeature, GenerateEquirectStage, DiffuseIrradianceStage, SpecularPrefilterStage, BrdfLutStage, ResolveStage);

	void SkyFeature::Create(RenderDevice& device)
	{
		generateEquirectPipeline_.Create(device);
		diffuseIrradiancePipeline_.Create(device);
		specularPrefilterPipeline_.Create(device);
		brdfLutPipeline_.Create(device);
		resolvePipeline_.Create(device);
	}

	void SkyFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		Uint32 sourceIndex = SC_INVALID;
		SkymapResource& skymapResource = *scene.resource_->skymapResource_;
		Query<Read<Active>, Read<SkyLight>> query(*scene.world_);
		query.ForEach([&](const Active& active, const SkyLight& skyLight)
			{
				if (sourceIndex != SC_INVALID || !active.active_ || !skyLight.useSkymap_)
				{
					return;
				}
				Handle<Skymap> handle = skymapResource.GetHandle(skyLight.skymapID_);
				if (handle.empty())
				{
					return;
				}
				Skymap* skymap = skymapResource.Resolve(handle);
				if (!skymap || !skymap->Valid())
				{
					return;
				}
				sourceIndex = skymap->ShaderResourceViewIndex();
			});

		generate_ = !generated_ || sourceIndex != sourceIndex_;
		generated_ = true;
		sourceIndex_ = sourceIndex;

		brdfLut_ = !brdfLutGenerated_;
		brdfLutGenerated_ = true;

		resolving_ = sourceIndex_ != SC_INVALID;
	}
}

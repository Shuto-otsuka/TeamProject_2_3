#include <GraphicsEngine/Sky/SkyFeature.h>
#include <GraphicsEngine/Sky/SkyStage.h>
#include <GraphicsEngine/Sky/Skymap.h>
#include <GraphicsEngine/Sky/SkymapResource.h>
#include <GraphicsEngine/Light/SkyLight.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudScapes.h>
#include <GraphicsEngine/System/CelestialSystem.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/Resource/GraphicsResourceSystem.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(SkyFeature, BuildStage, ResolveStage);

	void SkyFeature::Create(RenderDevice& device)
	{
		equirectPipeline_.Create(device);
		proceduralPipeline_.Create(device);
		irradiancePipeline_.Create(device);
		prefilterPipeline_.Create(device);
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

		equirect_ = !generated_ || sourceIndex != sourceIndex_;
		generated_ = true;
		sourceIndex_ = sourceIndex;

		Uint32 hash = 2166136261u;
		Bool cloudEnabled = false;
		Bool cloudFound = false;
		Query<Read<Active>, Read<VolumetricCloudScapes>> cloudQuery(*scene.world_);
		cloudQuery.ForEach([&](const Active& active, const VolumetricCloudScapes& cloud)
			{
				if (!active.active_ || cloudFound)
				{
					return;
				}
				cloudFound = true;
				cloudEnabled = cloud.enable_;
				const Byte* first = reinterpret_cast<const Byte*>(&cloud.cloudBottom_);
				const Byte* last = reinterpret_cast<const Byte*>(&cloud) + sizeof(VolumetricCloudScapes);
				for (const Byte* byte = first;byte < last;++byte)
				{
					hash = (hash ^ static_cast<Uint8>(*byte)) * 16777619u;
				}
			});
		const CelestialResult* celestial = scene.Find<CelestialResult>();
		if (celestial)
		{
			const Byte* first = reinterpret_cast<const Byte*>(celestial->skyZenithColor_);
			const Byte* last = first + sizeof(celestial->skyZenithColor_) + sizeof(celestial->skyHorizonColor_);
			for (const Byte* byte = first;byte < last;++byte)
			{
				hash = (hash ^ static_cast<Uint8>(*byte)) * 16777619u;
			}
		}

		proceduralSky_ = sourceIndex_ == SC_INVALID && cloudEnabled;
		++proceduralCounter_;
		procedural_ = proceduralSky_ && (!proceduralGenerated_ || hash != proceduralHash_ || proceduralCounter_ >= proceduralInterval_);
		if (procedural_)
		{
			proceduralGenerated_ = true;
			proceduralHash_ = hash;
			proceduralCounter_ = 0;
		}
		if (!proceduralSky_)
		{
			proceduralGenerated_ = false;
		}
		time_ = scene.views_.empty() ? 0.0f : scene.views_[0].scene_.totalTime_;

		filtering_ = equirect_ || procedural_;

		brdfLut_ = !brdfLutGenerated_;
		brdfLutGenerated_ = true;

		resolving_ = sourceIndex_ != SC_INVALID || proceduralSky_;
	}
}

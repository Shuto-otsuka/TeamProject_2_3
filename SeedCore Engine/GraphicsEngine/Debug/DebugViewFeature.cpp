#include <GraphicsEngine/Debug/DebugViewFeature.h>
#include <GraphicsEngine/Debug/DebugViewStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(DebugViewFeature, ResolveStage);

	void DebugViewFeature::Create(RenderDevice& device)
	{
		resolvePipeline_.Create(device);
	}

	void DebugViewFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		for (Bool& resolving : resolving_)
		{
			resolving = false;
		}
		for (const RenderView& view : scene.views_)
		{
			resolving_[static_cast<Uint32>(view.kind_)] = view.mode_ == RenderViewMode::Unlit || view.mode_ == RenderViewMode::Depth || view.mode_ == RenderViewMode::Meshlet || view.mode_ == RenderViewMode::Normal || view.mode_ == RenderViewMode::Roughness || view.mode_ == RenderViewMode::Metallic || view.mode_ == RenderViewMode::Emissive || view.mode_ == RenderViewMode::Velocity;
		}
	}
}

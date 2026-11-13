#include <GraphicsEngine/Debug/DebugViewFeature.h>
#include <GraphicsEngine/Debug/DebugViewStage.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(DebugViewFeature, UpdateStage, DrawStage, ResolveStage, ComposeStage);

	void DebugViewFeature::Create(RenderDevice& device)
	{
		clearPipeline_.Create(device);
		wireframePipeline_.Create(device);
		overdrawPipeline_.Create(device);
		resolvePipeline_.Create(device);
		boundsPipeline_.Create(device);
	}

	void DebugViewFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		for (Uint32 view = 0;view < RenderViewCount;++view)
		{
			clearing_[view] = false;
			wireframing_[view] = false;
			overdrawing_[view] = false;
			resolving_[view] = false;
			bounding_[view] = false;
		}
		for (const RenderView& view : scene.views_)
		{
			Size kind = static_cast<Size>(view.kind_);
			RenderViewMode mode = view.mode_;
			clearing_[kind] = mode == RenderViewMode::RayCount || mode == RenderViewMode::MeshletOverdraw;
			wireframing_[kind] = mode == RenderViewMode::Wireframe;
			overdrawing_[kind] = mode == RenderViewMode::MeshletOverdraw;
			bounding_[kind] = mode == RenderViewMode::MeshletBounds;
			resolving_[kind] = mode == RenderViewMode::Unlit || (mode >= RenderViewMode::BufferBaseColor && mode <= RenderViewMode::MeshletCulling) || mode == RenderViewMode::MeshletOverdraw || mode == RenderViewMode::LightComplexity || mode == RenderViewMode::LightCluster || mode >= RenderViewMode::RayDistance;
		}
	}
}

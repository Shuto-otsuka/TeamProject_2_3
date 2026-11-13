#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Debug/DebugViewShader.h>

namespace SeedCore
{
	class DebugViewFeature :public RenderFeature<DebugViewFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_ACCUMULATION(RayCount, RenderResourceDesc::ScaledTexture(RenderFormat::R32Uint), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero)
		);

	public:
		struct UpdateStage;

		struct DrawStage;

		struct ResolveStage;

		struct ComposeStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

	private:
		ComputePipeline<DebugViewClearDescription> clearPipeline_;

		GraphicsPipeline<DebugViewWireframeDescription> wireframePipeline_;

		GraphicsPipeline<DebugViewOverdrawDescription> overdrawPipeline_;

		ComputePipeline<DebugViewResolveDescription> resolvePipeline_;

		ComputePipeline<DebugViewBoundsDescription> boundsPipeline_;

		Bool clearing_[RenderViewCount] = { false, false, false };

		Bool wireframing_[RenderViewCount] = { false, false, false };

		Bool overdrawing_[RenderViewCount] = { false, false, false };

		Bool resolving_[RenderViewCount] = { false, false, false };

		Bool bounding_[RenderViewCount] = { false, false, false };
	};
}

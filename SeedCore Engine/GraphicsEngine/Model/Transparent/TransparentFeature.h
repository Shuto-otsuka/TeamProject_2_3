#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Model/Transparent/TransparentShader.h>

namespace SeedCore
{
	class TransparentFeature :public RenderFeature<TransparentFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_INTERNAL(Heads, RenderResourceDesc::ScaledTexture(RenderFormat::R32Uint), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Fragments, RenderResourceDesc::ScaledBuffer(sizeof(TransparentFragmentStructuredBuffer), 8, RenderResourceScale::Render, 1), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Counter, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 1), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct UpdateStage;

		struct DrawStage;

		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

	private:
		ComputePipeline<TransparentClearDescription> clearPipeline_;

		GraphicsPipeline<TransparentDrawDescription> drawPipeline_;

		ComputePipeline<TransparentResolveDescription> resolvePipeline_;
	};
}

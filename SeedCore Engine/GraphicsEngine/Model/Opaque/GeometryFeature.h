#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryShader.h>

namespace SeedCore
{
	class GeometryFeature :public RenderFeature<GeometryFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Depth, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_D32_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(HierarchicalDepth, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R32_FLOAT, RenderResourceScale::Render, 2, 8), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(VisibilityId, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R32G32_UINT), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(VisibilityTexcoord, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R32G32_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct DepthEarlyStage;

		struct EarlyDepthResolveStage;

		struct DepthLateStage;

		struct DepthResolveStage;

		struct VisibilityStage;

	public:
		void Create(RenderDevice& device);

	private:
		GraphicsPipeline<GeometryDepthDescription> depthPipeline_;

		GraphicsPipeline<GeometryVisibilityDescription> visibilityPipeline_;

		ComputePipeline<GeometryResolveDescription> resolvePipeline_;
	};
}

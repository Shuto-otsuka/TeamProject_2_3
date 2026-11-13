#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Model/Opaque/GeometryShader.h>
#include <GraphicsEngine/Model/ModelDriven.h>

namespace SeedCore
{
	class GeometryFeature :public RenderFeature<GeometryFeature>
	{
	public:
		SC_CONSTEX Uint32 instanceCapacity_ = 131072;

		SC_CONSTEX Uint32 transparentBase_ = 98304;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_ACCUMULATION(Instances, RenderResourceDesc::FixedBuffer(sizeof(ModelStructuredBuffer), instanceCapacity_), RenderResourceScope::Frame, RenderResourceLifetime::Transient),
			SC_RENDER_ACCUMULATION(Counts, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 4), RenderResourceScope::Frame, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(Depth, RenderResourceDesc::ScaledTexture(RenderFormat::D32Float), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(HierarchicalDepth, RenderResourceDesc::ScaledTexture(RenderFormat::R32Float, RenderResourceScale::Render, 2, 8), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(VisibilityId, RenderResourceDesc::ScaledTexture(RenderFormat::RG32Uint), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(VisibilityTexcoord, RenderResourceDesc::ScaledTexture(RenderFormat::RG32Float), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_DEBUG(Debug, RenderResourceDesc::ScaledTexture(RenderFormat::R32Uint), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero)
		);

	public:
		struct UpdateStage;

		struct DrawStage;

	public:
		void Create(RenderDevice& device);

	private:
		ComputePipeline<GeometryUpdateDescription> updatePipeline_;

		GraphicsPipeline<GeometryDepthDescription> depthPipeline_;

		GraphicsPipeline<GeometryVisibilityDescription> visibilityPipeline_;

		ComputePipeline<GeometryPyramidDescription> pyramidPipeline_;
	};
}

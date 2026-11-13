#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Model/Material/MaterialShader.h>

namespace SeedCore
{
	class MaterialFeature :public RenderFeature<MaterialFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(SurfaceBaseColor, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA8Unorm), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(SurfaceNormalRoughness, RenderResourceDesc::ScaledTexture(RenderFormat::R32Uint), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(SurfaceTangent, RenderResourceDesc::ScaledTexture(RenderFormat::R16Uint), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_ACCUMULATION(Velocity, RenderResourceDesc::ScaledTexture(RenderFormat::RG16Float), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_ACCUMULATION(SurfaceRadiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_DEBUG(Mip, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Buckets, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 1025), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(SortedPixels, RenderResourceDesc::ScaledBuffer(sizeof(Uint32), 1, RenderResourceScale::Render, 1), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct BuildStage;

		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

	private:
		ComputePipeline<MaterialPrepareDescription> preparePipeline_;

		ComputePipeline<MaterialClassifyDescription> classifyPipeline_;

		ComputePipeline<MaterialPrefixSumDescription> prefixSumPipeline_;

		ComputePipeline<MaterialScatterDescription> scatterPipeline_;

		ComputePipeline<MaterialResolveDescription> resolvePipeline_;
	};
}

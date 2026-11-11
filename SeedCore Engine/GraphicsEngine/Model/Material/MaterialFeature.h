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
			SC_RENDER_CONTRACT(SurfaceBaseColor, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R8G8B8A8_UNORM), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(SurfaceNormalRoughness, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R32_UINT), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(SurfaceTangent, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16_UINT), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_CONTRACT(Velocity, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_ACCUMULATION(SurfaceRadiance, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Buckets, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 1025), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(SortedPixels, RenderResourceDesc::ScaledBuffer(sizeof(Uint32), 1, RenderResourceScale::Render, 1), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct ClassifyStage;

		struct PrefixSumStage;

		struct ScatterStage;

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

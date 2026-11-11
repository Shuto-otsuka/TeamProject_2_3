#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Illumination/DirectLight/DirectLightShader.h>

namespace SeedCore
{
	class DirectLightFeature :public RenderFeature<DirectLightFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(PunctualRadiance, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero)
		);

	public:
		struct ResolveRatioStage;

	public:
		void Create(RenderDevice& device);

	private:
		ComputePipeline<DirectLightRatioDescription> ratioPipeline_;
	};
}

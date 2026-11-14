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
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero)
		);

	public:
		struct ResolveStage;

	public:
		void Create(RenderDevice& device);

	private:
		ComputePipeline<DirectLightRatioDescription> ratioPipeline_;
	};
}

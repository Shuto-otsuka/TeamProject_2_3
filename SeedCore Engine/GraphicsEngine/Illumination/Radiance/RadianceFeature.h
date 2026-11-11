#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Illumination/Radiance/RadianceShader.h>

namespace SeedCore
{
	class RadianceFeature :public RenderFeature<RadianceFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_ACCUMULATION(Color, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT), RenderResourceScope::View, RenderResourceLifetime::Transient)
		);

	public:
		struct ComposeStage;

	public:
		void Create(RenderDevice& device);

	private:
		ComputePipeline<RadianceComposeDescription> composePipeline_;
	};
}

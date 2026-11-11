#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Shape/HUD/HUDShader.h>
#include <GraphicsEngine/PostProcess/Sharpness/SharpnessFeature.h>

namespace SeedCore
{
	class HUDFeature :public RenderFeature<HUDFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_ACCUMULATION(DisplayColor, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero, &SharpnessFeature::Color),
			SC_RENDER_ACCUMULATION(Color, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R16G16B16A16_FLOAT, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_CONTRACT(ViewOutput, RenderResourceDesc::ScaledTexture(DXGI_FORMAT_R8G8B8A8_UNORM, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Imported)
		);

	public:
		struct OutputStage;

	public:
		void Create(RenderDevice& device);

	private:
		GraphicsPipeline<HUDOutputDescription> outputPipeline_;
	};
}

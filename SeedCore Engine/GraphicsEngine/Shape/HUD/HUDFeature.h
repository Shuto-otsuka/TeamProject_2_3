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
			SC_RENDER_ACCUMULATION(DisplayColor, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero, &SharpnessFeature::Color),
			SC_RENDER_ACCUMULATION(Color, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_CONTRACT(ViewOutput, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA8Unorm, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Imported),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(HUDComposeConstantBuffer)), RenderResourceScope::View, RenderResourceLifetime::Upload)
		);

	public:
		struct ComposeStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		GraphicsPipeline<HUDComposeDescription> composePipeline_;

		StaticArray<HUDComposeConstantBuffer, RenderViewCount> constants_;
	};
}

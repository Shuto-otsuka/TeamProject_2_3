#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Screen/Boot/BootShader.h>

namespace SeedCore
{
	class BootFeature :public RenderFeature<BootFeature>
	{
	public:
		SC_CONSTEX RenderFeatureSet set_ = RenderFeatureSet::Screen;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(BootConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_CONTRACT(Output, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA8Unorm, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Imported)
		);

	public:
		struct DrawStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		GraphicsPipeline<BootDrawDescription> drawPipeline_;

		BootConstantBuffer constants_;

		Bool drawing_ = false;
	};
}

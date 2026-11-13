#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/PostProcessShader.h>

namespace SeedCore
{
	class PostProcessFeature :public RenderFeature<PostProcessFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Color, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float, RenderResourceScale::Display), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(PostProcessComposeConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct ComposeStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<PostProcessComposeDescription> composePipeline_;

		PostProcessComposeConstantBuffer constants_;
	};
}

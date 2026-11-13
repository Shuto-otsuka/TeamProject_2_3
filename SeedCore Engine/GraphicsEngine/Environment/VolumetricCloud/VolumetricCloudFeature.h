#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudShader.h>

namespace SeedCore
{
	class VolumetricCloudFeature :public RenderFeature<VolumetricCloudFeature>
	{
	private:
		SC_CONSTEX Uint32 shapeSize_ = 128;

		SC_CONSTEX Uint32 detailSize_ = 64;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Constants, RenderResourceDesc::Constant(sizeof(VolumetricCloudConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_CONTRACT(Shape, RenderResourceDesc::FixedVolume(RenderFormat::R16Float, shapeSize_, shapeSize_, shapeSize_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Detail, RenderResourceDesc::FixedVolume(RenderFormat::R16Float, detailSize_, detailSize_, detailSize_), RenderResourceScope::Frame, RenderResourceLifetime::Persistent),
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero)
		);

	public:
		struct BuildStage;

		struct TraceStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<VolumetricCloudShapeDescription> shapePipeline_;

		ComputePipeline<VolumetricCloudDetailDescription> detailPipeline_;

		ComputePipeline<VolumetricCloudTraceDescription> tracePipeline_;

		VolumetricCloudConstantBuffer constants_;

		Uint32 frameIndex_ = 0;

		Bool baked_ = false;

		Bool baking_ = false;

		Bool tracing_ = false;
	};
}

#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Illumination/Shadow/Raytracing/RaytracingShadowShader.h>

namespace SeedCore
{
	class RaytracingShadowFeature :public RenderFeature<RaytracingShadowFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Visibility, RenderResourceDesc::ScaledTexture(RenderFormat::RG16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::One),
			SC_RENDER_DEBUG(Raw, RenderResourceDesc::ScaledTexture(RenderFormat::RG16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::One),
			SC_RENDER_INTERNAL(Accumulated, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(Moments, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(Age, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(Validity, RenderResourceDesc::ScaledTexture(RenderFormat::RG16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(DepthNormal, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA32Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Ping, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Pong, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(RaytracingShadowConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct TraceStage;

		struct FilterStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<RaytracingShadowTraceDescription> tracePipeline_;

		ComputePipeline<RaytracingShadowTemporalDescription> temporalPipeline_;

		ComputePipeline<RaytracingShadowMomentsDescription> momentsPipeline_;

		ComputePipeline<RaytracingShadowATrousDescription> aTrousPipeline_;

		RaytracingShadowConstantBuffer constants_;

		Bool tracing_ = false;
	};
}

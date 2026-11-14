#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflectionShader.h>

namespace SeedCore
{
	class RaytracingReflectionFeature :public RenderFeature<RaytracingReflectionFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_DEBUG(Raw, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Confidence, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_DEBUG(Reservoir, RenderResourceDesc::ScaledBuffer(sizeof(RaytracingReflectionReservoir), 1, RenderResourceScale::Render, 1), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(Reuse, RenderResourceDesc::ScaledTexture(RenderFormat::R32Uint), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Accumulated, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(Moments, RenderResourceDesc::ScaledTexture(RenderFormat::RG16Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(Age, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(Validity, RenderResourceDesc::ScaledTexture(RenderFormat::RG16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(DepthNormal, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA32Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Ping, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Pong, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(RaytracingReflectionConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct TraceStage;

		struct FilterStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		RaytracingPipeline<RaytracingReflectionTraceDescription> tracePipeline_;

		ComputePipeline<RaytracingReflectionSpatialDescription> spatialPipeline_;

		ComputePipeline<RaytracingReflectionTemporalDescription> temporalPipeline_;

		ComputePipeline<RaytracingReflectionMomentsDescription> momentsPipeline_;

		ComputePipeline<RaytracingReflectionATrousDescription> aTrousPipeline_;

		RaytracingReflectionConstantBuffer constants_;

		Bool tracing_ = false;
	};
}

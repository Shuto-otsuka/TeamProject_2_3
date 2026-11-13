#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Illumination/AmbientOcclusion/Raytracing/RaytracingAmbientOcclusionShader.h>

namespace SeedCore
{
	class RaytracingAmbientOcclusionFeature :public RenderFeature<RaytracingAmbientOcclusionFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Openness, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::One),
			SC_RENDER_DEBUG(Raw, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::One),
			SC_RENDER_INTERNAL(Accumulated, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(HistoryLength, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_DEBUG(Validity, RenderResourceDesc::ScaledTexture(RenderFormat::RG16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_DEBUG(Variance, RenderResourceDesc::ScaledTexture(RenderFormat::R16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(RaytracingAmbientOcclusionConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct TraceStage;

		struct FilterStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<RaytracingAmbientOcclusionTraceDescription> tracePipeline_;

		ComputePipeline<RaytracingAmbientOcclusionTemporalDescription> temporalPipeline_;

		RaytracingAmbientOcclusionConstantBuffer constants_;

		Bool tracing_ = false;
	};
}

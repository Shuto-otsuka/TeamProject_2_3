#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Illumination/Refraction/Raytracing/RaytracingRefractionShader.h>

namespace SeedCore
{
	class RaytracingRefractionFeature :public RenderFeature<RaytracingRefractionFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(RaytracingRefractionConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct TraceStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<RaytracingRefractionTraceDescription> tracePipeline_;

		RaytracingRefractionConstantBuffer constants_;

		Bool tracing_ = false;
	};
}

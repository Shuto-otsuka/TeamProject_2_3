#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposureShader.h>

namespace SeedCore
{
	class AutoExposureFeature :public RenderFeature<AutoExposureFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Exposure, RenderResourceDesc::FixedBuffer(sizeof(Float), 1), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Histogram, RenderResourceDesc::FixedBuffer(sizeof(Uint32), 256), RenderResourceScope::View, RenderResourceLifetime::Persistent),
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(AutoExposureConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload)
		);

	public:
		struct BuildStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<AutoExposureHistogramDescription> histogramPipeline_;

		ComputePipeline<AutoExposureAverageDescription> averagePipeline_;

		AutoExposureConstantBuffer constants_;

		Bool enabled_ = false;
	};
}

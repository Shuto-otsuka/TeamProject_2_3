#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Environment/VolumetricStar/VolumetricStarShader.h>

namespace SeedCore
{
	class VolumetricStarFeature :public RenderFeature<VolumetricStarFeature>
	{
	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_INTERNAL(Constants, RenderResourceDesc::Constant(sizeof(VolumetricStarConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_CONTRACT(Radiance, RenderResourceDesc::ScaledTexture(RenderFormat::RGBA16Float), RenderResourceScope::View, RenderResourceLifetime::Transient, RenderFallback::Zero)
		);

	public:
		struct TraceStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<VolumetricStarTraceDescription> tracePipeline_;

		VolumetricStarConstantBuffer constants_;

		StaticArray<VolumetricStarMeteor, 4> meteors_ = {};

		StaticArray<Float, 4> durations_ = {};

		std::mt19937 randomEngine_ = std::mt19937(std::random_device{}());

		Bool tracing_ = false;
	};
}

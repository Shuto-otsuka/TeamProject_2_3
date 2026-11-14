#pragma once
#include <GraphicsEngine/RenderGraph/RenderFeature.h>
#include <GraphicsEngine/Environment/Weather/WeatherShader.h>

namespace SeedCore
{
	class WeatherFeature :public RenderFeature<WeatherFeature>
	{
	private:
		SC_CONSTEX Uint32 rainCapacity_ = 10000;

		SC_CONSTEX Uint32 snowCapacity_ = 6000;

	public:
		SC_RENDER_RESOURCES
		(
			SC_RENDER_CONTRACT(Constants, RenderResourceDesc::Constant(sizeof(WeatherConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Emitter, RenderResourceDesc::Constant(sizeof(WeatherEmitterConstantBuffer)), RenderResourceScope::Frame, RenderResourceLifetime::Upload),
			SC_RENDER_INTERNAL(Particles, RenderResourceDesc::FixedBuffer(sizeof(WeatherParticleStructuredBuffer), rainCapacity_ + snowCapacity_), RenderResourceScope::View, RenderResourceLifetime::Persistent)
		);

	public:
		struct UpdateStage;

		struct DrawStage;

	public:
		void Create(RenderDevice& device);

		void Gather(const RenderScene& scene, RenderFrame& frame);

		void Prepare(const RenderFrame& frame, RenderUpload& upload);

	private:
		ComputePipeline<WeatherUpdateDescription> updatePipeline_;

		GraphicsPipeline<WeatherDrawDescription> drawPipeline_;

		WeatherConstantBuffer constants_;

		WeatherEmitterConstantBuffer emitter_;

		Bool spawned_[RenderViewCount] = {};

		Bool falling_ = false;
	};
}

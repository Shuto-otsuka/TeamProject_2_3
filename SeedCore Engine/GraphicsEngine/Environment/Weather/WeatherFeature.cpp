#include <GraphicsEngine/Environment/Weather/WeatherFeature.h>
#include <GraphicsEngine/Environment/Weather/WeatherStage.h>
#include <GraphicsEngine/Environment/Weather/Weather.h>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudScapes.h>
#include <GraphicsEngine/RenderGraph/RenderFeatureRegistry.h>
#include <GraphicsEngine/RenderGraph/RenderScene.h>
#include <GraphicsEngine/RenderGraph/RenderUpload.h>
#include <FoundationEngine/World/ECS/Query/Query.h>
#include <FoundationEngine/World/ECS/Component/Active.h>

namespace SeedCore
{
	REGISTER_RENDER_FEATURE(WeatherFeature, UpdateStage, DrawStage);

	void WeatherFeature::Create(RenderDevice& device)
	{
		updatePipeline_.Create(device);
		drawPipeline_.Create(device);
	}

	void WeatherFeature::Gather(const RenderScene& scene, RenderFrame& frame)
	{
		World& world = *scene.world_;
		constants_ = WeatherConstantBuffer{};
		emitter_ = WeatherEmitterConstantBuffer{};
		emitter_.rainCapacity_ = rainCapacity_;

		Float rainAmount = 0.0f;
		Bool cloudFound = false;
		Query<Read<Active>, Read<VolumetricCloudScapes>> cloudQuery(world);
		cloudQuery.ForEach([&](const Active& active, const VolumetricCloudScapes& cloud)
			{
				if (!active.active_ || cloudFound)
				{
					return;
				}
				cloudFound = true;
				rainAmount = cloud.rain_;
				emitter_.wind_ = Vector3(cloud.windSpeed_ * 10.0f, 0.0f, 0.0f);
			});

		Bool weatherFound = false;
		Query<Read<Active>, Read<Weather>> weatherQuery(world);
		weatherQuery.ForEach([&](const Active& active, const Weather& weather)
			{
				if (!active.active_ || weatherFound)
				{
					return;
				}
				weatherFound = true;

				constants_.wetness_ = weather.wetness_;
				constants_.snowCoverage_ = weather.snowCoverage_;
				constants_.thunderFlash_ = weather.thunderFlash_;
				constants_.thunderSeed_ = weather.thunderSeed_;
				constants_.snowIntensity_ = weather.snowIntensity_;

				const Rain& rain = weather.rain_;
				emitter_.rainCount_ = weather.rainEnabled_ ? static_cast<Uint>(rainCapacity_ * std::clamp(rain.density_, 0.0f, 1.0f) * std::clamp(rainAmount, 0.0f, 1.0f)) : 0;
				emitter_.rainFallSpeed_ = rain.fallSpeed_;
				emitter_.rainSize_ = rain.size_;
				emitter_.rainStreakLength_ = rain.streakLength_;
				emitter_.rainBrightness_ = rain.brightness_;
				emitter_.rainVolumeRadius_ = rain.volumeRadius_;
				emitter_.rainVolumeHeight_ = rain.volumeHeight_;
				emitter_.rainColor_ = Vector3(rain.color_.r, rain.color_.g, rain.color_.b);

				const Snow& snow = weather.snow_;
				emitter_.snowCount_ = weather.snowEnabled_ ? static_cast<Uint>(snowCapacity_ * std::clamp(snow.density_, 0.0f, 1.0f) * std::clamp(weather.snowIntensity_, 0.0f, 1.0f)) : 0;
				emitter_.snowFallSpeed_ = snow.fallSpeed_;
				emitter_.snowSwayAmount_ = snow.swayAmount_;
				emitter_.snowSize_ = snow.size_;
				emitter_.snowBrightness_ = snow.brightness_;
				emitter_.snowVolumeRadius_ = snow.volumeRadius_;
				emitter_.snowVolumeHeight_ = snow.volumeHeight_;
				emitter_.snowColor_ = Vector3(snow.color_.r, snow.color_.g, snow.color_.b);
			});

		falling_ = emitter_.rainCount_ + emitter_.snowCount_ > 0;
	}

	void WeatherFeature::Prepare(const RenderFrame& frame, RenderUpload& upload)
	{
		upload.Write(Constants, RenderViewKind::Game, std::span<const WeatherConstantBuffer>(&constants_, 1));
		upload.Write(Emitter, RenderViewKind::Game, std::span<const WeatherEmitterConstantBuffer>(&emitter_, 1));
	}
}

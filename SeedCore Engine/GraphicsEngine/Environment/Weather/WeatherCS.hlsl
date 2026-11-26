#include <GraphicsEngine/Environment/Weather/Weather.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>

struct WeatherUpdateBindings
{
	uint weather_emitter_;
	uint weather_particles_;
};
ConstantBuffer<WeatherUpdateBindings> update_bindings : register(b0, space1);

[numthreads(64, 1, 1)]
void Update(uint3 id : SV_DispatchThreadID)
{
	RWStructuredBuffer<WeatherParticleStructuredBuffer> particles = ResourceDescriptorHeap[update_bindings.weather_particles_];
	uint capacity;
	uint stride;
	particles.GetDimensions(capacity, stride);
	if (id.x >= capacity)
	{
		return;
	}

	ConstantBuffer<WeatherEmitterConstantBuffer> emitter = ResourceDescriptorHeap[update_bindings.weather_emitter_];
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	bool rain = id.x < emitter.rain_capacity_;
	uint local_index = rain ? id.x : id.x - emitter.rain_capacity_;
	float volume_radius = rain ? emitter.rain_volume_radius_ : emitter.snow_volume_radius_;
	float volume_height = rain ? emitter.rain_volume_height_ : emitter.snow_volume_height_;
	bool first = dispatch_bindings.constants_ != 0;

	WeatherParticleStructuredBuffer particle = particles[id.x];
	float3 relative = particle.position_ - scene.camera_position_.xyz;
	if (first || relative.y < -volume_height || dot(relative.xz, relative.xz) > volume_radius * volume_radius)
	{
		float3 random = IQHash3(float3(float(local_index) + particle.seed_ * 137.51, (rain ? 11.0 : 37.0) + particle.seed_ * 251.73, particle.seed_ * 71.31 + 1.0));
		float angle = random.x * 6.28318530718;
		float radius = sqrt(random.y) * volume_radius;
		float height = first ? (random.z * 2.0 - 1.0) * volume_height : volume_height;
		particle.position_ = scene.camera_position_.xyz + float3(cos(angle) * radius, height, sin(angle) * radius);
		particle.velocity_ = float3(0.0, -(rain ? emitter.rain_fall_speed_ : emitter.snow_fall_speed_), 0.0) + emitter.wind_ * (rain ? 1.0 : 0.3);
		particle.life_ = 0.0;
		particle.seed_ = random.z;
	}
	else
	{
		float3 sway = float3(0.0, 0.0, 0.0);
		if (!rain)
		{
			float phase = particle.seed_ * 6.28318530718;
			sway = float3(sin(scene.total_time_ * 0.8 + phase), 0.0, cos(scene.total_time_ * 0.6 + phase)) * emitter.snow_sway_amount_;
		}
		particle.position_ += (particle.velocity_ + sway) * scene.delta_time_;
		particle.life_ = min(particle.life_ + scene.delta_time_ / 0.25, 1.0);
	}
	particles[id.x] = particle;
}

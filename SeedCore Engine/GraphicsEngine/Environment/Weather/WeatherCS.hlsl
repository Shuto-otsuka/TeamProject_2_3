#include <GraphicsEngine/Environment/Weather/Weather.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>

struct WeatherUpdateBindings
{
	uint weather_emitter_;
	uint weather_particles_;
};
ConstantBuffer<WeatherUpdateBindings> update_bindings : register(b0, space1);

/**
* [EN]
* Weather.Update: advances every rain and snow particle of this view, one
* thread per entry of Weather.Particles (rain pool first, snow pool after
* it). There is no collision here; the hardware depth test of Weather.Draw
* hides whatever falls behind geometry. A particle that leaves the cylinder
* around the camera (below by volume_height, or beyond volume_radius
* horizontally) respawns at the top of the cylinder with a fresh random XZ
* offset, so the volume always surrounds the camera. The hash chains off the
* particle's previous seed so each respawn lands somewhere new. A dispatch
* constant of 1 marks the view's first frame: every particle respawns and is
* scattered over the whole cylinder height instead of the ceiling, so they
* do not fall in one synchronized sheet. Snow additionally sways with a
* phase taken from its own seed.
*
* ---------------------------------------------------------------------
*
* [JP]
* Weather.Update。このビューの雨と雪の全パーティクルを進める。Weather.Particles の
* 1 件につき 1 スレッド（前に雨の枠、後ろに雪の枠）。衝突はここでは扱わず、
* ジオメトリの奥に落ちた分は Weather.Draw のハードウェアの深度テストで隠れる。
* カメラの周りの円柱（下は volume_height、水平は volume_radius）から出た
* パーティクルは、円柱の天井へランダムな XZ のずれで出し直すので、ボリュームは
* 常にカメラを囲む。ハッシュは前の種から繋ぐので、出し直すたびに違う場所になる。
* ディスパッチの定数 1 はビューの最初のフレームの印で、全部を出し直し、天井では
* なく円柱の高さ全体へ散らす（全部が 1 枚の幕のように揃って落ちないように）。
* 雪はさらに、自分の種から取った位相で揺れる。
*/
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

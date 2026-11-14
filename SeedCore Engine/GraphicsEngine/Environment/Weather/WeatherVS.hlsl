#include <GraphicsEngine/Environment/Weather/Weather.hlsli>
#include <GraphicsEngine/Culling/FrustumCulling.hlsli>

/**
* [EN]
* Weather.Draw for devices without mesh shaders: one DrawInstanced instance
* per falling particle, 6 vertices (two triangles) each, placed exactly like
* WeatherMS.
*
* ---------------------------------------------------------------------
*
* [JP]
* メッシュシェーダーの無いデバイス用の Weather.Draw。落ちているパーティクル 1 個に
* つき DrawInstanced のインスタンス 1 つ、6 頂点（2 三角形）。配置は WeatherMS と同じ。
*/
WeatherOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	WeatherOutput output = (WeatherOutput)0;
	output.position = float4(2.0, 2.0, 2.0, 1.0);

	ConstantBuffer<WeatherEmitterConstantBuffer> emitter = ResourceDescriptorHeap[draw_bindings.weather_emitter_];
	if (instance_id >= emitter.rain_count_ + emitter.snow_count_)
	{
		return output;
	}

	bool rain = instance_id < emitter.rain_count_;
	uint buffer_index = rain ? instance_id : emitter.rain_capacity_ + instance_id - emitter.rain_count_;
	StructuredBuffer<WeatherParticleStructuredBuffer> particles = ResourceDescriptorHeap[draw_bindings.weather_particles_];
	WeatherParticleStructuredBuffer particle = particles[buffer_index];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	float size = rain ? emitter.rain_size_ : emitter.snow_size_;
	float brightness = rain ? emitter.rain_brightness_ : emitter.snow_brightness_;
	float3 color = rain ? emitter.rain_color_ : emitter.snow_color_;
	float streak_length = rain ? emitter.rain_streak_length_ : 0.0;
	if (!IsInsideFrustum(particle.position_, size * 2.0, scene.current_view_projection_))
	{
		return output;
	}

	float3 quad_up = scene.inverse_view_[1].xyz;
	float half_length = size * 0.5;
	if (streak_length > 0.0)
	{
		float speed = length(particle.velocity_);
		if (speed > 0.0001)
		{
			quad_up = particle.velocity_ / speed;
			half_length = max(size * 0.5, speed * streak_length * 0.5);
		}
	}

	float3 view_direction = normalize(scene.camera_position_.xyz - particle.position_);
	float3 quad_right = cross(quad_up, view_direction);
	float quad_right_length = length(quad_right);
	quad_right = quad_right_length > 0.0001 ? quad_right / quad_right_length : scene.inverse_view_[0].xyz;

	uint corners[6] = { 0, 1, 2, 1, 3, 2 };
	uint corner_index = corners[vertex_id % 6];
	float2 offset = float2((corner_index & 1) != 0 ? 1.0 : -1.0, (corner_index & 2) != 0 ? -1.0 : 1.0);
	float3 world_position = particle.position_ + quad_right * (offset.x * size * 0.5) + quad_up * (offset.y * half_length);
	output.position = mul(float4(world_position, 1.0), scene.current_view_projection_);
	output.local = offset;
	output.color = color * brightness * saturate(particle.life_);
	output.rain = rain ? 1 : 0;
	return output;
}

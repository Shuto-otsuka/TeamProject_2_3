#include <GraphicsEngine/Environment/Weather/Weather.hlsli>
#include <GraphicsEngine/Culling/FrustumCulling.hlsli>

/**
* [EN]
* Weather.Draw: one thread group draws 32 consecutive falling particles (the
* rain prefix first, then the snow prefix), 4 vertices and 2 triangles each.
* Each quad faces the camera; rain is stretched along its velocity by
* streak_length_, snow stays square. A particle outside the view frustum is
* placed outside the clip volume.
*
* ---------------------------------------------------------------------
*
* [JP]
* Weather.Draw。1 スレッドグループが落ちているパーティクルの連続する 32 個
* （先に雨、続いて雪）を、1 個 4 頂点・2 三角形で描く。四角形はカメラを向き、
* 雨は速度の向きへ streak_length_ の分だけ伸ばし、雪は正方形のまま。視錐台の
* 外のパーティクルはクリップ空間の外へ置く。
*/
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices WeatherOutput verts[128], out indices uint3 tris[64])
{
	ConstantBuffer<WeatherEmitterConstantBuffer> emitter = ResourceDescriptorHeap[draw_bindings.weather_emitter_];
	uint total = emitter.rain_count_ + emitter.snow_count_;
	uint first = gid * 32;
	uint particle_count = first < total ? min(32, total - first) : 0;
	SetMeshOutputCounts(particle_count * 4, particle_count * 2);
	if (particle_count == 0)
	{
		return;
	}

	uint particle_local = gtid / 4;
	if (particle_local < particle_count)
	{
		uint particle_index = first + particle_local;
		bool rain = particle_index < emitter.rain_count_;
		uint buffer_index = rain ? particle_index : emitter.rain_capacity_ + particle_index - emitter.rain_count_;
		StructuredBuffer<WeatherParticleStructuredBuffer> particles = ResourceDescriptorHeap[draw_bindings.weather_particles_];
		WeatherParticleStructuredBuffer particle = particles[buffer_index];
		SceneConstantBuffer scene = GetSceneConstantBuffer();

		float size = rain ? emitter.rain_size_ : emitter.snow_size_;
		float brightness = rain ? emitter.rain_brightness_ : emitter.snow_brightness_;
		float3 color = rain ? emitter.rain_color_ : emitter.snow_color_;
		float streak_length = rain ? emitter.rain_streak_length_ : 0.0;

		WeatherOutput output = (WeatherOutput)0;
		output.position = float4(2.0, 2.0, 2.0, 1.0);
		if (IsInsideFrustum(particle.position_, size * 2.0, scene.current_view_projection_))
		{
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

			uint corner_index = gtid % 4;
			float2 offset = float2((corner_index & 1) != 0 ? 1.0 : -1.0, (corner_index & 2) != 0 ? -1.0 : 1.0);
			float3 world_position = particle.position_ + quad_right * (offset.x * size * 0.5) + quad_up * (offset.y * half_length);
			output.position = mul(float4(world_position, 1.0), scene.current_view_projection_);
			output.local = offset;
			output.color = color * brightness * saturate(particle.life_);
			output.rain = rain ? 1 : 0;
		}
		verts[gtid] = output;
	}

	if (gtid < particle_count)
	{
		uint base = gtid * 4;
		tris[gtid * 2] = uint3(base, base + 1, base + 2);
		tris[gtid * 2 + 1] = uint3(base + 1, base + 3, base + 2);
	}
}

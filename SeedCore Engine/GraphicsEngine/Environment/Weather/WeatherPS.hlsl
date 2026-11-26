#include <GraphicsEngine/Environment/Weather/Weather.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Illumination/VolumetricLight/Froxel.hlsli>

float4 main(WeatherOutput input) : SV_Target0
{
	float alpha;
	if (input.rain != 0)
	{
		alpha = exp(-input.local.x * input.local.x * 12.0) * saturate(1.0 - abs(input.local.y));
	}
	else
	{
		alpha = saturate(1.0 - dot(input.local, input.local));
		alpha = alpha * alpha;
	}

	if (alpha <= 0.001)
	{
		discard;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture3D<float4> fog_volume = ResourceDescriptorHeap[draw_bindings.raytracing_volumetric_light_fog_];
	uint3 froxel_dimensions;
	fog_volume.GetDimensions(froxel_dimensions.x, froxel_dimensions.y, froxel_dimensions.z);
	float slice = saturate(ViewZToFroxelSlice(input.position.w, scene.near_plane_, scene.far_plane_) - 0.5 / float(froxel_dimensions.z));
	float4 fog = fog_volume.SampleLevel(sampler_linear_clamp, float3(input.position.xy * scene.inverse_screen_size_, slice), 0);
	return float4(input.color * fog.a, alpha);
}

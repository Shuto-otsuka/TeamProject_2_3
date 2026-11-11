#include <GraphicsEngine/Shader/Scene.hlsli>

struct SkyResolveBindings
{
	uint sky_cubemap_;
	uint geometry_depth_;
	uint sky_radiance_;
};
ConstantBuffer<SkyResolveBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> radiance = ResourceDescriptorHeap[bindings.sky_radiance_];
	uint width;
	uint height;
	radiance.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	Texture2D<float> depth = ResourceDescriptorHeap[bindings.geometry_depth_];
	if (depth.Load(int3(id.xy, 0)) != 0.0)
	{
		radiance[id.xy] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float2 uv = (float2(id.xy) + 0.5) / float2(width, height);
	float4 near_point = mul(float4(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0, 1.0, 1.0), scene.inverse_view_projection_);
	float3 direction = normalize(near_point.xyz / near_point.w - scene.camera_position_.xyz);

	TextureCube<float4> cubemap = ResourceDescriptorHeap[bindings.sky_cubemap_];
	radiance[id.xy] = float4(cubemap.SampleLevel(sampler_linear_clamp, direction, 0).rgb, 1.0);
}

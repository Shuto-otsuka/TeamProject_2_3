#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Illumination/VolumetricLight/Froxel.hlsli>

struct RadianceComposeBindings
{
	uint geometry_depth_;
	uint sky_radiance_;
	uint material_surface_radiance_;
	uint raytracing_volumetric_light_fog_volume_;
	uint radiance_color_;
};
ConstantBuffer<RadianceComposeBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Compose(uint3 id : SV_DispatchThreadID)
{
	RWTexture2D<float4> color = ResourceDescriptorHeap[bindings.radiance_color_];
	uint width;
	uint height;
	color.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	Texture2D<float> depth = ResourceDescriptorHeap[bindings.geometry_depth_];
	Texture2D<float4> sky = ResourceDescriptorHeap[bindings.sky_radiance_];
	Texture2D<float4> surface = ResourceDescriptorHeap[bindings.material_surface_radiance_];

	float device_depth = depth.Load(int3(id.xy, 0));
	float3 radiance = device_depth == 0.0 ? sky.Load(int3(id.xy, 0)).rgb : surface.Load(int3(id.xy, 0)).rgb;

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture3D<float4> fog_volume = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_fog_volume_];
	uint3 froxel_dimensions;
	fog_volume.GetDimensions(froxel_dimensions.x, froxel_dimensions.y, froxel_dimensions.z);
	float view_z = device_depth == 0.0 ? scene.far_plane_ : scene.near_plane_ / device_depth;
	float slice = saturate(ViewZToFroxelSlice(view_z, scene.near_plane_, scene.far_plane_) - 0.5 / float(froxel_dimensions.z));
	float4 fog = fog_volume.SampleLevel(sampler_linear_clamp, float3((float2(id.xy) + 0.5) / float2(width, height), slice), 0);
	color[id.xy] = float4(radiance * fog.a + fog.rgb, 1.0);
}

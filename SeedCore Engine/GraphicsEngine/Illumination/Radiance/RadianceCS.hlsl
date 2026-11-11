#include <GraphicsEngine/Shader/Scene.hlsli>

struct RadianceComposeBindings
{
	uint geometry_depth_;
	uint sky_radiance_;
	uint material_surface_radiance_;
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

	float3 radiance = depth.Load(int3(id.xy, 0)) == 0.0 ? sky.Load(int3(id.xy, 0)).rgb : surface.Load(int3(id.xy, 0)).rgb;
	color[id.xy] = float4(radiance, 1.0);
}

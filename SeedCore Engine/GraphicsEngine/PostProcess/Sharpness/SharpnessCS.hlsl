#include <GraphicsEngine/Shader/Scene.hlsli>

struct SharpnessConstantBuffer
{
	float amount_;
	float3 sharpness_padding_0_;
};

struct SharpnessResolveBindings
{
	uint tone_mapping_color_;
	uint sharpness_constants_;
	uint sharpness_color_;
};
ConstantBuffer<SharpnessResolveBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[bindings.sharpness_color_];
	ConstantBuffer<SharpnessConstantBuffer> constants = ResourceDescriptorHeap[bindings.sharpness_constants_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	Texture2D<float4> source = ResourceDescriptorHeap[bindings.tone_mapping_color_];

	int2 pixel = int2(dtid.xy);
	int2 pixel_max = int2(width - 1, height - 1);

	float3 center = source.Load(int3(pixel, 0)).rgb;
	float3 north = source.Load(int3(clamp(pixel + int2(0, -1), int2(0, 0), pixel_max), 0)).rgb;
	float3 south = source.Load(int3(clamp(pixel + int2(0, 1), int2(0, 0), pixel_max), 0)).rgb;
	float3 west = source.Load(int3(clamp(pixel + int2(-1, 0), int2(0, 0), pixel_max), 0)).rgb;
	float3 east = source.Load(int3(clamp(pixel + int2(1, 0), int2(0, 0), pixel_max), 0)).rgb;

	float3 blur = (north + south + west + east) * 0.25;
	float amount = constants.amount_;
	float3 color = saturate(center + (center - blur) * amount);

	destination[dtid.xy] = float4(color, 1.0);
}

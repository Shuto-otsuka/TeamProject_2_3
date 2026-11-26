#include <GraphicsEngine/Shader/Scene.hlsli>

struct FilmGrainConstantBuffer
{
	uint colored_;
	float intensity_;
	float size_;
	float luminance_response_;
};

struct FilmGrainResolveBindings
{
	uint sharpness_color_;
	uint film_grain_constants_;
	uint hud_display_;
};
ConstantBuffer<FilmGrainResolveBindings> bindings : register(b0, space1);

float GrainHash(float2 position, float seed)
{
	return frac(sin(dot(position, float2(12.9898, 78.233)) + seed) * 43758.5453);
}

float GrainLuminanceResponse(float luminance, float response)
{
	float midtone_weight = 4.0 * luminance * (1.0 - luminance);
	return lerp(1.0, saturate(midtone_weight), saturate(response));
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[bindings.hud_display_];
	ConstantBuffer<FilmGrainConstantBuffer> constants = ResourceDescriptorHeap[bindings.film_grain_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[bindings.sharpness_color_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	float3 color = source.Load(int3(dtid.xy, 0)).rgb;

	float intensity = constants.intensity_;
	float size = max(constants.size_, 1.0);
	float response = constants.luminance_response_;
	bool colored = constants.colored_ != 0;

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float seed = scene.total_time_;

	float2 grain_position = floor(float2(dtid.xy) / size);

	float3 grain;
	if (colored)
	{
		grain.r = GrainHash(grain_position, seed);
		grain.g = GrainHash(grain_position, seed + 17.0);
		grain.b = GrainHash(grain_position, seed + 43.0);
	}
	else
	{
		grain = GrainHash(grain_position, seed).xxx;
	}

	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float weight = GrainLuminanceResponse(saturate(luminance), response) * intensity;

	destination[dtid.xy] = float4(saturate(color + (grain * 2.0 - 1.0) * weight), 1.0);
}

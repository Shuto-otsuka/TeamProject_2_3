#include <GraphicsEngine/Shader/Scene.hlsli>

struct LensDistortionConstantBuffer
{
	float k1_;
	float k2_;
	float k3_;
	float scale_;
};

struct LensDistortionResolveBindings
{
	uint bokeh_color_;
	uint lens_distortion_constants_;
	uint lens_distortion_color_;
};
ConstantBuffer<LensDistortionResolveBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[bindings.lens_distortion_color_];
	ConstantBuffer<LensDistortionConstantBuffer> constants = ResourceDescriptorHeap[bindings.lens_distortion_constants_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	Texture2D<float4> source = ResourceDescriptorHeap[bindings.bokeh_color_];

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	float k1 = constants.k1_;
	float k2 = constants.k2_;
	float k3 = constants.k3_;
	float scale = max(constants.scale_, 0.0001);

	float aspect = float(width) / float(height);
	float2 centered = (uv - 0.5) * float2(aspect, 1.0);

	float radius_squared = dot(centered, centered);
	float radius_fourth = radius_squared * radius_squared;
	float radius_sixth = radius_fourth * radius_squared;

	float distortion = 1.0 + k1 * radius_squared + k2 * radius_fourth + k3 * radius_sixth;

	float2 distorted = centered * distortion / scale;
	float2 sample_uv = distorted / float2(aspect, 1.0) + 0.5;

	destination[dtid.xy] = float4(source.SampleLevel(sampler_linear_clamp, sample_uv, 0).rgb, 1.0);
}

#include <GraphicsEngine/Shader/Scene.hlsli>

struct LensFlareConstantBuffer
{
	float threshold_;
	float intensity_;
	float streak_length_;
	float streak_attenuation_;
	float chromatic_aberration_;
	float angle_offset_;
	uint ghost_count_;
	float ghost_dispersal_;
	float ghost_intensity_;
	float halo_width_;
	uint axis_count_;
	float spike_variation_;
};

float3 BrightPass(float3 color, float threshold)
{
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float weight = max(luminance - threshold, 0.0) / max(luminance, 0.0001);
	return color * weight;
}

float2 AxisDirection(uint axis, float angle_offset, uint axis_count)
{
	float angle = angle_offset + (3.14159265 / float(axis_count)) * float(axis);
	return float2(cos(angle), sin(angle));
}

float AxisVariation(uint axis, float variation, float phase)
{
	float noise = frac(sin(float(axis) * 12.9898 + phase) * 43758.5453);
	return 1.0 - variation * noise;
}

struct LensFlareDownsampleBindings
{
	uint bokeh_color_;
	uint lens_flare_constants_;
	uint lens_flare_bright_;
};
ConstantBuffer<LensFlareDownsampleBindings> downsample_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Downsample(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2DArray<float4> destination = ResourceDescriptorHeap[downsample_bindings.lens_flare_bright_];

	uint width, height, slices;
	destination.GetDimensions(width, height, slices);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<LensFlareConstantBuffer> constants = ResourceDescriptorHeap[downsample_bindings.lens_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[downsample_bindings.bokeh_color_];

	uint source_width, source_height;
	source.GetDimensions(source_width, source_height);
	float2 source_texel = 1.0 / float2(source_width, source_height);

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float threshold = constants.threshold_;

	float3 result = BrightPass(source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, -1.0) * source_texel, 0).rgb, threshold);
	result += BrightPass(source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, -1.0) * source_texel, 0).rgb, threshold);
	result += BrightPass(source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, 1.0) * source_texel, 0).rgb, threshold);
	result += BrightPass(source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, 1.0) * source_texel, 0).rgb, threshold);

	destination[uint3(dtid.xy, 0)] = float4(result * 0.25, 1.0);
}

float3 StreakGather(Texture2DArray<float4> source, float slice, float2 uv, float2 step_vector, float spacing, float attenuation)
{
	float safe_attenuation = clamp(attenuation, 0.5, 0.999);

	float3 sum = source.SampleLevel(sampler_linear_clamp, float3(uv, slice), 0).rgb;
	float weight_sum = 1.0;

	[unroll]
	for (uint tap = 1; tap <= 3; tap++)
	{
		float texel_distance = spacing * float(tap);
		float weight = pow(safe_attenuation, texel_distance);
		float2 offset = step_vector * texel_distance;

		sum += source.SampleLevel(sampler_linear_clamp, float3(uv + offset, slice), 0).rgb * weight;
		sum += source.SampleLevel(sampler_linear_clamp, float3(uv - offset, slice), 0).rgb * weight;
		weight_sum += weight * 2.0;
	}

	return sum / weight_sum;
}

void BlurPass(uint3 dtid, float spacing, Texture2DArray<float4> source, bool shared_source, RWTexture2DArray<float4> destination, ConstantBuffer<LensFlareConstantBuffer> constants)
{
	uint width, height, slices;
	destination.GetDimensions(width, height, slices);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float2 texel = 1.0 / float2(width, height);
	float angle_offset = constants.angle_offset_;
	float attenuation = constants.streak_attenuation_;
	float length_scale = constants.streak_length_ * 4.0;
	uint axis_count = constants.axis_count_;
	float variation = constants.spike_variation_;

	[loop]
	for (uint axis = 0; axis < axis_count; axis++)
	{
		float2 step_vector = AxisDirection(axis, angle_offset, axis_count) * texel * length_scale;
		float axis_attenuation = attenuation * AxisVariation(axis, variation * 0.15, 0.0);
		float3 result = StreakGather(source, shared_source ? 0.0 : float(axis), uv, step_vector, spacing, axis_attenuation);
		destination[uint3(dtid.xy, axis)] = float4(result, 1.0);
	}
}

struct LensFlareBlurBindings
{
	uint lens_flare_constants_;
	uint lens_flare_source_;
	uint lens_flare_destination_;
};
ConstantBuffer<LensFlareBlurBindings> blur_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Blur(uint3 dtid : SV_DispatchThreadID)
{
	uint iteration = dispatch_bindings.constants_;
	ConstantBuffer<LensFlareConstantBuffer> constants = ResourceDescriptorHeap[blur_bindings.lens_flare_constants_];
	Texture2DArray<float4> source = ResourceDescriptorHeap[blur_bindings.lens_flare_source_];
	RWTexture2DArray<float4> destination = ResourceDescriptorHeap[blur_bindings.lens_flare_destination_];
	BlurPass(dtid, float(1u << (iteration * 2)), source, iteration == 0, destination, constants);
}

struct LensFlareGlareBindings
{
	uint lens_flare_constants_;
	uint lens_flare_pong_;
	uint lens_flare_radiance_;
};
ConstantBuffer<LensFlareGlareBindings> glare_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Glare(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[glare_bindings.lens_flare_radiance_];

	uint width, height;
	output.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<LensFlareConstantBuffer> constants = ResourceDescriptorHeap[glare_bindings.lens_flare_constants_];
	Texture2DArray<float4> axis_source = ResourceDescriptorHeap[glare_bindings.lens_flare_pong_];

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float chromatic_aberration = constants.chromatic_aberration_;
	float intensity = constants.intensity_;
	float angle_offset = constants.angle_offset_;
	uint axis_count = constants.axis_count_;
	float variation = constants.spike_variation_;

	float3 accum = float3(0.0, 0.0, 0.0);

	[loop]
	for (uint axis = 0; axis < axis_count; axis++)
	{
		float2 direction = AxisDirection(axis, angle_offset, axis_count);

		float2 fringe = direction * chromatic_aberration;

		float r = axis_source.SampleLevel(sampler_linear_clamp, float3(uv + fringe, axis), 0).r;
		float g = axis_source.SampleLevel(sampler_linear_clamp, float3(uv, axis), 0).g;
		float b = axis_source.SampleLevel(sampler_linear_clamp, float3(uv - fringe, axis), 0).b;

		accum += float3(r, g, b) * AxisVariation(axis, variation, 7.0);
	}

	output[dtid.xy] = float4(accum * intensity, 1.0);
}

float3 SampleDistorted(Texture2DArray<float4> source, float2 uv, float2 direction, float3 distortion)
{
	float r = source.SampleLevel(sampler_linear_wrap, float3(uv + direction * distortion.r, 0.0), 0).r;
	float g = source.SampleLevel(sampler_linear_wrap, float3(uv + direction * distortion.g, 0.0), 0).g;
	float b = source.SampleLevel(sampler_linear_wrap, float3(uv + direction * distortion.b, 0.0), 0).b;
	return float3(r, g, b);
}

float3 LensColor(float radius01)
{
	return 0.6 + 0.4 * cos(6.28318530718 * (radius01 + float3(0.0, 0.33, 0.67)));
}

struct LensFlareGhostBindings
{
	uint lens_flare_constants_;
	uint lens_flare_bright_;
	uint lens_flare_radiance_;
};
ConstantBuffer<LensFlareGhostBindings> ghost_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Ghost(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[ghost_bindings.lens_flare_radiance_];

	uint width, height;
	output.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<LensFlareConstantBuffer> constants = ResourceDescriptorHeap[ghost_bindings.lens_flare_constants_];
	Texture2DArray<float4> source = ResourceDescriptorHeap[ghost_bindings.lens_flare_bright_];

	float2 uv = 1.0 - (float2(dtid.xy) + 0.5) / float2(width, height);

	float2 aspect_scale = float2(float(width) / float(height), 1.0);
	float max_radius = length(float2(0.5, 0.5) * aspect_scale);

	uint ghost_count = min(constants.ghost_count_, 8u);
	float ghost_dispersal = constants.ghost_dispersal_;
	float ghost_intensity = constants.ghost_intensity_;
	float halo_width = constants.halo_width_;
	float chromatic_aberration = constants.chromatic_aberration_;
	float intensity = constants.intensity_;

	float3 distortion = float3(-chromatic_aberration, 0.0, chromatic_aberration);

	float2 to_center = float2(0.5, 0.5) - uv;
	float2 ghost_direction = normalize(to_center + 1e-6);
	float2 ghost_vector = to_center * ghost_dispersal;

	float3 result = float3(0.0, 0.0, 0.0);

	[loop]
	for (uint index = 0; index < ghost_count; index++)
	{
		float2 sample_uv = uv + ghost_vector * float(index);
		float radius01 = length((frac(sample_uv) - 0.5) * aspect_scale) / max_radius;
		float weight = pow(saturate(1.0 - radius01), 10.0);

		result += SampleDistorted(source, sample_uv, ghost_direction, distortion) * weight;
	}

	result *= LensColor(saturate(length(to_center * aspect_scale) / max_radius));

	if (halo_width > 0.0)
	{
		float2 halo_direction = normalize(to_center * aspect_scale + 1e-6) / aspect_scale;
		float2 halo_uv = uv + halo_direction * halo_width;
		float halo_radius01 = length((frac(halo_uv) - 0.5) * aspect_scale) / max_radius;
		float halo_weight = pow(saturate(1.0 - halo_radius01), 5.0);

		result += SampleDistorted(source, halo_uv, halo_direction, distortion) * halo_weight;
	}

	result *= ghost_intensity * intensity;

	output[dtid.xy] = float4(output[dtid.xy].rgb + result, 1.0);
}

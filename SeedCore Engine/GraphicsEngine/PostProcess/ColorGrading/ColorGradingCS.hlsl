#include <GraphicsEngine/Shader/Scene.hlsli>

struct ColorGradingRange
{
	float temperature_;
	float saturation_;
	float contrast_;
	float gamma_;
	float gain_;
	float offset_;
	float2 color_grading_range_padding_0_;
};

struct ColorGradingConstantBuffer
{
	float shadows_max_;
	float highlights_min_;
	float2 color_grading_padding_0_;
	ColorGradingRange global_;
	ColorGradingRange shadows_;
	ColorGradingRange midtones_;
	ColorGradingRange highlights_;
};

struct ColorGradingResolveBindings
{
	uint post_process_color_;
	uint color_grading_constants_;
	uint color_grading_color_;
};
ConstantBuffer<ColorGradingResolveBindings> bindings : register(b0, space1);

float3 WhiteBalanceCoefficients(float temperature)
{
	float shift = temperature * 1.5;

	float x = 0.31271 - shift * (shift < 0.0 ? 0.1 : 0.05);
	float y = 2.87 * x - 3.0 * x * x - 0.27509507;

	float capital_y = 1.0;
	float capital_x = capital_y * x / max(y, 0.0001);
	float capital_z = capital_y * (1.0 - x - y) / max(y, 0.0001);

	float3 target_lms;
	target_lms.x = 0.7328 * capital_x + 0.4296 * capital_y - 0.1624 * capital_z;
	target_lms.y = -0.7036 * capital_x + 1.6975 * capital_y + 0.0061 * capital_z;
	target_lms.z = 0.0030 * capital_x + 0.0136 * capital_y + 0.9834 * capital_z;

	const float3 d65_lms = float3(0.949237, 1.03542, 1.08728);

	return d65_lms / max(target_lms, 0.0001);
}

float3 ApplyWhiteBalance(float3 color, float temperature)
{
	const float3x3 linear_to_lms = float3x3(
		3.90405e-1, 5.49941e-1, 8.92632e-3,
		7.08416e-2, 9.63172e-1, 1.35775e-3,
		2.31082e-2, 1.28021e-1, 9.36245e-1);

	const float3x3 lms_to_linear = float3x3(
		2.85847e+0, -1.62879e+0, -2.48910e-2,
		-2.10182e-1, 1.15820e+0, 3.24281e-4,
		-4.18120e-2, -1.18169e-1, 1.06867e+0);

	float3 lms = mul(linear_to_lms, color);
	lms *= WhiteBalanceCoefficients(temperature);

	return mul(lms_to_linear, lms);
}

float3 ColorCorrect(float3 color, float temperature, float saturation, float contrast, float gamma, float gain, float offset)
{
	color = max(0.0, ApplyWhiteBalance(color, temperature));

	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	color = max(0.0, lerp(luminance.xxx, color, saturation));

	float safe_contrast = clamp(contrast, 0.1, 4.0);
	float safe_gamma = clamp(gamma, 0.1, 4.0);

	color = pow(max(color, 0.0) * (1.0 / 0.18), safe_contrast) * 0.18;
	color = pow(max(color, 0.0), 1.0 / safe_gamma);

	return color * gain + offset;
}

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[bindings.color_grading_color_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<ColorGradingConstantBuffer> constants = ResourceDescriptorHeap[bindings.color_grading_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[bindings.post_process_color_];
	float3 color = source.Load(int3(dtid.xy, 0)).rgb;

	ColorGradingRange global_range = constants.global_;
	ColorGradingRange shadows_range = constants.shadows_;
	ColorGradingRange midtones_range = constants.midtones_;
	ColorGradingRange highlights_range = constants.highlights_;

	float3 shadows_result = ColorCorrect(color,
		global_range.temperature_ + shadows_range.temperature_,
		global_range.saturation_ * shadows_range.saturation_,
		global_range.contrast_ * shadows_range.contrast_,
		global_range.gamma_ * shadows_range.gamma_,
		global_range.gain_ * shadows_range.gain_,
		global_range.offset_ + shadows_range.offset_);

	float3 midtones_result = ColorCorrect(color,
		global_range.temperature_ + midtones_range.temperature_,
		global_range.saturation_ * midtones_range.saturation_,
		global_range.contrast_ * midtones_range.contrast_,
		global_range.gamma_ * midtones_range.gamma_,
		global_range.gain_ * midtones_range.gain_,
		global_range.offset_ + midtones_range.offset_);

	float3 highlights_result = ColorCorrect(color,
		global_range.temperature_ + highlights_range.temperature_,
		global_range.saturation_ * highlights_range.saturation_,
		global_range.contrast_ * highlights_range.contrast_,
		global_range.gamma_ * highlights_range.gamma_,
		global_range.gain_ * highlights_range.gain_,
		global_range.offset_ + highlights_range.offset_);

	float shadows_max = constants.shadows_max_;
	float highlights_min = constants.highlights_min_;

	float graded_luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float shadows_weight = 1.0 - smoothstep(0.0, max(shadows_max, 0.0001), graded_luminance);
	float highlights_weight = smoothstep(highlights_min, max(highlights_min * 2.0, highlights_min + 0.0001), graded_luminance);
	float midtones_weight = saturate(1.0 - shadows_weight - highlights_weight);

	color = shadows_result * shadows_weight + midtones_result * midtones_weight + highlights_result * highlights_weight;

	destination[dtid.xy] = float4(max(color, 0.0), 1.0);
}

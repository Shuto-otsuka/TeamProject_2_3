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

/**
* [EN]
* Reference:
* - https://en.wikipedia.org/wiki/Unsharp_masking
*
* This is the textbook unsharp mask, not a published real-time technique -
* there is no graphics paper behind it. Worth knowing that the modern
* real-time standard for this slot is AMD FidelityFX CAS (Contrast Adaptive
* Sharpening, https://gpuopen.com/fidelityfx-cas/), which varies sharpening
* strength per pixel by local contrast so flat regions do not gain noise and
* already-contrasty edges do not ring. This pass does neither - it applies
* amount_ uniformly. If sharpening ever needs to be pushed hard enough that
* halos or amplified noise show up, replacing this with CAS is the fix.
*
* Reads ToneMapping.Color (tone-mapped and sRGB-encoded) and applies a 5-tap
* cross unsharp mask into Sharpness.Color, pushing each pixel away from its
* 4-neighbor average by amount_ - the classic sharpen/edge-enhancement look.
* When SharpnessSettings.enabled_ is off the Stage does not run and
* Sharpness.Color passes ToneMapping.Color through.
*
* ---------------------------------------------------------------------
*
* [JP]
* ToneMapping.Color（トーンマップ済み・sRGB エンコード済み）を読み、5 タップの
* 十字アンシャープマスクを掛けて Sharpness.Color へ書く。各ピクセルを 4 近傍
* 平均から amount_ ぶん引き離す — 典型的なシャープ/輪郭強調の見た目。
* SharpnessSettings.enabled_ がオフなら Stage は走らず、Sharpness.Color は
* ToneMapping.Color を素通しで指す。
*/
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

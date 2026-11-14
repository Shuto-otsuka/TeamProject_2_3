#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/PostProcess/ToneMapping/ToneMapping.hlsli>

struct ToneMappingConstantBuffer
{
	uint enabled_;
	uint mode_;
	float2 tone_mapping_padding_0_;
};

struct ToneMappingResolveBindings
{
	uint color_grading_color_;
	uint tone_mapping_constants_;
	uint tone_mapping_color_;
};
ConstantBuffer<ToneMappingResolveBindings> bindings : register(b0, space1);

/**
* [EN]
* The display pass: reads the exposed and graded HDR color (ColorGrading.Color,
* which passes PostProcess.Color through when grading is off), applies the
* selected tone curve (or none - clamp only), and encodes to sRGB into
* ToneMapping.Color. Runs every frame regardless of
* ToneMappingSettings.enabled_ - that toggles only whether the CURVE applies -
* because the sRGB encode is not optional: the views are shown through
* R8G8B8A8_UNORM targets with no hardware sRGB view anywhere in this engine,
* so this is the only place a linear HDR value is gamma-encoded for display.
*
* [JP]
* 表示パス: 露出とグレーディング済みの HDR カラー（ColorGrading.Color。
* グレーディングが無効なら PostProcess.Color を素通しで指す）を読み、選択された
* トーンカーブ（またはカーブ無し=クランプのみ）を掛け、sRGB へエンコードして
* ToneMapping.Color へ書く。ToneMappingSettings.enabled_ に関わらず毎フレーム
* 走る — それが切り替えるのは【カーブ】の適用有無だけ。sRGB エンコードは
* 省略できない: ビューは R8G8B8A8_UNORM のターゲットで表示され、このエンジン
* にはハードウェア sRGB ビューがどこにも無いので、リニアな HDR 値が表示用に
* ガンマエンコードされる場所はここが唯一。
*/

[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[bindings.tone_mapping_color_];

	uint width, height;
	output.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	ConstantBuffer<ToneMappingConstantBuffer> constants = ResourceDescriptorHeap[bindings.tone_mapping_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[bindings.color_grading_color_];
	float3 color = source.Load(int3(dtid.xy, 0)).rgb;

	if (constants.enabled_ != 0)
	{
		uint mode = constants.mode_;
		if (mode == 1)
		{
			color = ReinhardToneMap(color);
		}
		else if (mode == 2)
		{
			color = AcesFilmicToneMap(color);
		}
		else if (mode == 3)
		{
			color = PbrNeutralToneMap(color);
		}
		/// [EN] mode == 0 (None): no curve, only the saturate below applies.
		/// [JP] mode == 0 (None): カーブ無し、下の saturate だけがかかる。
	}

	color = saturate(color);
	color = LinearToSrgb(color);

	output[dtid.xy] = float4(color, 1.0);
}

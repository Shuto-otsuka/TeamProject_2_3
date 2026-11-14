#include <GraphicsEngine/Shader/Scene.hlsli>

struct VignetteConstantBuffer
{
	float intensity_;
	float exponent_;
	float2 vignette_padding_0_;
	float4 color_;
};

struct VignetteResolveBindings
{
	uint chromatic_aberration_color_;
	uint vignette_constants_;
	uint vignette_color_;
};
ConstantBuffer<VignetteResolveBindings> bindings : register(b0, space1);

/**
* [EN]
* Reference:
* - https://www.cs.cmu.edu/~sensing-sensors/readings/vignetting.pdf
* - https://en.wikipedia.org/wiki/Vignetting
*
* Natural vignetting: the cosine-fourth law. Illumination at the sensor
* falls off as cos^4 of the angle off the optical axis, a consequence of the
* projected areas of the pupil and the pixel combined with the inverse
* square law. Unlike optical vignetting (lens elements shading each other,
* which stopping down cures) or mechanical vignetting (a hood clipping the
* corners), this one is intrinsic to every lens including an ideal one, so
* it is the kind worth modelling as the default.
*
* exponent_ defaults to 4 for exactly that reason - that value IS the
* physical law. Moving it is art direction, so it is a slider rather than a
* constant, but 4 is where it belongs if the goal is a plausible lens.
*
* This pass runs BEFORE exposure and the tone curve. Vignetting is light
* that never reached the sensor, so it has to be part of what gets exposed;
* darkening the already-exposed, already-tone-mapped image instead would
* respond quite differently to the curve and would not survive auto
* exposure sensibly.
*
* Reading and writing the same texture is safe here and is what the
* renderer does when chromatic aberration ran first: this is a pure
* per-pixel multiply with no neighbour taps, so there is no read-write
* hazard (the same reason BokehCS.hlsl may read-modify-write depth of
* field's buffer).
*
* ---------------------------------------------------------------------
*
* [JP]
* 自然ビネット: コサイン4乗則。センサー面の照度は光軸からの角度の cos^4 で
* 落ちる。瞳と画素の投影面積、および逆二乗則の帰結。光学ビネット(素子同士の
* 遮蔽。絞れば解消する)や機械ビネット(フードによる四隅のケラレ)と違い、
* これは理想レンズを含むあらゆるレンズに内在するので、既定でモデル化する
* 価値があるのはこちら。
*
* exponent_ の既定値が4なのはまさにその理由で、この値が【物理法則そのもの】。
* 動かすのはアートディレクションなので定数ではなくスライダーにしてあるが、
* もっともらしいレンズを目指すなら4が本来の位置。
*
* このパスは露出とトーンカーブより【前】に走る。ビネットは「そもそも
* センサーへ届かなかった光」なので、露出される対象そのものに含まれて
* いなければならない。露出もトーンマップも済んだ画像を後から暗くすると、
* カーブへの応答がまるで変わるし、自動露出とも噛み合わない。
*
* 同じテクスチャを読み書きしても安全で、色収差が先に走った場合に
* レンダラーが実際そうする: これは近傍タップの無い画素ごとの単純な乗算
* なので読み書きの競合が起きない(BokehCS.hlsl が被写界深度のバッファを
* read-modify-write できるのと同じ理由)。
*/
[numthreads(8, 8, 1)]
void main(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[bindings.vignette_color_];
	ConstantBuffer<VignetteConstantBuffer> constants = ResourceDescriptorHeap[bindings.vignette_constants_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	Texture2D<float4> source = ResourceDescriptorHeap[bindings.chromatic_aberration_color_];

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float3 color = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;

	float intensity = constants.intensity_;
	float exponent = constants.exponent_;
	float3 vignette_color = constants.color_.rgb;

	float aspect = float(width) / float(height);
	float2 centered = (uv - 0.5) * float2(aspect, 1.0);
	float radius = length(centered) / length(float2(0.5 * aspect, 0.5));

	float angle = atan(radius);
	float falloff = pow(saturate(cos(angle)), max(exponent, 0.0));

	float blend = pow(saturate(falloff), max(intensity, 0.0));

	destination[dtid.xy] = float4(lerp(vignette_color, color, blend), 1.0);
}

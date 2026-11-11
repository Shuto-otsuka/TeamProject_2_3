#include <GraphicsEngine/Shader/Scene.hlsli>

struct AnamorphicFlareConstantBuffer
{
	float threshold_;
	float intensity_;
	float streak_length_;
	float attenuation_;
	float4 tint_;
};


/**
* [EN]
* Reference:
* - https://bartwronski.com/2015/03/09/anamorphic-lens-flares-and-visual-effects/
* - https://github.com/keijiro/KinoStreak
* - https://www.chrisoat.com/papers/Oat-ScenePostprocessing.pdf
*
* Anamorphic flare: the long horizontal streak a cine anamorphic lens throws
* off bright points. Separate from LensFlareCS.hlsl because the cause is
* different - that one is aperture diffraction plus inter-element
* reflections, this one is the cylindrical squeeze element.
*
* The streak is horizontal for a non-obvious reason, and getting the order
* right is what makes this implementation simple. An anamorphic lens
* SQUEEZES the image 2:1 horizontally while shooting. The internal
* reflection happens inside that squeezed space as an ordinary ROUND flare.
* Projection then stretches the image back out, and the round flare becomes
* a horizontal streak. So the passes here work in a horizontally squeezed
* buffer (half the width of the other quarter-res post-process buffers) and
* Compose samples it back with normal UVs - the 2x horizontal stretch falls
* out of the sampling for free, with no directional logic needed for it.
*
* A 2:1 squeeze on its own only yields a 2:1 ellipse though, and real
* anamorphic streaks are far longer than that. So BlurPass1..4 additionally
* blur horizontally inside the squeezed space, out to streak_length_. That
* part is art direction rather than physics and it is not energy conserving
* - every shipped implementation of this effect does the same (KinoStreak
* says as much about itself).
*
* The blur itself is Kawase's light streak, the same kernel LensFlareCS.hlsl
* uses: 3 taps per side at a texel spacing of 4^(pass-1), a tap `d` texels
* out weighted attenuation^d, normalized by the weight sum. The exponential
* decay IS the streak's falloff - an equal-weight box average produces a bar
* of constant brightness instead of light.
*
* ---------------------------------------------------------------------
*
* [JP]
* アナモルフィックフレア: シネマ用アナモルフィックレンズが明るい点に対して
* 出す、横方向の長い筋。LensFlareCS.hlsl と分けているのは原因が別だから —
* あちらは絞りの回折と素子間反射、こちらはシリンドリカル(円柱)圧縮素子。
*
* 筋が横向きになる理由は直感に反し、その順序を正しく捉えることがこの実装を
* 単純にしている。アナモルフィックレンズは撮影時に像を水平方向へ【2:1に
* 圧縮】する。レンズ内部反射はその圧縮空間の中で【普通の丸いフレア】として
* 起きる。上映時に水平へ引き伸ばして戻すことで、丸かったフレアが横長の筋に
* なる。したがってここのパス群は横に圧縮されたバッファ(他の1/4解像度
* ポストプロセスバッファの半分の幅)の中で処理し、Compose が通常のUVで
* サンプルして戻す — 横2倍の引き伸ばしはサンプル時に勝手に起きるので、
* そのための方向処理は一切要らない。
*
* ただし 2:1 の圧縮だけでは 2:1 の楕円にしかならず、実写のアナモルフィック
* 筋はそれよりはるかに細長い。そこで BlurPass1..4 が圧縮空間の中でさらに
* 横方向へ streak_length_ ぶんブラーして長さを稼ぐ。この部分は物理では
* なくアートディレクションで、エネルギー保存もしていない — このエフェクトの
* 実装は軒並みそうしている(KinoStreak は自らそう明言している)。
*
* ブラー自体は Kawase のライトストリークで、LensFlareCS.hlsl と同じカーネル:
* 片側3タップを 4^(パス-1) テクセル間隔で取り、d テクセル先のタップの重みを
* attenuation^d にし、重みの合計で正規化する。この指数減衰が【筋の減衰
* そのもの】で、等重みの箱平均にすると明るさが一定のバーになって光に
* 見えない。
*/

/**
* [EN]
* Soft threshold weighting used before the bright-pass extraction below (the
* Prefilter pass). Fades to 0 as luminance approaches threshold from above,
* rather than a hard cutoff.
*
* ---------------------------------------------------------------------
*
* [JP]
* 下のブライトパス抽出(Prefilterパス)前に使うソフトなしきい値重み。
* 硬い切り捨てではなく、輝度が threshold へ上から近づくにつれて0へ
* フェードする。
*/
float3 AnamorphicBrightPass(float3 color, float threshold)
{
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float weight = max(luminance - threshold, 0.0) / max(luminance, 0.0001);
	return color * weight;
}

/**
* [EN]
* Kawase's light-streak kernel (fixed to horizontal). Center tap + 3 taps on
* each side, weighted by attenuation^(texel distance).
*
* The attenuation must always be kept below 1. If it exceeds 1, pow becomes
* exponential 【growth】 instead of decay, and since the texel distance
* reaches up to 64*3 = 192, this quickly hits Inf. The final
* sum / weight_sum then returns Inf/Inf = NaN, which spreads from Compose
* into the output buffer and across the whole screen when
* PostProcessCS.hlsl adds it into hdr_color - turning the frame solid black.
* The CPU-side slider range cannot be trusted - a scene saved before that
* range changed can still carry an old-range value, and this actually
* produced a black screen on the lens flare side.
*
* Normalizing by the weight sum is also required - without it each pass
* would apply up to 7x gain and saturate over 4 passes. Overall brightness
* is intensity_'s job, not the kernel gain.
*
* ---------------------------------------------------------------------
*
* [JP]
* Kawase のライトストリークのカーネル(横方向固定)。中心1タップ + 左右3
* タップずつ、重みは attenuation^(テクセル距離)。
*
* 減衰は必ず1未満に抑える。1を超えると pow が減衰ではなく指数【増加】に
* なり、テクセル距離が最大 64*3 = 192 まで伸びるためすぐ Inf になる。
* すると最後の sum / weight_sum が Inf/Inf = NaN を返し、NaN が Compose
* から出力バッファへ、さらに PostProcessCS.hlsl の hdr_color への加算で
* 画面全体へ広がって真っ黒になる。CPU側のスライダー範囲を信用しては
* いけない - レンジ変更前に保存されたシーンには旧レンジの値が入っており、
* レンズフレア側で実際にこれで黒画面になった。
*
* 重み合計で正規化するのも必須で、しないと1パスあたり最大7倍のゲインが
* 掛かって4パスで飽和する。明るさを決めるのはカーネルのゲインではなく
* intensity_。
*/
float3 AnamorphicStreakGather(Texture2D<float4> source, float2 uv, float texel_width, float spacing, float attenuation, float length_scale)
{
	float safe_attenuation = clamp(attenuation, 0.5, 0.999);

	float3 sum = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	float weight_sum = 1.0;

	[unroll]
	for (uint tap = 1; tap <= 3; tap++)
	{
		float texel_distance = spacing * float(tap);
		float weight = pow(safe_attenuation, texel_distance);
		float offset = texel_width * texel_distance * length_scale;

		sum += source.SampleLevel(sampler_linear_clamp, uv + float2(offset, 0.0), 0).rgb * weight;
		sum += source.SampleLevel(sampler_linear_clamp, uv - float2(offset, 0.0), 0).rgb * weight;
		weight_sum += weight * 2.0;
	}


/**
* [EN]
* Shared body for Blur1..4: reads source and writes the streak to destination.
* The passes alternate between AnamorphicFlare.Ping and AnamorphicFlare.Pong,
* each pass a Stage of its own.
*
* ---------------------------------------------------------------------
*
* [JP]
* Blur1..4 の共通本体。source を読み、ストリークを destination へ書く。
* パスは AnamorphicFlare.Ping と AnamorphicFlare.Pong を交互に読み書きし、
* 1 パスが 1 つの Stage。
*/
void AnamorphicBlurPass(uint3 dtid, float spacing, Texture2D<float4> source, RWTexture2D<float4> destination, ConstantBuffer<AnamorphicFlareConstantBuffer> constants)
{
	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float texel_width = 1.0 / float(width);
	float attenuation = constants.attenuation_;
	float length_scale = constants.streak_length_ * 4.0;

	destination[dtid.xy] = float4(AnamorphicStreakGather(source, uv, texel_width, spacing, attenuation, length_scale), 1.0);
}

struct AnamorphicFlarePrefilterBindings
{
	uint bokeh_color_;
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_ping_;
};
ConstantBuffer<AnamorphicFlarePrefilterBindings> prefilter_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Prefilter(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[prefilter_bindings.anamorphic_flare_ping_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[prefilter_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[prefilter_bindings.bokeh_color_];
	uint source_width, source_height;
	source.GetDimensions(source_width, source_height);
	float2 source_texel = 1.0 / float2(source_width, source_height);

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float threshold = constants.threshold_;

	float3 result = float3(0.0, 0.0, 0.0);

	[unroll]
	for (uint row = 0; row < 2; row++)
	{
		float vertical_offset = (float(row) * 2.0 - 1.0) * source_texel.y;

		[unroll]
		for (uint column = 0; column < 4; column++)
		{
			float horizontal_offset = (float(column) * 2.0 - 3.0) * source_texel.x;
			float2 tap_uv = uv + float2(horizontal_offset, vertical_offset);

			result += AnamorphicBrightPass(source.SampleLevel(sampler_linear_clamp, tap_uv, 0).rgb, threshold);
		}
	}

	destination[dtid.xy] = float4(result * 0.125, 1.0);
}

struct AnamorphicFlareBlur1Bindings
{
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_ping_;
	uint anamorphic_flare_pong_;
};
ConstantBuffer<AnamorphicFlareBlur1Bindings> blur1_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Blur1(uint3 dtid : SV_DispatchThreadID)
{
	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[blur1_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[blur1_bindings.anamorphic_flare_ping_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[blur1_bindings.anamorphic_flare_pong_];
	AnamorphicBlurPass(dtid, 1.0, source, destination, constants);
}

struct AnamorphicFlareBlur2Bindings
{
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_pong_;
	uint anamorphic_flare_ping_;
};
ConstantBuffer<AnamorphicFlareBlur2Bindings> blur2_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Blur2(uint3 dtid : SV_DispatchThreadID)
{
	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[blur2_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[blur2_bindings.anamorphic_flare_pong_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[blur2_bindings.anamorphic_flare_ping_];
	AnamorphicBlurPass(dtid, 4.0, source, destination, constants);
}

struct AnamorphicFlareBlur3Bindings
{
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_ping_;
	uint anamorphic_flare_pong_;
};
ConstantBuffer<AnamorphicFlareBlur3Bindings> blur3_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Blur3(uint3 dtid : SV_DispatchThreadID)
{
	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[blur3_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[blur3_bindings.anamorphic_flare_ping_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[blur3_bindings.anamorphic_flare_pong_];
	AnamorphicBlurPass(dtid, 16.0, source, destination, constants);
}

struct AnamorphicFlareBlur4Bindings
{
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_pong_;
	uint anamorphic_flare_ping_;
};
ConstantBuffer<AnamorphicFlareBlur4Bindings> blur4_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Blur4(uint3 dtid : SV_DispatchThreadID)
{
	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[blur4_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[blur4_bindings.anamorphic_flare_pong_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[blur4_bindings.anamorphic_flare_ping_];
	AnamorphicBlurPass(dtid, 64.0, source, destination, constants);
}

struct AnamorphicFlareComposeBindings
{
	uint anamorphic_flare_constants_;
	uint anamorphic_flare_ping_;
	uint anamorphic_flare_radiance_;
};
ConstantBuffer<AnamorphicFlareComposeBindings> compose_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Compose(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> output = ResourceDescriptorHeap[compose_bindings.anamorphic_flare_radiance_];

	uint width, height;
	output.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<AnamorphicFlareConstantBuffer> constants = ResourceDescriptorHeap[compose_bindings.anamorphic_flare_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[compose_bindings.anamorphic_flare_ping_];

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);
	float intensity = constants.intensity_;
	float3 tint = constants.tint_.rgb;

	float3 streak = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;

	output[dtid.xy] = float4(streak * tint * intensity, 1.0);
}

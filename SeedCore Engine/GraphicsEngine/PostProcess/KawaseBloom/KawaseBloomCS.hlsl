#include <GraphicsEngine/Shader/Scene.hlsli>

struct KawaseBloomConstantBuffer
{
	float threshold_;
	float soft_knee_;
	float filter_radius_;
	float kawase_bloom_padding_0_;
};

/**
* [EN]
* Reference:
* - https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
* - https://community.arm.com/cfs-file/__key/communityserver-blogs-components-weblogfiles/00-00-00-20-66/siggraph2015_2D00_mmg_2D00_marius_2D00_slides.pdf
*
* Bloom: the broad glow bright parts of the scene bleed into their
* surroundings. Built as a progressive downsample chain followed by a
* progressive additive upsample chain over 6 levels (level0 = half the
* native resolution, each level half of the one before) - the Kawase-lineage
* structure, using the tap patterns from Jorge Jimenez's "Next Generation
* Post Processing in Call of Duty: Advanced Warfare": a 13-tap downsample
* and a 3x3 tent upsample.
*
* Pass order is Prefilter -> Downsample x5 -> Upsample x5, so level0
* (KawaseBloom.Radiance) ends up holding the final result that
* PostProcessCS.hlsl samples and adds into the HDR color before exposure.
* Every level is its own render graph resource and every iteration its own
* graph pass, binding the levels it reads and writes as source and
* destination, so the graph places the barrier between a level being
* written and being read.
*
* The Karis average in Prefilter is the reason this uses the COD
* tap patterns rather than the cheaper 5-tap/8-tap dual-Kawase filter: with
* ray-traced HDR input a single blown-out subpixel is easy to produce, and
* averaging it naively makes it grow into a large flickering block as it
* propagates up the chain. Weighting each of the four contributing groups by
* 1/(1+luma) before averaging suppresses exactly that.
*
* ---------------------------------------------------------------------
*
* [JP]
* ブルーム: シーンの明るい部分が周囲へにじみ出る広いグロー。6レベル
* (level0 がネイティブ解像度の1/2、以降それぞれ半分ずつ)にわたる段階的な
* ダウンサンプルチェーンと、それに続く段階的な加算アップサンプルチェーン
* として構築する — 構造は Kawase 系列で、タップパターンは Jorge Jimenez の
* 「Next Generation Post Processing in Call of Duty: Advanced Warfare」の
* もの(13タップダウンサンプルと3x3テントアップサンプル)を使う。
*
* パスの順序は Prefilter → Downsample ×5 → Upsample ×5 で、
* 最終結果は level0（KawaseBloom.Radiance）に入る — PostProcessCS.hlsl が
* それをサンプルして露出適用前のHDRカラーへ加算する。各レベルはそれぞれ
* レンダーグラフのリソースで、1 回の繰り返しが 1 つのグラフのパスとして
* 読み書きするレベルを source と destination にバインドするので、レベルを
* 書いてから読むまでのバリアはグラフが置く。
*
* Prefilter の Karis 平均が、より安価な5タップ/8タップの
* dual-Kawase ではなくCOD版のタップパターンを採る理由そのもの:
* レイトレHDR入力では1画素だけ飛び抜けて明るい点が簡単に発生し、
* それを素直に平均するとチェーンを昇るにつれて巨大なちらつくブロックに
* 育ってしまう。寄与する4グループそれぞれを平均前に 1/(1+luma) で
* 重み付けすると、それがちょうど抑えられる。
*/

float Luminance(float3 color)
{
	return dot(color, float3(0.2126, 0.7152, 0.0722));
}

/**
* [EN]
* Karis average weight. Brighter pixels get a smaller weight, so a single
* extremely bright pixel (a firefly) does not dominate the averaged result.
*
* ---------------------------------------------------------------------
*
* [JP]
* Karis平均の重み。明るい画素ほど小さい重みになるので、1画素だけ極端に
* 明るい点(ファイアフライ)が平均結果を支配しなくなる。
*/
float KarisWeight(float3 color)
{
	return 1.0 / (1.0 + Luminance(color));
}

/**
* [EN]
* Screening before scene color is ingested. Bloom is the first effect in the
* chain to touch the scene color, so this is the entry point for non-finite
* values. Worse, since the Karis weight is "brighter = smaller weight", a
* color of +Inf gets a weight of 0, and color * weight produces
* Inf * 0 = NaN - the very mechanism meant to suppress fireflies becomes a
* NaN source itself, a counter-intuitive path.
*
* Any NaN born here spreads as it climbs the downsample chain, and also
* reaches lens flare and anamorphic flare, which read the same bright-pass
* buffer - so a single broken pixel shows up as black in several unrelated
* places on screen, each shaped by that effect's own kernel. Separately from
* fixing it at the source (lighting), it is always folded here too, at the
* ingestion point.
*
* ---------------------------------------------------------------------
*
* [JP]
* シーンカラーを取り込む前の検査。ブルームはチェーンの最初にシーンカラーへ
* 触る効果なので、ここが非有限値の入口になる。しかも Karis 重みは
* 「明るいほど小さい重み」なので、色が +Inf のとき重みは 0 になり、
* color * weight が Inf * 0 = NaN を生む - ファイアフライ抑制の仕組み
* そのものが NaN の発生源になるという、直感に反する経路。
*
* 生まれた NaN はダウンサンプルチェーンを上りながら広がり、同じ明部
* バッファを読むレンズフレアやアナモルフィックフレアへも渡るため、
* 1点の破綻が画面上の複数の無関係な場所へ、各効果のカーネル形状で
* 黒として現れる。値を作った側(ライティング)の対策とは別に、
* 取り込み口でも必ず畳んでおく。
*/
float3 SanitizeSceneColor(float3 color)
{
	bool invalid = any(isnan(color)) || any(isinf(color));
	return invalid ? float3(0, 0, 0) : max(color, 0.0);
}

/**
* [EN]
* Threshold with a soft knee (UE4-style). Cutting hard at threshold_ makes a
* pixel hovering near the threshold flicker in and out frame to frame, so it
* is instead ramped up smoothly with a quadratic curve over the soft_knee_
* width.
*
* ---------------------------------------------------------------------
*
* [JP]
* ソフトニー付きのしきい値(UE4式)。threshold_ で硬く切ると、しきい値
* 付近を行き来する画素がフレームごとに出たり消えたりしてちらつくため、
* soft_knee_ の幅だけ二次曲線で滑らかに立ち上げる。
*/
float3 SoftThreshold(float3 color, float threshold, float soft_knee)
{
	float knee = max(threshold * soft_knee, 1e-4);
	float brightest = max(color.r, max(color.g, color.b));

	float soft = clamp(brightest - threshold + knee, 0.0, 2.0 * knee);
	soft = soft * soft / (4.0 * knee);

	float contribution = max(soft, brightest - threshold) / max(brightest, 1e-4);
	return color * contribution;
}

/**
* [EN]
* COD:AW's 13-tap downsample. Weight is concentrated on the center and the
* inner 4 points, summing to exactly 1.0 (0.125*5 + 0.0625*4 + 0.03125*4).
* texel is the 【source】 texel size.
*
* ---------------------------------------------------------------------
*
* [JP]
* COD:AWの13タップダウンサンプル。中心と内側4点に重みを寄せた形で、
* 合計がちょうど1.0になる(0.125*5 + 0.0625*4 + 0.03125*4)。texel は
* 【ソース側】のテクセルサイズ。
*/
float3 Downsample13Tap(Texture2D<float4> source, float2 uv, float2 texel)
{
	float3 a = source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, 2.0) * texel, 0).rgb;
	float3 b = source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, 2.0) * texel, 0).rgb;
	float3 c = source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, 2.0) * texel, 0).rgb;

	float3 d = source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, 0.0) * texel, 0).rgb;
	float3 e = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	float3 f = source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, 0.0) * texel, 0).rgb;

	float3 g = source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, -2.0) * texel, 0).rgb;
	float3 h = source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, -2.0) * texel, 0).rgb;
	float3 i = source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, -2.0) * texel, 0).rgb;

	float3 j = source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, 1.0) * texel, 0).rgb;
	float3 k = source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, 1.0) * texel, 0).rgb;
	float3 l = source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, -1.0) * texel, 0).rgb;
	float3 m = source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, -1.0) * texel, 0).rgb;

	float3 result = e * 0.125;
	result += (a + c + g + i) * 0.03125;
	result += (b + d + f + h) * 0.0625;
	result += (j + k + l + m) * 0.125;
	return result;
}

/**
* [EN]
* Same 13 taps as Downsample13Tap, but each inner group of 4 points is
* weighted by the Karis weight before averaging. Used only for the first
* stage (where the full-resolution scene color is read) - that is the only
* place fireflies enter, since every later level is already averaged.
*
* ---------------------------------------------------------------------
*
* [JP]
* Downsample13Tap と同じ13タップだが、内側の4点グループごとにKaris重みを
* 掛けてから平均する。初段(フル解像度のシーンカラーを読む所)だけで
* 使う - ファイアフライが入ってくるのはそこだけで、以降のレベルは既に
* 平均済みだから。
*/
float3 Downsample13TapKaris(Texture2D<float4> source, float2 uv, float2 texel)
{
	float3 a = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, 2.0) * texel, 0).rgb);
	float3 b = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, 2.0) * texel, 0).rgb);
	float3 c = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, 2.0) * texel, 0).rgb);

	float3 d = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, 0.0) * texel, 0).rgb);
	float3 e = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv, 0).rgb);
	float3 f = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, 0.0) * texel, 0).rgb);

	float3 g = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-2.0, -2.0) * texel, 0).rgb);
	float3 h = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, -2.0) * texel, 0).rgb);
	float3 i = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(2.0, -2.0) * texel, 0).rgb);

	float3 j = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, 1.0) * texel, 0).rgb);
	float3 k = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, 1.0) * texel, 0).rgb);
	float3 l = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(-1.0, -1.0) * texel, 0).rgb);
	float3 m = SanitizeSceneColor(source.SampleLevel(sampler_linear_clamp, uv + float2(1.0, -1.0) * texel, 0).rgb);

	float3 group0 = (a + b + d + e) * 0.25;
	float3 group1 = (b + c + e + f) * 0.25;
	float3 group2 = (d + e + g + h) * 0.25;
	float3 group3 = (e + f + h + i) * 0.25;
	float3 group4 = (j + k + l + m) * 0.25;

	float weight0 = KarisWeight(group0) * 0.125;
	float weight1 = KarisWeight(group1) * 0.125;
	float weight2 = KarisWeight(group2) * 0.125;
	float weight3 = KarisWeight(group3) * 0.125;
	float weight4 = KarisWeight(group4) * 0.5;

	float weight_sum = weight0 + weight1 + weight2 + weight3 + weight4;
	float3 result = group0 * weight0 + group1 * weight1 + group2 * weight2 + group3 * weight3 + group4 * weight4;
	return result / max(weight_sum, 1e-4);
}

/**
* [EN]
* COD:AW's 3x3 tent upsample (center 4, edges 2, corners 1, sum 16). radius
* is in UV units and uses the same value regardless of level - a lower-
* resolution level has a larger texel, so the same UV radius naturally
* produces a wider bleed.
*
* ---------------------------------------------------------------------
*
* [JP]
* COD:AWの3x3テントアップサンプル(中心4、辺2、角1、合計16)。radius は
* UV単位で、レベルによらず同じ値を使う - 低解像度レベルほど1テクセルが
* 大きいので、同じUV半径でも自然に広い滲みになる。
*/
float3 UpsampleTent(Texture2D<float4> source, float2 uv, float radius)
{
	float3 a = source.SampleLevel(sampler_linear_clamp, uv + float2(-radius, radius), 0).rgb;
	float3 b = source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, radius), 0).rgb;
	float3 c = source.SampleLevel(sampler_linear_clamp, uv + float2(radius, radius), 0).rgb;

	float3 d = source.SampleLevel(sampler_linear_clamp, uv + float2(-radius, 0.0), 0).rgb;
	float3 e = source.SampleLevel(sampler_linear_clamp, uv, 0).rgb;
	float3 f = source.SampleLevel(sampler_linear_clamp, uv + float2(radius, 0.0), 0).rgb;

	float3 g = source.SampleLevel(sampler_linear_clamp, uv + float2(-radius, -radius), 0).rgb;
	float3 h = source.SampleLevel(sampler_linear_clamp, uv + float2(0.0, -radius), 0).rgb;
	float3 i = source.SampleLevel(sampler_linear_clamp, uv + float2(radius, -radius), 0).rgb;

	float3 result = e * 4.0;
	result += (b + d + f + h) * 2.0;
	result += (a + c + g + i);
	return result * (1.0 / 16.0);
}

/**
* [EN]
* Shared body for Downsample. The levels to read and write are passed in by
* the entry point, bound per iteration as source and destination.
*
* ---------------------------------------------------------------------
*
* [JP]
* Downsample の共通本体。読み書きするレベルは、エントリポイントが回ごとの
* source と destination のバインディングから渡す。
*/
void DownsampleLevel(uint3 dtid, Texture2D<float4> source, RWTexture2D<float4> destination)
{
	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	uint source_width, source_height;
	source.GetDimensions(source_width, source_height);
	float2 source_texel = 1.0 / float2(source_width, source_height);

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	destination[dtid.xy] = float4(Downsample13Tap(source, uv, source_texel), 1.0);
}

/**
* [EN]
* Shared body for Upsample. Stretches the level one below (lower
* resolution) with the tent filter and 【adds】 it into the destination level
* (read-modify-write). Because it adds, every level's contribution
* accumulates while climbing the chain, layering a wide bleed and a fine
* bleed together.
*
* ---------------------------------------------------------------------
*
* [JP]
* Upsample の共通本体。1つ下(低解像度)のレベルをテントで引き伸ばし、
* 書き込み先レベルへ【加算】する(read-modify-write)。加算なのでチェーンを
* 上がるにつれて全レベルの寄与が積み上がり、広い滲みと細かい滲みが同時に
* 乗る。
*/
void UpsampleLevel(uint3 dtid, Texture2D<float4> source, RWTexture2D<float4> destination, float radius)
{
	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	float3 result = destination[dtid.xy].rgb + UpsampleTent(source, uv, radius);
	destination[dtid.xy] = float4(result, 1.0);
}

struct KawaseBloomPrefilterBindings
{
	uint bokeh_color_;
	uint kawase_bloom_constants_;
	uint kawase_bloom_radiance_;
};
ConstantBuffer<KawaseBloomPrefilterBindings> prefilter_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Prefilter(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> destination = ResourceDescriptorHeap[prefilter_bindings.kawase_bloom_radiance_];

	uint width, height;
	destination.GetDimensions(width, height);

	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	ConstantBuffer<KawaseBloomConstantBuffer> constants = ResourceDescriptorHeap[prefilter_bindings.kawase_bloom_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[prefilter_bindings.bokeh_color_];

	uint source_width, source_height;
	source.GetDimensions(source_width, source_height);
	float2 source_texel = 1.0 / float2(source_width, source_height);

	float2 uv = (float2(dtid.xy) + 0.5) / float2(width, height);

	float3 result = Downsample13TapKaris(source, uv, source_texel);
	result = SoftThreshold(result, constants.threshold_, constants.soft_knee_);

	destination[dtid.xy] = float4(result, 1.0);
}

struct KawaseBloomDownsampleBindings
{
	uint kawase_bloom_source_;
	uint kawase_bloom_destination_;
};
ConstantBuffer<KawaseBloomDownsampleBindings> downsample_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Downsample(uint3 dtid : SV_DispatchThreadID)
{
	Texture2D<float4> source = ResourceDescriptorHeap[downsample_bindings.kawase_bloom_source_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[downsample_bindings.kawase_bloom_destination_];
	DownsampleLevel(dtid, source, destination);
}

struct KawaseBloomUpsampleBindings
{
	uint kawase_bloom_constants_;
	uint kawase_bloom_source_;
	uint kawase_bloom_destination_;
};
ConstantBuffer<KawaseBloomUpsampleBindings> upsample_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Upsample(uint3 dtid : SV_DispatchThreadID)
{
	ConstantBuffer<KawaseBloomConstantBuffer> constants = ResourceDescriptorHeap[upsample_bindings.kawase_bloom_constants_];
	Texture2D<float4> source = ResourceDescriptorHeap[upsample_bindings.kawase_bloom_source_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[upsample_bindings.kawase_bloom_destination_];
	UpsampleLevel(dtid, source, destination, constants.filter_radius_);
}

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

/**
* [EN]
* Reference:
* - https://john-chapman.github.io/2017/11/05/pseudo-lens-flare.html
* - https://www.iryoku.com/next-generation-post-processing-in-call-of-duty-advanced-warfare/
*
* Aperture diffraction-spike ("starburst") lens flare, built as a
* multi-pass Kawase-style directional blur (the same technique described in
* Call of Duty: Advanced Warfare's next-gen post-processing talk) instead of
* a single-dispatch sparse gather.
*
* The spike pattern follows the physics of an n-bladed iris: each blade
* edge diffracts light perpendicular to itself, and one edge produces a
* single LINE (two opposing arms 180 degrees apart), so an even-bladed iris
* shows n spikes - opposing edges are parallel and their lines coincide -
* while an odd-bladed one shows 2n. That makes the axis count n/2 for even
* n and n for odd n; LensFlareFeature::Gather derives it from
* BokehSettings::bladeCount_ (the same physical iris the bokeh shape comes
* from) into axis_count_, and the axes are spread evenly over half a turn.
* Each axis is blurred independently and bidirectionally across 4 passes
* (Blur, 4 iterations), ping-ponging between that axis's slices of
* LensFlare.Ping and LensFlare.Pong.
*
* Each pass takes 3 taps per side at a texel spacing of 4^(pass-1)
* (1, 4, 16, 64) and weights a tap `d` texels out by
* streak_attenuation_^d - Kawase's a^(b*s) formula. That exponential decay
* IS the spike's falloff: it is what makes an arm taper and fade toward its
* tip. An equal-weight box average over the same taps produces no falloff
* at all, so every arm stays as bright at its tip as at its root and reads
* as a hard solid bar rather than light. The per-pass result is normalized
* by the weight sum, because without that each pass multiplies brightness
* by up to 7 and four passes blow out to a saturated white cross;
* intensity_ is the knob for overall brightness, not the kernel gain.
*
* Because each pass reads the PREVIOUS pass's already-filtered result
* rather than the raw HDR source, the streak comes out continuous even when
* the underlying bright source is large (e.g. a close point light's soft
* specular highlight) - a single-dispatch sparse gather would instead
* reproduce separate copies of that source at each sample position, visible
* as a chain of overlapping circles ("beads on a string").
* The first Blur iteration also performs the bright-pass extraction (from the same HDR
* source PostProcessCS.hlsl reads); Glare reads each axis's final
* (post-pass-6) buffer, applies the color-fringing chromatic_aberration_
* offset and intensity_ once, and writes the summed result directly into
* LensFlare.Radiance, which PostProcessCS.hlsl samples and adds
* into the HDR color before exposure. A final Ghost pass adds the classic
* "ghost chain" - bright-passed copies of the source strung out through
* screen center to the opposite side, fading with distance from screen
* center and with ghost index - on top of the streak result via a single
* gather dispatch that read-modify-writes the same output texture.
*
* ---------------------------------------------------------------------
*
* [JP]
* 絞りの回折スパイク(スターバースト)型レンズフレア。1回のディスパッチで
* 疎にゲザーする方式ではなく、多段階Kawase式ディレクショナルブラー
* (Call of Duty: Advanced Warfareの次世代ポストプロセス講演で知られる手法)
* として構築する。
*
* 棘の並びは羽根n枚の絞りの物理に従う: 各羽根のエッジはそれ自身に垂直な
* 方向へ回折し、1つのエッジが【直線1本】(180°対向する腕2本)を作る。
* よって偶数枚の絞りでは棘がn本にしか見えず(向かい合うエッジが平行で
* 直線が重なるため)、奇数枚では2n本になる。軸数は偶数で n/2、奇数で n。
* LensFlareFeature::Gather が BokehSettings::bladeCount_
* (ボケの形を決めるのと同じ物理的な絞り)から計算して axis_count_ に
* 入れ、軸は半周に等間隔で並ぶ。各軸を独立に、4回(Blur の繰り返し)
* かけて双方向にブラーする。LensFlare.Ping と LensFlare.Pong の
* その軸のスライスの間でピンポンする。
*
* 各パスは片側3タップずつを 4^(パス番号-1) テクセル間隔(1, 4, 16, 64)で
* 取り、d テクセル先のタップの重みを streak_attenuation_^d にする —
* Kawase の a^(b*s) の式そのもの。この指数減衰が【棘の減衰そのもの】で、
* 腕が先端へ向かって細く暗くなるのはこれによる。同じタップを等重みで
* 箱平均すると減衰が一切起きず、どの腕も先端まで根元と同じ明るさの
* 硬いバーになって光に見えない。パスごとの結果は重みの合計で正規化する。
* しないと1パスあたり最大7倍のゲインが掛かり、4パスで飽和した白い十字に
* なる。全体の明るさを決めるのはカーネルのゲインではなく intensity_。
*
* 各パスが生のHDRソースではなく【前パスの既にフィルタ済みの結果】を
* 読むため、元の明るい光源が大きい場合(近距離のポイントライトの柔らかい
* スペキュラハイライトなど)でも連続した線になる — 1回ディスパッチの
* 疎なゲザーは、少数のサンプル位置ごとにその光源の独立したコピーを
* 複製してしまい、重なり合った円の数珠つなぎ(ビーズ状)に見えていた。
* Blur の 1 回目はブライトパス抽出も兼ねる
* (PostProcessCS.hlsl が読むのと同じHDRソースから)。Glare は各軸の
* 最終(パス4後)バッファを読み、色収差(chromatic_aberration_)オフセットと
* intensity_ を1回だけ適用し、合計結果を LensFlare.Radiance(PostProcessCS.hlsl がサンプルして露出適用前のHDRカラーへ
* 加算する)へ直接書き込む。最後に Ghost パスが古典的な「ゴーストチェーン」
* (画面中心を通り抜けて反対側まで連なる光源のブライトパス済みコピー、
* 画面中心からの距離とゴーストのインデックスに応じてフェードする)を
* ストリークの結果へ加算する — 1回のゲザーディスパッチで同じ出力
* テクスチャを読み書きする。
*/

/**
* [EN]
* Soft threshold weighting used before the bright-pass extraction below (the
* Downsample pass). Fades to 0 as luminance approaches threshold from above,
* rather than a hard cutoff.
*
* ---------------------------------------------------------------------
*
* [JP]
* 下のブライトパス抽出(Downsampleパス)前に使うソフトなしきい値重み。
* 硬い切り捨てではなく、輝度が threshold へ上から近づくにつれて0へ
* フェードする。
*/
float3 BrightPass(float3 color, float threshold)
{
	float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));
	float weight = max(luminance - threshold, 0.0) / max(luminance, 0.0001);
	return color * weight;
}

/**
* [EN]
* Direction of a spike's axis. One axis is "a single line = two opposing
* arms", so the axes are spread evenly over half a turn (pi). This follows
* directly from an iris's diffraction physics: each blade edge diffracts
* perpendicular to itself, so an n-bladed iris shows n spikes when n is
* even, 2n when n is odd - axis counts of n/2 and n respectively. The C++
* side (LensFlareFeature::Gather) computes axis_count_ from
* BokehSettings::bladeCount_. A 6-bladed iris gives 3 axes 60 degrees apart,
* matching the value that used to be hardcoded.
*
* ---------------------------------------------------------------------
*
* [JP]
* 棘の軸の向き。軸1本が「直線1本 = 対向する腕2本」なので、軸を半周(π)に
* 等間隔で並べる。これは絞りの回折そのもので、羽根の各エッジがそれ自身に
* 垂直な方向へ回折し、羽根n枚なら偶数でn本、奇数で2n本の棘になる - 軸数は
* それぞれ n/2、n。軸数はC++側(LensFlareFeature::Gather)が
* BokehSettings::bladeCount_ から計算して axis_count_ に入れ、羽根6枚なら
* 3軸60°間隔で、これは以前ハードコードされていた値と一致する。
*/
float2 AxisDirection(uint axis, float angle_offset, uint axis_count)
{
	float angle = angle_offset + (3.14159265 / float(axis_count)) * float(axis);
	return float2(cos(angle), sin(angle));
}

/**
* [EN]
* Per-spike variation coefficient. Built deterministically from the axis
* index, so it stays stable frame to frame instead of flickering over time.
* With variation at 0 this always returns 1.0, giving an ideally symmetric
* star. This is stylization, not physics - a perfect iris is fully
* symmetric; a real one is uneven because of manufacturing tolerance and
* assembly misalignment of the blades.
*
* ---------------------------------------------------------------------
*
* [JP]
* 棘ごとのばらつき係数。軸番号から決定論的に作るのでフレーム間で安定し、
* 時間的なちらつきにならない。variation が0なら常に1.0を返して理想的な
* 対称の星になる。物理ではなく演出 - 理想的な絞りは完全に対称で、実物が
* 不揃いなのは羽根の製造誤差と組み付けのずれ。
*/
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

/**
* [EN]
* The core Kawase light-streak kernel. Takes the center tap plus 3 taps on
* each side along the axis (7 taps total), weighted by
* attenuation^(texel distance) - plain exponential decay with distance. This
* is why a spike tapers and darkens toward its tip; an equal-weight box
* average would produce no decay at all, leaving a hard bar equally bright
* from root to tip (which is what actually happened before this).
*
* spacing is the texel interval, quadrupling per pass (1, 4, 16, 64). Since
* the weight is exponential in distance, a pass with wider spacing has its
* side taps' contribution shrink much faster, building a long streak in few
* passes while the tip still fades out naturally.
*
* Normalized by dividing by the weight sum. Without this each pass would
* apply up to 7x gain, reaching thousands of times over 4 passes and
* saturating. Overall brightness is intensity_'s job instead.
*
* ---------------------------------------------------------------------
*
* [JP]
* Kawaseのライトストリークのカーネル本体。中心1タップ + 軸方向の両側3
* タップずつの計7タップを取り、重みは attenuation^(テクセル距離) - 距離に
* 対する指数減衰そのもの。これが棘が先端へ向かって細く暗くなる理由で、
* 等重みの箱型平均だと減衰が一切起きず、根元から先端まで同じ明るさの
* 硬いバーになってしまう(実際そうなっていた)。
*
* spacing はパスごとに4倍になるテクセル間隔(1, 4, 16, 64)。重みが距離の
* 指数なので、間隔が広いパスほど側タップの寄与が急速に小さくなり、少ない
* パス数で長い筋を作りつつ先端が自然に消える。
*
* 重みの合計で割って正規化する。割らないとパスごとに最大7倍のゲインが
* 掛かり、4パスで数千倍になって飽和する。明るさの調整は intensity_ の役目。
*/
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

/**
* [EN]
* Shared body for Blur: for every axis, applies the streak kernel to
* that axis's slice of source and writes it to the same slice of destination.
* The first iteration reads LensFlare.Bright (one slice, shared by every axis); the
* later iterations alternate between LensFlare.Ping and LensFlare.Pong, each
* iteration a graph pass of its own.
*
* ---------------------------------------------------------------------
*
* [JP]
* Blur の共通本体。軸ごとに、source のその軸のスライスへストリークの
* カーネルを掛け、destination の同じスライスへ書く。1 回目は
* LensFlare.Bright（全軸で共有する 1 スライス）を読み、以降の回は
* LensFlare.Ping と LensFlare.Pong を交互に読み書きする。1 回が 1 つの
* グラフのパス。
*/
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

/**
* [EN]
* Per-pass texel spacing is 4^(pass number - 1) = 1, 4, 16, 64 - the
* multiplier Kawase's light streak defines. With 3 taps per side per pass,
* the reach totals 3 + 12 + 48 + 192 = 255 texels (over half the screen,
* since the working resolution is 1/4 native). The 4x multiplier needs fewer
* passes than a 2x step would, which is what let this drop from 6 passes to
* 4.
*
* ---------------------------------------------------------------------
*
* [JP]
* パスごとのテクセル間隔は 4^(パス番号-1) = 1, 4, 16, 64。Kawase の
* ライトストリークが定める倍率で、1パスあたり片側3タップなので到達距離は
* 3 + 12 + 48 + 192 = 255 テクセル(作業解像度がネイティブの1/4なので画面の
* 半分以上)に届く。倍率が4なぶん2倍刻みより少ないパス数で済み、以前の6
* パスから4パスへ減らせた。
*/
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

/**
* [EN]
* Samples color shifted radially (chromatic aberration). Reading R/G/B each
* with a different offset along direction produces the rainbow fringe at the
* edge of a ghost or halo.
*
* ---------------------------------------------------------------------
*
* [JP]
* 半径方向に色をずらしてサンプルする(色収差)。R/G/Bをそれぞれ direction
* に沿った別のオフセットで読むことで、ゴーストとハローの縁に虹色の
* フリンジが出る。
*/
float3 SampleDistorted(Texture2DArray<float4> source, float2 uv, float2 direction, float3 distortion)
{
	float r = source.SampleLevel(sampler_linear_wrap, float3(uv + direction * distortion.r, 0.0), 0).r;
	float g = source.SampleLevel(sampler_linear_wrap, float3(uv + direction * distortion.g, 0.0), 0).g;
	float b = source.SampleLevel(sampler_linear_wrap, float3(uv + direction * distortion.b, 0.0), 0).b;
	return float3(r, g, b);
}

/**
* [EN]
* Reference:
* - https://john-chapman.github.io/2017/11/05/pseudo-lens-flare.html
*   (John Chapman, "Pseudo Lens Flare" - the ghost-chain + halo construction
*   the Ghost pass below follows.)
*
* Lens color gradient looked up by normalized radius from screen center. The
* canonical implementation samples a 1D LUT texture, but this substitutes a
* phase-shifted cosine palette instead to avoid adding an asset - each ghost
* changing color is the single biggest factor in a lens flare reading as
* "natural", a very different look from adding everything in plain white.
*
* ---------------------------------------------------------------------
*
* [JP]
* 画面中心からの正規化半径で引くレンズカラーのグラデーション。本来の実装は
* 1DのLUTテクスチャを引くが、アセットを増やさずに済むよう位相をずらした
* コサインパレットで代用する - ゴーストごとに色が変わるのがレンズフレアが
* 「自然」に見える一番の要因なので、白のまま加算するのとは見た目が大きく
* 変わる。
*/
float3 LensColor(float radius01)
{
	return 0.6 + 0.4 * cos(6.28318530718 * (radius01 + float3(0.0, 0.33, 0.67)));
}

/**
* [EN]
* Ghost: a chain of bokeh circles (ghosts) strung out to the opposite side
* across screen center, plus a halo (ring) around them. Follows John
* Chapman's "Pseudo Lens Flare" construction. Completes in a single
* dispatch, no multi-pass ping-pong.
*
* The key point is 【flipping UV before marching toward center】: the output
* pixel's position is point-reflected as uv = 1 - uv, and a vector toward
* center is then stepped from there, so a light source casts its ghosts on
* the OPPOSITE side - matching the arrangement that actually happens in a
* real lens. Without the flip, ghosts would line up on the same side as the
* light source and immediately fly off-screen.
*
* Samples use sampler_linear_wrap, and the weight calculation also uses the
* frac()'d position (matching the weight to the actual post-wrap sample
* position). A clamp sampler would instead stretch the edge pixels for a
* ghost that goes off-screen.
*
* The input is the bright-passed buffer Downsample wrote. This adds onto the
* streak result Glare wrote (a read-modify-write; Ghost is a later step of the same
* Feature, so the graph places the barrier).
*
* ---------------------------------------------------------------------
*
* [JP]
* Ghost: 画面中心を挟んで反対側へ連なるボケ円のチェーン(ゴースト)と、
* その周りのハロー(輪)。John Chapman の "Pseudo Lens Flare" の構成を
* 踏襲する。1回のディスパッチで完結し、多段階ピンポンは使わない。
*
* 要点は【UVを反転してから中心へ向かって進む】こと: 出力画素の位置を
* uv = 1 - uv と点対称に反転した上で中心へ向かうベクトルを刻むため、光源は
* その反対側にゴーストを落とす - 実際のレンズで起きるのと同じ並び方になる。
* 反転しないと、ゴーストは光源と同じ側へ、しかも画面外へすぐ飛び出す向きに
* 並んでしまう。
*
* サンプルは sampler_linear_wrap で読み、重みの計算にも frac() を掛けた
* 位置を使う(ラップ後の実際のサンプル位置と重みを一致させる)。クランプ
* サンプラだと画面外へ出たゴーストが端の画素を引き伸ばす。
*
* 入力は Downsample が書いたブライトパス済みバッファ。Glare が書いた
* ストリーク結果へ加算する(読み書き。Ghost は同じ Feature の後のステップ
* なので、バリアはグラフが置く)。
*/
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

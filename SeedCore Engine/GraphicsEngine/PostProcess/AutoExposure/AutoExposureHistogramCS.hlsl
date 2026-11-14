#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposure.hlsli>

struct AutoExposureHistogramBindings
{
	uint upscale_color_;
	uint auto_exposure_constants_;
	uint auto_exposure_histogram_;
};
ConstantBuffer<AutoExposureHistogramBindings> bindings : register(b0, space1);


/**
* [EN]
* Reference:
* - https://bruop.github.io/exposure/
* - https://alextardif.com/HistogramLuminance.html
*
* Builds a 256-bin log-luminance histogram of the HDR scene color
* (AutoExposureAverageCS.hlsl reduces it into a smoothed exposure value next
* frame's tone map reads). Bin 0 is reserved for near-black pixels and is
* excluded from the average by the reduce pass - without that, letterboxing,
* a clear background color, or a mostly-empty skybox would drag the average
* toward black and the image would perpetually over-expose trying to
* compensate.
*
* [numthreads(16,16,1)] = exactly 256 threads per group, chosen so each thread
* owns exactly one histogram bin during the group-shared -> global reduction
* step, avoiding a second indexing scheme. The reduction itself (zero the
* local histogram, atomically bin every pixel in the group into groupshared
* memory, then each thread adds its one bin to the global buffer) trades 256
* global atomics per group for what would otherwise be one global atomic per
* pixel - the standard cheap trick for histogram building on the GPU.
*
* AutoExposure.Histogram starts each frame at 0 because
* AutoExposureAverageCS.hlsl clears every bin after reading it; this shader
* only adds.
*
* [JP]
* HDR シーンカラーの 256 ビン log 輝度ヒストグラムを構築する
* (AutoExposureAverageCS.hlsl がこれを縮約して、次フレームのトーンマップが
* 読む平滑化露出値にする)。ビン0は「ほぼ黒」専用で、縮約パスが平均から
* 除外する — 除外しないと、レターボックスや背景クリアカラー、ほぼ空の
* スカイボックスが平均を黒側へ引きずり、露出が永遠にオーバーになろうとして
* 破綻する。
*
* [numthreads(16,16,1)] = グループあたりちょうど256スレッド。group-shared→
* global の縮約段階で各スレッドがヒストグラムのビンを1つずつ受け持てるよう、
* 別のインデックス方式を持ち込まずに済むようこの数にしている。縮約自体
* (ローカルヒストグラムをゼロ初期化→グループ内の全ピクセルを groupshared へ
* 原子加算→各スレッドが自分のビン1つをグローバルバッファへ加算)は、ピクセル
* ごとに1回のグローバル原子加算が要るところを、グループごと256回の
* グローバル原子加算で済ませる、GPU ヒストグラム構築の定番の軽量化。
*
* AutoExposure.Histogram は、AutoExposureAverageCS.hlsl が読んだ後に全ビンを
* 0 に戻すので、毎フレーム 0 から始まる。このシェーダは加算のみ行う。
*/

groupshared uint local_histogram[256];

[numthreads(16, 16, 1)]
void main(uint3 dtid : SV_DispatchThreadID, uint group_index : SV_GroupIndex)
{
	ConstantBuffer<AutoExposureConstantBuffer> constants = ResourceDescriptorHeap[bindings.auto_exposure_constants_];
	local_histogram[group_index] = 0;
	GroupMemoryBarrierWithGroupSync();

	Texture2D<float4> source = ResourceDescriptorHeap[bindings.upscale_color_];

	uint width, height;
	source.GetDimensions(width, height);

	if (dtid.x < width && dtid.y < height)
	{
		float3 color = source.Load(int3(dtid.xy, 0)).rgb;
		float luminance = dot(color, float3(0.2126, 0.7152, 0.0722));

		uint bin;
		if (luminance < 0.0001)
		{
			bin = 0;
		}
		else
		{
			float min_log = constants.min_log_luminance_;
			float max_log = constants.max_log_luminance_;
			float normalized_log_luminance = saturate((log2(luminance) - min_log) / max(max_log - min_log, 0.0001));
			bin = uint(normalized_log_luminance * 254.0) + 1;
		}

		InterlockedAdd(local_histogram[bin], 1);
	}

	GroupMemoryBarrierWithGroupSync();

	uint count = local_histogram[group_index];
	if (count > 0)
	{
		RWStructuredBuffer<uint> histogram = ResourceDescriptorHeap[bindings.auto_exposure_histogram_];
		InterlockedAdd(histogram[group_index], count);
	}
}

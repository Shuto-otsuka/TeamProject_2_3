#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposure.hlsli>

struct AutoExposureHistogramBindings
{
	uint upscale_color_;
	uint auto_exposure_constants_;
	uint auto_exposure_histogram_;
};
ConstantBuffer<AutoExposureHistogramBindings> bindings : register(b0, space1);

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

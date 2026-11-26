#include <GraphicsEngine/PostProcess/AutoExposure/AutoExposure.hlsli>

struct AutoExposureAverageBindings
{
	uint auto_exposure_constants_;
	uint auto_exposure_histogram_;
	uint auto_exposure_exposure_;
};
ConstantBuffer<AutoExposureAverageBindings> bindings : register(b0, space1);

[numthreads(1, 1, 1)]
void main()
{
	ConstantBuffer<AutoExposureConstantBuffer> constants = ResourceDescriptorHeap[bindings.auto_exposure_constants_];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	RWStructuredBuffer<uint> histogram = ResourceDescriptorHeap[bindings.auto_exposure_histogram_];

	histogram[0] = 0;

	float weighted_log_sum = 0.0;
	float total_count = 0.0;

	for (uint bin = 1; bin < 256; bin++)
	{
		float count = float(histogram[bin]);
		histogram[bin] = 0;
		weighted_log_sum += count * (float(bin - 1) / 254.0);
		total_count += count;
	}

	float min_log = constants.min_log_luminance_;
	float max_log = constants.max_log_luminance_;

	float avg_log_luminance = total_count > 0.0 ? (weighted_log_sum / total_count) * (max_log - min_log) + min_log : min_log;

	float key_value = constants.key_value_;
	float target_ev = clamp(log2(max(key_value, 0.0001)) - avg_log_luminance, -8.0, 8.0);

	RWStructuredBuffer<float> exposure = ResourceDescriptorHeap[bindings.auto_exposure_exposure_];
	float smoothed_ev = exposure[0];

	bool brightening = target_ev < smoothed_ev;
	float speed = brightening ? constants.adapt_speed_to_bright_ : constants.adapt_speed_to_dark_;
	float blend = 1.0 - exp(-scene.delta_time_ * max(speed, 0.0001));

	smoothed_ev += (target_ev - smoothed_ev) * blend;
	exposure[0] = smoothed_ev;
}

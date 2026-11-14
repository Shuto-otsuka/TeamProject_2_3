#ifndef __AUTO_EXPOSURE_HLSL__
#define __AUTO_EXPOSURE_HLSL__

#include <GraphicsEngine/Shader/Scene.hlsli>

struct AutoExposureConstantBuffer
{
	float min_log_luminance_;
	float max_log_luminance_;
	float key_value_;
	float adapt_speed_to_bright_;
	float adapt_speed_to_dark_;
	float3 auto_exposure_padding_0_;
};

#endif // __AUTO_EXPOSURE_HLSL__

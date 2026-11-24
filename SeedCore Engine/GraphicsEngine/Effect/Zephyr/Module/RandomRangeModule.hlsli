// @node RandomRange
// @label Random Range
// @category 数式
// @output float3 Value
// @input float3 min = (0.0, 0.0, 0.0) constant
// @input float3 max = (1.0, 1.0, 1.0) constant

#ifndef __RANDOM_RANGE_MODULE_HLSL__
#define __RANDOM_RANGE_MODULE_HLSL__

#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct RandomRangeInputs
{
    float3 min_;
    float3 max_;
};

float3 RandomRange(uint seed, RandomRangeInputs inputs)
{
    float3 random = float3(WangHash(seed), WangHash(seed ^ 0x68bc21ebu), WangHash(seed ^ 0xb5297a4du)) / 4294967295.0;
    return lerp(inputs.min_, inputs.max_, random);
}

#endif // __RANDOM_RANGE_MODULE_HLSL__
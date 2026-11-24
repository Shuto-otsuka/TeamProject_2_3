// @node Point
// @category 発生形状
// @output shape

#ifndef __POINT_MODULE_HLSL__
#define __POINT_MODULE_HLSL__

#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct PointInputs
{
    /// No Code
};

ParticleShape Point(uint seed, PointInputs inputs)
{
    float cos_theta = WangHash(seed) / 4294967295.0 * 2.0 - 1.0;
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    float phi = WangHash(seed ^ 0x68bc21ebu) / 4294967295.0 * 6.2831853;

    ParticleShape shape;
    shape.position_ = float3(0.0, 0.0, 0.0);
    shape.normal_ = float3(sin_theta * cos(phi), cos_theta, sin_theta * sin(phi));
    return shape;
}

#endif // __POINT_MODULE_HLSL__
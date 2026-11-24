// @node Cone
// @category 発生形状
// @output shape
// @input float angle = 30.0
// @input float radius = 0.0
// @input float length = 1.0

#ifndef __CONE_MODULE_HLSL__
#define __CONE_MODULE_HLSL__

#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct ConeInputs
{
    float angle_;
    float radius_;
    float length_;
};

ParticleShape Cone(uint seed, ConeInputs inputs)
{
    float cos_theta = lerp(cos(radians(inputs.angle_)), 1.0, WangHash(seed) / 4294967295.0);
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    float phi = WangHash(seed ^ 0x68bc21ebu) / 4294967295.0 * 6.2831853;
    float3 direction = float3(sin_theta * cos(phi), cos_theta, sin_theta * sin(phi));

    float base_radius = inputs.radius_ * sqrt(WangHash(seed ^ 0xb5297a4du) / 4294967295.0);
    float base_phi = WangHash(seed ^ 0x1b56c4e9u) / 4294967295.0 * 6.2831853;
    float3 base = float3(cos(base_phi) * base_radius, 0.0, sin(base_phi) * base_radius);

    ParticleShape shape;
    shape.position_ = base + direction * inputs.length_ * (WangHash(seed ^ 0x2f8a1c33u) / 4294967295.0);
    shape.normal_ = direction;
    return shape;
}

#endif // __CONE_MODULE_HLSL__
// @node Sphere
// @category 発生形状
// @output shape
// @input float radius = 1.0
// @input float thickness = 1.0

#ifndef __SPHERE_MODULE_HLSL__
#define __SPHERE_MODULE_HLSL__

#include <GraphicsEngine/Shader/Noise.hlsli>
#include <GraphicsEngine/Effect/Zephyr/Particle.hlsli>

struct SphereInputs
{
    float radius_;
    float thickness_;
};

ParticleShape Sphere(uint seed, SphereInputs inputs)
{
    float cos_theta = WangHash(seed) / 4294967295.0 * 2.0 - 1.0;
    float sin_theta = sqrt(1.0 - cos_theta * cos_theta);
    float phi = WangHash(seed ^ 0x68bc21ebu) / 4294967295.0 * 6.2831853;
    float3 direction = float3(sin_theta * cos(phi), cos_theta, sin_theta * sin(phi));

    float inner = 1.0 - saturate(inputs.thickness_);
    float volume = lerp(inner * inner * inner, 1.0, WangHash(seed ^ 0xb5297a4du) / 4294967295.0);

    ParticleShape shape;
    shape.position_ = direction * inputs.radius_ * pow(volume, 1.0 / 3.0);
    shape.normal_ = direction;
    return shape;
}

#endif // __SPHERE_MODULE_HLSL__
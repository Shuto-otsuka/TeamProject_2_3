#include <GraphicsEngine/Sky/Sky.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct SkyIrradianceBindings
{
    uint sky_cubemap_;
    uint sky_diffuse_irradiance_;
};
ConstantBuffer<SkyIrradianceBindings> bindings : register(b0, space1);

/**
* [EN]
* Convolves the environment cube into a diffuse-irradiance cube by integrating
* the cosine-weighted hemisphere around each output direction. One thread per
* irradiance cube texel.
*
* [JP]
* environment キューブを拡散 irradiance キューブへ畳み込む。各出力方向まわりの
* コサイン重み半球を積分する。irradiance キューブテクセル 1 つにつき 1 スレッド。
*/
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    RWTexture2DArray<float4> destination = ResourceDescriptorHeap[bindings.sky_diffuse_irradiance_];
    uint width;
    uint height;
    uint element_count;
    destination.GetDimensions(width, height, element_count);
    if (id.x >= width || id.y >= height)
    {
        return;
    }

    float2 uv = (float2(id.xy) + 0.5) / float(width) * 2.0 - 1.0;
    float3 normal = CubeFaceDirection(id.z, uv);

    TextureCube<float4> environment = ResourceDescriptorHeap[bindings.sky_cubemap_];

    float3 up = abs(normal.y) < 0.999 ? float3(0.0, 1.0, 0.0) : float3(1.0, 0.0, 0.0);
    float3 right = normalize(cross(up, normal));
    up = cross(normal, right);

    float3 irradiance = float3(0.0, 0.0, 0.0);
    float sample_count = 0.0;

    const float delta_phi = 0.025;
    const float delta_theta = 0.025;

    for (float phi = 0.0; phi < 2.0 * SKY_PI; phi += delta_phi)
    {
        for (float theta = 0.0; theta < 0.5 * SKY_PI; theta += delta_theta)
        {
            float3 tangent_sample = cos(phi) * sin(theta) * right + sin(phi) * sin(theta) * up + cos(theta) * normal;
            irradiance += environment.SampleLevel(sampler_linear_clamp, tangent_sample, 0).rgb * cos(theta) * sin(theta);
            sample_count += 1.0;
        }
    }

    irradiance = SKY_PI * irradiance / max(sample_count, 1.0);

    destination[id] = float4(irradiance, 1.0);
}

#include <GraphicsEngine/Sky/Sky.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct SkyBrdfLutBindings
{
    uint sky_brdf_lut_;
};
ConstantBuffer<SkyBrdfLutBindings> bindings : register(b0, space1);

/**
* [EN]
* Integrates the split-sum specular BRDF environment lookup table (the scale
* and bias terms) into an RG texture. Environment-independent, so it is
* generated once and shared by every skymap. One thread per LUT texel; the x
* axis is N.V and the y axis is roughness.
*
* [JP]
* 分割和の鏡面 BRDF 環境ルックアップテーブル（scale と bias 項）を RG
* テクスチャへ積分する。環境に依存しないため 1 回だけ生成して全スカイマップで
* 共有する。LUT テクセル 1 つにつき 1 スレッド。x 軸が N.V、y 軸が roughness。
*/

float GeometrySchlickGgxImageBasedLighting(float normal_dot_direction, float roughness)
{
    float alpha = roughness;
    float k = (alpha * alpha) / 2.0;
    return normal_dot_direction / (normal_dot_direction * (1.0 - k) + k);
}

float GeometrySmith(float normal_dot_view, float normal_dot_light, float roughness)
{
    return GeometrySchlickGgxImageBasedLighting(normal_dot_view, roughness) * GeometrySchlickGgxImageBasedLighting(normal_dot_light, roughness);
}

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    RWTexture2D<float2> destination = ResourceDescriptorHeap[bindings.sky_brdf_lut_];
    uint width;
    uint height;
    destination.GetDimensions(width, height);
    if (id.x >= width || id.y >= height)
    {
        return;
    }

    float normal_dot_view = (float(id.x) + 0.5) / float(width);
    float roughness = (float(id.y) + 0.5) / float(height);

    float3 view = float3(sqrt(1.0 - normal_dot_view * normal_dot_view), 0.0, normal_dot_view);
    float3 normal = float3(0.0, 0.0, 1.0);

    float scale = 0.0;
    float bias = 0.0;

    const uint sample_count = 1024;
    for (uint sample_index = 0; sample_index < sample_count; sample_index++)
    {
        float2 xi = Hammersley(sample_index, sample_count);
        float3 half_vector = ImportanceSampleGgx(xi, normal, roughness);
        float3 light = normalize(2.0 * dot(view, half_vector) * half_vector - view);

        float normal_dot_light = max(light.z, 0.0);
        float normal_dot_half = max(half_vector.z, 0.0);
        float view_dot_half = max(dot(view, half_vector), 0.0);

        if (normal_dot_light > 0.0)
        {
            float geometry = GeometrySmith(normal_dot_view, normal_dot_light, roughness);
            float geometry_visibility = (geometry * view_dot_half) / (normal_dot_half * normal_dot_view);
            float fresnel = pow(1.0 - view_dot_half, 5.0);

            scale += (1.0 - fresnel) * geometry_visibility;
            bias += fresnel * geometry_visibility;
        }
    }

    scale /= float(sample_count);
    bias /= float(sample_count);

    destination[id.xy] = float2(scale, bias);
}

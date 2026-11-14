#include <GraphicsEngine/Sky/Sky.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct SkyPrefilterBindings
{
    uint sky_cubemap_;
    uint sky_specular_;
};
ConstantBuffer<SkyPrefilterBindings> bindings : register(b0, space1);

/**
* [EN]
* Prefilters the environment cube for a single roughness level (one mip of the
* prefiltered specular cube) using GGX importance sampling. One thread per
* destination cube texel. At roughness 0 it copies the environment directly.
* The dispatch constant holds the mip in its low 16 bits and the mip count in
* its high 16 bits; the roughness of a mip is mip / (mip count - 1).
*
* [JP]
* GGX 重点サンプリングで environment キューブを 1 つの roughness レベル
* （プリフィルタ鏡面キューブの 1 ミップ）へプリフィルタする。出力キューブ
* テクセル 1 つにつき 1 スレッド。roughness 0 では environment をそのままコピー。
* ディスパッチの定数は、下位 16 ビットがミップ、上位 16 ビットがミップの数。
* ミップの roughness は ミップ / (ミップの数 - 1)。
*/
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    uint mip = dispatch_bindings.constants_ & 0xFFFF;
    uint mip_count = dispatch_bindings.constants_ >> 16;

    RWTexture2DArray<float4> destination = ResourceDescriptorHeap[bindings.sky_specular_ + mip];
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
    float3 view = normal;

    TextureCube<float4> environment = ResourceDescriptorHeap[bindings.sky_cubemap_];

    float roughness = mip_count <= 1 ? 0.0 : float(mip) / float(mip_count - 1);

    if (roughness < 0.001)
    {
        float3 color = environment.SampleLevel(sampler_linear_clamp, normal, 0).rgb;
        destination[id] = float4(color, 1.0);
        return;
    }

    const uint sample_count = 256;
    float3 prefiltered = float3(0.0, 0.0, 0.0);
    float total_weight = 0.0;

    for (uint sample_index = 0; sample_index < sample_count; sample_index++)
    {
        float2 xi = Hammersley(sample_index, sample_count);
        float3 half_vector = ImportanceSampleGgx(xi, normal, roughness);
        float3 light = normalize(2.0 * dot(view, half_vector) * half_vector - view);

        float normal_dot_light = dot(normal, light);
        if (normal_dot_light > 0.0)
        {
            prefiltered += environment.SampleLevel(sampler_linear_clamp, light, 0).rgb * normal_dot_light;
            total_weight += normal_dot_light;
        }
    }

    prefiltered /= max(total_weight, 0.001);

    destination[id] = float4(prefiltered, 1.0);
}

#include <GraphicsEngine/Sky/Sky.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct SkyGenerateEquirectBindings
{
    uint sky_cubemap_;
};
ConstantBuffer<SkyGenerateEquirectBindings> bindings : register(b0, space1);

/**
* [EN]
* Projects an equirectangular HDR (Texture2D) onto the six faces of the
* environment cube (mip 0). One thread per destination cube texel. Without a
* skymap the cube is filled with black.
*
* [JP]
* パノラマ HDR（Texture2D）を environment キューブの 6 面（ミップ 0）へ投影
* する。出力キューブテクセル 1 つにつき 1 スレッド。スカイマップが無ければ
* キューブを黒で埋める。
*/
[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
    RWTexture2DArray<float4> destination = ResourceDescriptorHeap[bindings.sky_cubemap_];
    uint width;
    uint height;
    uint element_count;
    destination.GetDimensions(width, height, element_count);
    if (id.x >= width || id.y >= height)
    {
        return;
    }

    float2 uv = (float2(id.xy) + 0.5) / float(width) * 2.0 - 1.0;
    float3 direction = CubeFaceDirection(id.z, uv);

    float4 color = float4(0.0, 0.0, 0.0, 1.0);
    if (dispatch_bindings.constants_ != 0xFFFFFFFF)
    {
        Texture2D<float4> source = ResourceDescriptorHeap[dispatch_bindings.constants_];
        color = source.SampleLevel(sampler_linear_clamp, EquirectangularUv(direction), 0);
    }
    destination[id] = color;
}

#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>

FullscreenOutput main(uint vertex_id : SV_VertexID)
{
    FullscreenOutput output;
    float2 texcoord = float2((vertex_id << 1) & 2, vertex_id & 2);
    output.position = float4(texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    output.texcoord = texcoord;
    return output;
}
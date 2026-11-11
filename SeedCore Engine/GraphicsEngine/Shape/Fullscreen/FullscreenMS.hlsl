#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>

[NumThreads(3, 1, 1)]
[OutputTopology("triangle")]
void main(uint group_thread_id : SV_GroupThreadID, out vertices FullscreenOutput output_vertices[3], out indices uint3 output_triangles[1])
{
    SetMeshOutputCounts(3, 1);

    float2 texcoord = float2((group_thread_id << 1) & 2, group_thread_id & 2);
    output_vertices[group_thread_id].position = float4(texcoord * float2(2.0, -2.0) + float2(-1.0, 1.0), 0.0, 1.0);
    output_vertices[group_thread_id].texcoord = texcoord;

    if (group_thread_id == 0)
    {
        output_triangles[0] = uint3(0, 1, 2);
    }
}
#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct PrimitiveWireframeBindings
{
	uint primitive_wireframes_;
	uint geometry_depth_;
};
ConstantBuffer<PrimitiveWireframeBindings> bindings : register(b0, space1);

/**
* [EN]
* One instance spans 3 groups of 64 lines, enough for the capsule's 166
* lines, so gid splits into which instance and which slice of its line list
* this group covers. Each line is emitted as a quad (4 vertices, 2
* triangles) so it can be drawn thicker than one pixel.
*
* ---------------------------------------------------------------------
*
* [JP]
* 1 つのインスタンスは 64 本ずつの 3 グループにまたがる。カプセルの 166 本が
* 入る数なので、gid を「どのインスタンスか」と「その線の一覧のどの部分か」に
* 分ける。1 ピクセルより太く描けるよう、各線は四角形（頂点 4 つ、三角形 2 つ）
* として出力する。
*/
[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices PrimitiveWireframeMSOutput verts[64 * 4], out indices uint3 triangles[64 * 2])
{
	StructuredBuffer<PrimitiveWireframeStructuredBuffer> instances = ResourceDescriptorHeap[bindings.primitive_wireframes_];
	PrimitiveWireframeStructuredBuffer instance = instances[gid / 3];

	uint line_base = (gid % 3) * 64;
	uint line_total = GetPrimitiveWireframeLineCount(instance.shape_kind_);
	uint line_count = line_base < line_total ? min(64, line_total - line_base) : 0;
	SetMeshOutputCounts(line_count * 4, line_count * 2);

	if (gtid < line_count)
	{
		float4 corners[4];
		float4 color;
		PrimitiveWireframeQuad(instance, line_base + gtid, corners, color);
		for (uint corner = 0; corner < 4; corner++)
		{
			verts[gtid * 4 + corner].position = corners[corner];
			verts[gtid * 4 + corner].color = color;
		}
		triangles[gtid * 2 + 0] = uint3(gtid * 4 + 0, gtid * 4 + 1, gtid * 4 + 2);
		triangles[gtid * 2 + 1] = uint3(gtid * 4 + 2, gtid * 4 + 1, gtid * 4 + 3);
	}
}

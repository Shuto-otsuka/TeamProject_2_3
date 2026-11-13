#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct PrimitiveCanvasBindings
{
	uint primitive_wireframes_;
};
ConstantBuffer<PrimitiveCanvasBindings> bindings : register(b0, space1);

/// [EN] Six vertices per line draw its quad as two triangles; this maps each vertex to a quad corner (0/1 at the a end, 2/3 at the b end).
/// [JP] 1 本の線につき頂点 6 つで、四角形を三角形 2 つとして描く。各頂点を四角形の隅（a 端が 0/1、b 端が 2/3）へ対応させる。
static const uint quad_corners[6] = { 0, 1, 2, 2, 1, 3 };

/**
* [EN]
* Fallback when mesh shaders are unavailable. Each drawn instance is one of
* the 3 slices of 64 lines of a shape, matching the mesh shader's groups.
*
* ---------------------------------------------------------------------
*
* [JP]
* メッシュシェーダーが使えないときの代わり。描く 1 インスタンスは、形の
* 64 本ずつの 3 つの区切りの 1 つで、メッシュシェーダーのグループと対応する。
*/
PrimitiveWireframeMSOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	PrimitiveWireframeMSOutput output = (PrimitiveWireframeMSOutput)0;
	output.position = float4(2.0, 2.0, 2.0, 1.0);

	StructuredBuffer<PrimitiveWireframeStructuredBuffer> instances = ResourceDescriptorHeap[bindings.primitive_wireframes_];
	PrimitiveWireframeStructuredBuffer instance = instances[instance_id / 3];

	uint line_index = (instance_id % 3) * 64 + vertex_id / 6;
	if (line_index >= GetPrimitiveWireframeLineCount(instance.shape_kind_))
	{
		return output;
	}

	float4 corners[4];
	float4 color;
	if (PrimitiveWireframeQuad(instance, line_index, corners, color))
	{
		output.position = corners[quad_corners[vertex_id % 6]];
		output.color = color;
	}
	return output;
}

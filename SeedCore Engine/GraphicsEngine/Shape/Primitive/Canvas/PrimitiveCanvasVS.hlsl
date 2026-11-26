#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli>
#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct PrimitiveCanvasBindings
{
	uint primitive_wireframes_;
};
ConstantBuffer<PrimitiveCanvasBindings> bindings : register(b0, space1);

static const uint quad_corners[6] = { 0, 1, 2, 2, 1, 3 };

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

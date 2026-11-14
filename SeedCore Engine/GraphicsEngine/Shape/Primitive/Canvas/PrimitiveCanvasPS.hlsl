#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli>

struct PrimitiveCanvasBindings
{
	uint primitive_wireframes_;
};
ConstantBuffer<PrimitiveCanvasBindings> bindings : register(b0, space1);

float4 main(PrimitiveWireframeMSOutput input) : SV_Target0
{
	return input.color;
}

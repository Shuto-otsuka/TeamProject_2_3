#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli>

struct PrimitiveWireframeBindings
{
	uint primitive_wireframes_;
	uint geometry_depth_;
};
ConstantBuffer<PrimitiveWireframeBindings> bindings : register(b0, space1);

float4 main(PrimitiveWireframeMSOutput input) : SV_Target0
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture2D<float> depth = ResourceDescriptorHeap[bindings.geometry_depth_];
	uint width;
	uint height;
	depth.GetDimensions(width, height);
	float2 uv = input.position.xy / scene.display_size_;
	uint2 pixel = min(uint2(uv * float2(width, height)), uint2(width, height) - 1);
	if (input.position.z < depth.Load(int3(pixel, 0)) - 1.0e-5)
	{
		discard;
	}
	return input.color;
}

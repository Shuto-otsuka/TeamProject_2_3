#include <GraphicsEngine/Shape/Primitive/Wireframe/PrimitiveWireframe.hlsli>

struct PrimitiveWireframeBindings
{
	uint primitive_wireframes_;
	uint geometry_depth_;
};
ConstantBuffer<PrimitiveWireframeBindings> bindings : register(b0, space1);

/**
* [EN]
* The lines are drawn after upscaling, at display resolution, so the scene
* depth (at render resolution) is read at the same place on screen and the
* parts of a line behind the scene are dropped. Depth is reversed-Z, so a
* larger value is nearer.
*
* ---------------------------------------------------------------------
*
* [JP]
* 線はアップスケールの後に表示解像度で描くので、シーンの深度（描画解像度）を
* 画面上の同じ位置で読み、シーンより奥にある線の部分を捨てる。深度は逆 Z
* なので、値が大きいほど手前。
*/
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

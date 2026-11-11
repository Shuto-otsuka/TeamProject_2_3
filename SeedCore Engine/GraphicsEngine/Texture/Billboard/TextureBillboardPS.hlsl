#include <GraphicsEngine/Texture/Billboard/TextureBillboard.hlsli>
#include <GraphicsEngine/Texture/TextureShading.hlsli>

/**
* [EN]
* Texture.Billboard pixel shader: the quad's colour from ShadeTexture
* (TextureShading.hlsli), premultiplied for the premultiplied blend into
* Radiance.Color.
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Billboard のピクセルシェーダー。四角形の色を ShadeTexture
* （TextureShading.hlsli）で求め、Radiance.Color へのプリマルチプライドの
* ブレンドに合わせてアルファを掛けた色で返す。
*/
float4 main(TextureBillboardOutput input) : SV_Target0
{
	StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
	StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
	TextureQuadStructuredBuffer quad = quads[input.quad_index];
	return ShadeTexture(units[quad.unit_], quad, input.texcoord);
}

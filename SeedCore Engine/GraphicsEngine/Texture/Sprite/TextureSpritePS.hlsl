#include <GraphicsEngine/Texture/Sprite/TextureSprite.hlsli>
#include <GraphicsEngine/Texture/TextureShading.hlsli>
#include <GraphicsEngine/PostProcess/Outline/Outline.hlsli>

/**
* [EN]
* Texture.Sprite pixel shader: the quad's colour from ShadeTexture
* (TextureShading.hlsli), premultiplied for the premultiplied blend into
* HUD.Color. When the unit's actor is outlined, the more opaque half of the
* quad also records itself in Outline.Mask with the largest depth, since a
* sprite lies in front of the whole scene; the mask is render resolution,
* so the display pixel is scaled down to it first.
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Sprite のピクセルシェーダー。四角形の色を ShadeTexture
* （TextureShading.hlsli）で求め、HUD.Color へのプリマルチプライドの
* ブレンドに合わせてアルファを掛けた色で返す。ユニットのアクターが縁取られる
* ときは、四角形のうち不透明寄りの部分が Outline.Mask にも一番大きい深度で
* 自分を記録する。スプライトはシーン全体より手前にあるため。マスクは描画解像度
* なので、表示上のピクセルを先にその大きさへ縮める。
*/
float4 main(TextureSpriteOutput input) : SV_Target0
{
	StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
	StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
	TextureQuadStructuredBuffer quad = quads[input.quad_index];
	TextureUnitStructuredBuffer unit = units[quad.unit_];
	float4 color = ShadeTexture(unit, quad, input.texcoord);

	StructuredBuffer<OutlineStyleStructuredBuffer> outline_styles = ResourceDescriptorHeap[bindings.outline_styles_];
	if (color.a > 0.5 && unit.actor_ != TEXTURE_INVALID && outline_styles[unit.actor_].flags_ != 0)
	{
		SceneConstantBuffer scene = GetSceneConstantBuffer();
		RWTexture2D<uint> outline_mask = ResourceDescriptorHeap[bindings.outline_mask_];
		InterlockedMax(outline_mask[uint2(input.position.xy * scene.screen_size_ / scene.display_size_)], (0x7FFFu << 17) | unit.actor_);
	}
	return color;
}

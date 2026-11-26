#include <GraphicsEngine/Texture/Sprite/TextureSprite.hlsli>
#include <GraphicsEngine/Texture/TextureShading.hlsli>
#include <GraphicsEngine/PostProcess/Outline/Outline.hlsli>

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

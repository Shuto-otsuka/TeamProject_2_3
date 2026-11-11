#include <GraphicsEngine/Texture/Sprite/TextureSprite.hlsli>

/**
* [EN]
* Texture.Sprite for devices without mesh shaders: one DrawInstanced
* instance per entry of the sprite section of Texture.DrawList, 6 vertices (two triangles) each, placed exactly
* like TextureSpriteMS.
*
* ---------------------------------------------------------------------
*
* [JP]
* メッシュシェーダーの無いデバイス用の Texture.Sprite。Texture.DrawList の
* スプライトの区間の 1 件につき
* DrawInstanced のインスタンス 1 つ、6 頂点（2 三角形）。配置は TextureSpriteMS と
* 同じ。
*/
TextureSpriteOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	ConstantBuffer<TextureConstantBuffer> constants = ResourceDescriptorHeap[bindings.texture_constants_];
	StructuredBuffer<uint> draw_list = ResourceDescriptorHeap[bindings.texture_draw_list_];
	StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
	StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
	uint quad_index = draw_list[constants.sprite_base_ + instance_id];
	TextureQuadStructuredBuffer quad = quads[quad_index];
	TextureUnitStructuredBuffer unit = units[quad.unit_];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	uint corners[6] = { 0, 1, 2, 1, 3, 2 };
	uint corner_index = corners[vertex_id % 6];
	float2 corner = float2(corner_index & 1, corner_index >> 1);
	float2 clip = float2(corner.x * 2.0 - 1.0, 1.0 - corner.y * 2.0);
	if (unit.display_ != TEXTURE_DISPLAY_FULLSCREEN)
	{
		float2 local = quad.rect_min_ + corner * quad.rect_size_;
		float rotation = unit.kind_ == TEXTURE_KIND_FONT ? 0.0 : atan2(unit.world_[0].y, unit.world_[0].x);
		float2 scale = float2(length(unit.world_[0].xyz), length(unit.world_[1].xyz));
		float2 rotated = float2(local.x * cos(rotation) - local.y * sin(rotation), local.x * sin(rotation) + local.y * cos(rotation));
		float2 pixel = (rotated * scale + unit.world_[3].xy) * (scene.display_size_.y / TEXTURE_CANVAS_HEIGHT);
		clip = float2(pixel.x / scene.display_size_.x * 2.0 - 1.0, 1.0 - pixel.y / scene.display_size_.y * 2.0);
	}

	TextureSpriteOutput output;
	output.position = float4(clip, 0.0, 1.0);
	output.texcoord = lerp(quad.uv_min_, quad.uv_max_, corner);
	output.quad_index = quad_index;
	return output;
}

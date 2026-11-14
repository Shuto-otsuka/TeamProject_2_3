#include <GraphicsEngine/Texture/Billboard/TextureBillboard.hlsli>

/**
* [EN]
* Texture.Billboard for devices without mesh shaders: one DrawInstanced
* instance per entry of the billboard section of Texture.Draws, 6
* vertices (two triangles) each, placed exactly like TextureBillboardMS.
*
* ---------------------------------------------------------------------
*
* [JP]
* メッシュシェーダーの無いデバイス用の Texture.Billboard。Texture.Draws の
* ビルボードの区間の 1 件につき DrawInstanced のインスタンス 1 つ、6 頂点
* （2 三角形）。配置は TextureBillboardMS と同じ。
*/
TextureBillboardOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	ConstantBuffer<TextureConstantBuffer> constants = ResourceDescriptorHeap[bindings.texture_constants_];
	StructuredBuffer<uint> draws = ResourceDescriptorHeap[bindings.texture_draws_];
	StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
	StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
	uint quad_index = draws[constants.billboard_base_ + instance_id];
	TextureQuadStructuredBuffer quad = quads[quad_index];
	TextureUnitStructuredBuffer unit = units[quad.unit_];
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	uint corners[6] = { 0, 1, 2, 1, 3, 2 };
	uint corner_index = corners[vertex_id % 6];
	float2 corner = float2(corner_index & 1, corner_index >> 1);

	float2 local = quad.rect_min_ + corner * quad.rect_size_;
	float3 world_position = mul(float4(local, 0.0, 1.0), unit.world_).xyz;
	if ((unit.flags_ & TEXTURE_UNIT_FACE_CAMERA) != 0)
	{
		float rotation = unit.kind_ == TEXTURE_KIND_FONT ? 0.0 : atan2(unit.world_[0].y, unit.world_[0].x);
		float2 scale = float2(length(unit.world_[0].xyz), length(unit.world_[1].xyz));
		float2 rotated = float2(local.x * cos(rotation) - local.y * sin(rotation), local.x * sin(rotation) + local.y * cos(rotation)) * scale;
		world_position = unit.world_[3].xyz + scene.inverse_view_[0].xyz * rotated.x + scene.inverse_view_[1].xyz * rotated.y;
	}

	TextureBillboardOutput output;
	output.position = mul(float4(world_position, 1.0), scene.current_view_projection_);
	output.texcoord = lerp(quad.uv_min_, quad.uv_max_, corner);
	output.quad_index = quad_index;
	return output;
}

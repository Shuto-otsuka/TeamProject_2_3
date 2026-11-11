#include <GraphicsEngine/Texture/Billboard/TextureBillboard.hlsli>

/**
* [EN]
* Texture.Billboard: one thread group draws 32 consecutive entries of the
* billboard section of Texture.DrawList (from billboard_base_), 4 vertices
* and 2 triangles per quad, in list order so the premultiplied blend layers
* them as Texture.Expand ordered them.
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Billboard。1 スレッドグループが Texture.DrawList のビルボードの区間
* （billboard_base_ から）の連続する 32 件を、1 四角形 4 頂点・2 三角形で描く。
* リストの順に出すので、プリマルチプライドのブレンドは Texture.Expand が並べた
* 順に重なる。
*/
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices TextureBillboardOutput verts[128], out indices uint3 tris[64])
{
	ConstantBuffer<TextureConstantBuffer> constants = ResourceDescriptorHeap[bindings.texture_constants_];
	uint first = gid * 32;
	uint quad_count = first < constants.billboard_count_ ? min(32, constants.billboard_count_ - first) : 0;
	SetMeshOutputCounts(quad_count * 4, quad_count * 2);
	if (quad_count == 0)
	{
		return;
	}

	uint quad_local = gtid / 4;
	if (quad_local < quad_count)
	{
		StructuredBuffer<uint> draw_list = ResourceDescriptorHeap[bindings.texture_draw_list_];
		StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
		StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
		uint quad_index = draw_list[constants.billboard_base_ + first + quad_local];
		TextureQuadStructuredBuffer quad = quads[quad_index];
		TextureUnitStructuredBuffer unit = units[quad.unit_];
		SceneConstantBuffer scene = GetSceneConstantBuffer();

		uint corner_index = gtid % 4;
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
		verts[gtid] = output;
	}

	if (gtid < quad_count * 2)
	{
		uint base = (gtid / 2) * 4;
		tris[gtid] = (gtid & 1) == 0 ? uint3(base + 0, base + 1, base + 2) : uint3(base + 1, base + 3, base + 2);
	}
}

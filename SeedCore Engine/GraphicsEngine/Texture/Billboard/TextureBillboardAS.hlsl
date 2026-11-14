#include <GraphicsEngine/Texture/Billboard/TextureBillboard.hlsli>
#include <GraphicsEngine/Culling/FrustumCulling.hlsli>

groupshared TextureBillboardPayload payload;
groupshared uint candidates[32];

/**
* [EN]
* Texture.Billboard culling: one thread group per 32 consecutive entries of
* the billboard section of Texture.Draws, one thread per quad. The quad's
* four corners are placed in the world exactly as TextureBillboardMS places
* them (facing the camera when the unit asks to), and the quad is kept when
* the sphere around them touches the view frustum. The survivors are packed
* in draw-list order so the premultiplied blend still layers them as
* Texture.Expand ordered them, and the group ends in exactly one DispatchMesh,
* launching one TextureBillboardMS group for all of them (none when nothing
* survives).
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Billboard のカリング。Texture.Draws のビルボードの区間の連続する 32 件に
* つき 1 スレッドグループ、四角形 1 つにつき 1 スレッド。四角形の 4 隅を
* TextureBillboardMS と同じ式で（ユニットが求めればカメラに向けて）ワールドに置き、
* それを囲む球が視錐台に触れていれば残す。残ったものは描画リストの順のまま詰める
* ので、プリマルチプライドのブレンドは Texture.Expand が並べた順に重なる。どの
* グループも最後に DispatchMesh をちょうど 1 回呼び、残った全部を
* TextureBillboardMS の 1 グループで描く（何も残らなければ起動しない）。
*/
[NumThreads(32, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
	ConstantBuffer<TextureConstantBuffer> constants = ResourceDescriptorHeap[bindings.texture_constants_];
	uint draw_index = gid * 32 + gtid;
	uint candidate = TEXTURE_INVALID;
	if (draw_index < constants.billboard_count_)
	{
		StructuredBuffer<uint> draws = ResourceDescriptorHeap[bindings.texture_draws_];
		StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
		StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
		uint quad_index = draws[constants.billboard_base_ + draw_index];
		TextureQuadStructuredBuffer quad = quads[quad_index];
		TextureUnitStructuredBuffer unit = units[quad.unit_];
		SceneConstantBuffer scene = GetSceneConstantBuffer();

		float rotation = unit.kind_ == TEXTURE_KIND_FONT ? 0.0 : atan2(unit.world_[0].y, unit.world_[0].x);
		float2 scale = float2(length(unit.world_[0].xyz), length(unit.world_[1].xyz));
		float3 corners[4];
		for (uint corner_index = 0; corner_index < 4; corner_index++)
		{
			float2 corner = float2(corner_index & 1, corner_index >> 1);
			float2 local = quad.rect_min_ + corner * quad.rect_size_;
			corners[corner_index] = mul(float4(local, 0.0, 1.0), unit.world_).xyz;
			if ((unit.flags_ & TEXTURE_UNIT_FACE_CAMERA) != 0)
			{
				float2 rotated = float2(local.x * cos(rotation) - local.y * sin(rotation), local.x * sin(rotation) + local.y * cos(rotation)) * scale;
				corners[corner_index] = unit.world_[3].xyz + scene.inverse_view_[0].xyz * rotated.x + scene.inverse_view_[1].xyz * rotated.y;
			}
		}
		float3 center = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25;
		float radius = sqrt(max(max(dot(corners[0] - center, corners[0] - center), dot(corners[1] - center, corners[1] - center)), max(dot(corners[2] - center, corners[2] - center), dot(corners[3] - center, corners[3] - center))));
		if (IsInsideFrustum(center, radius, scene.current_view_projection_))
		{
			candidate = quad_index;
		}
	}
	candidates[gtid] = candidate;
	GroupMemoryBarrierWithGroupSync();

	if (gtid == 0)
	{
		uint count = 0;
		for (uint candidate_index = 0; candidate_index < 32; candidate_index++)
		{
			if (candidates[candidate_index] != TEXTURE_INVALID)
			{
				payload.quads_[count] = candidates[candidate_index];
				count++;
			}
		}
		payload.count_ = count;
	}
	GroupMemoryBarrierWithGroupSync();

	DispatchMesh(payload.count_ != 0 ? 1 : 0, 1, 1, payload);
}

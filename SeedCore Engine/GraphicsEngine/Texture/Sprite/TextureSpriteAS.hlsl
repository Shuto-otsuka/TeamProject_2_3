#include <GraphicsEngine/Texture/Sprite/TextureSprite.hlsli>

groupshared TextureSpritePayload payload;
groupshared uint candidates[32];

/**
* [EN]
* Texture.Sprite culling: one thread group per 32 consecutive entries of the
* sprite section of Texture.Draws, one thread per quad. A sprite quad is
* placed on the canvas exactly as TextureSpriteMS places it and kept when its
* pixel rectangle overlaps the view; a fullscreen quad is always kept. The
* survivors are packed in draw-list order so the units' drawing order holds,
* and the group ends in exactly one DispatchMesh, launching one
* TextureSpriteMS group for all of them (none when nothing survives).
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Sprite のカリング。Texture.Draws のスプライトの区間の連続する 32 件に
* つき 1 スレッドグループ、四角形 1 つにつき 1 スレッド。スプライトの四角形は
* TextureSpriteMS と同じ式でキャンバスに置き、その画素の矩形がビューと重なれば残す。
* 全画面の四角形は常に残す。残ったものは描画リストの順のまま詰めるので、ユニットの
* 描く順番が保たれる。どのグループも最後に DispatchMesh をちょうど 1 回呼び、残った
* 全部を TextureSpriteMS の 1 グループで描く（何も残らなければ起動しない）。
*/
[NumThreads(32, 1, 1)]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID)
{
	ConstantBuffer<TextureConstantBuffer> constants = ResourceDescriptorHeap[bindings.texture_constants_];
	uint draw_index = gid * 32 + gtid;
	uint candidate = TEXTURE_INVALID;
	if (draw_index < constants.sprite_count_)
	{
		StructuredBuffer<uint> draws = ResourceDescriptorHeap[bindings.texture_draws_];
		StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
		StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
		uint quad_index = draws[constants.sprite_base_ + draw_index];
		TextureQuadStructuredBuffer quad = quads[quad_index];
		TextureUnitStructuredBuffer unit = units[quad.unit_];
		candidate = quad_index;
		if (unit.display_ != TEXTURE_DISPLAY_FULLSCREEN)
		{
			SceneConstantBuffer scene = GetSceneConstantBuffer();
			float rotation = unit.kind_ == TEXTURE_KIND_FONT ? 0.0 : atan2(unit.world_[0].y, unit.world_[0].x);
			float2 scale = float2(length(unit.world_[0].xyz), length(unit.world_[1].xyz));
			float2 pixel_min = float2(3.402823e38, 3.402823e38);
			float2 pixel_max = float2(-3.402823e38, -3.402823e38);
			for (uint corner_index = 0; corner_index < 4; corner_index++)
			{
				float2 corner = float2(corner_index & 1, corner_index >> 1);
				float2 local = quad.rect_min_ + corner * quad.rect_size_;
				float2 rotated = float2(local.x * cos(rotation) - local.y * sin(rotation), local.x * sin(rotation) + local.y * cos(rotation));
				float2 pixel = (rotated * scale + unit.world_[3].xy) * (scene.display_size_.y / TEXTURE_CANVAS_HEIGHT);
				pixel_min = min(pixel_min, pixel);
				pixel_max = max(pixel_max, pixel);
			}
			if (any(pixel_max < 0.0) || any(pixel_min > scene.display_size_))
			{
				candidate = TEXTURE_INVALID;
			}
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

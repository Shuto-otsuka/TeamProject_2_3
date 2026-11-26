#include <GraphicsEngine/Texture/Sprite/TextureSprite.hlsli>

groupshared TextureSpritePayload payload;
groupshared uint candidates[32];

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
			float2 clip_min = float2(3.402823e38, 3.402823e38);
			float2 clip_max = float2(-3.402823e38, -3.402823e38);
			for (uint corner_index = 0; corner_index < 4; corner_index++)
			{
				float2 corner = float2(corner_index & 1, corner_index >> 1);
				float2 local = quad.rect_min_ + corner * quad.rect_size_;
				float2 rotated = float2(local.x * cos(rotation) - local.y * sin(rotation), local.x * sin(rotation) + local.y * cos(rotation));
				float2 canvas = rotated * scale + unit.world_[3].xy;
				float2 clip;
				if (scene.view_kind_ == SCENE_VIEW_KIND_CANVAS)
				{
					float3 world_position = float3(100000.0 + canvas.x, 100000.0 + TEXTURE_CANVAS_HEIGHT - canvas.y, 100000.0);
					clip = mul(float4(world_position, 1.0), scene.current_view_projection_).xy;
				}
				else
				{
					float2 pixel = canvas * (scene.display_size_.y / TEXTURE_CANVAS_HEIGHT);
					clip = float2(pixel.x / scene.display_size_.x * 2.0 - 1.0, 1.0 - pixel.y / scene.display_size_.y * 2.0);
				}
				clip_min = min(clip_min, clip);
				clip_max = max(clip_max, clip);
			}
			if (any(clip_max < -1.0) || any(clip_min > 1.0))
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

#include "../Font.hlsli"
#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Culling.hlsli"
#include "../../Shader/Dispatch.hlsli"

groupshared uint survived_count;
groupshared uint local_indices[32];
groupshared FontASPayload payload;

[numthreads(32, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID, uint3 dtid : SV_DispatchThreadID)
{
	StructuredBuffer<FontSpriteStructuredBuffer> font_sprite = GetFontSpriteStructuredBuffer(shader_resource_indices.font_.sprite_index_);
	SceneConstantBuffer scene_constant = GetSceneConstantBuffer();

	if (gtid.x == 0)
	{
		survived_count = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	bool is_visible = false;
	bool is_ranged = dispatch_buffer_index_ != 0xFFFFFFFFu;
	uint range_start = is_ranged ? dispatch_buffer_index_ : 0u;
	uint range_batch = font_sprite[range_start].batch_;
	uint glyph_id = range_start + dtid.x;

	if (glyph_id < 65536)
	{
		FontSpriteStructuredBuffer glyph = font_sprite[glyph_id];

		if ((!is_ranged || glyph.batch_ == range_batch) && glyph.size_.x > 0.0 && glyph.size_.y > 0.0)
		{
			is_visible = IsVisibleInScreen(glyph.position_, glyph.size_, scene_constant.display_size_);
		}
	}

	local_indices[gtid.x] = is_visible ? glyph_id : 0xFFFFFFFFu;

	GroupMemoryBarrierWithGroupSync();

	if (gtid.x == 0)
	{
		for (uint index = 0; index < 32; ++index)
		{
			if (local_indices[index] != 0xFFFFFFFFu)
			{
				payload.glyph_indices[survived_count] = local_indices[index];
				++survived_count;
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();

	DispatchMesh(survived_count, 1, 1, payload);
}

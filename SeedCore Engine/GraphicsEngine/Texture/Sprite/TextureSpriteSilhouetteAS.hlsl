#include "../Texture.hlsli"
#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Culling.hlsli"
#include "../../Shader/Dispatch.hlsli"

groupshared uint survived_count;
groupshared uint local_indices[32];
groupshared TextureASPayload payload;

[numthreads(32, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID, uint3 dtid : SV_DispatchThreadID)
{
	StructuredBuffer<TextureSpriteStructuredBuffer> texture_sprite = GetTextureSpriteStructuredBuffer(shader_resource_indices.texture_.sprite_index_);
	SceneConstantBuffer scene_constant = GetSceneConstantBuffer();

	if (gtid.x == 0)
	{
		survived_count = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	bool is_visible = false;
	bool is_ranged = dispatch_buffer_index_ != 0xFFFFFFFFu;
	uint range_start = is_ranged ? dispatch_buffer_index_ : 0u;
	uint range_batch = texture_sprite[range_start].batch_;
	uint sprite_id = range_start + dtid.x;

	if (sprite_id < 32768)
	{
		TextureSpriteStructuredBuffer sprite = texture_sprite[sprite_id];

		if ((!is_ranged || sprite.batch_ == range_batch) && sprite.selected_ != 0 && sprite.scale_.x > 0.0 && sprite.scale_.y > 0.0)
		{
			is_visible = IsVisibleInScreen(sprite.position_, sprite.texture_size_ * sprite.scale_, scene_constant.display_size_);
		}
	}

	local_indices[gtid.x] = is_visible ? sprite_id : 0xFFFFFFFFu;

	GroupMemoryBarrierWithGroupSync();

	if (gtid.x == 0)
	{
		for (uint index = 0; index < 32; ++index)
		{
			if (local_indices[index] != 0xFFFFFFFFu)
			{
				payload.texture_indices[survived_count] = local_indices[index];
				++survived_count;
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();

	DispatchMesh(survived_count, 1, 1, payload);
}

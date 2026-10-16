#include "../Movie.hlsli"
#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Culling.hlsli"
#include "../../Shader/Dispatch.hlsli"

groupshared uint survived_count;
groupshared uint local_indices[32];
groupshared MovieASPayload payload;

[numthreads(32, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID, uint3 dtid : SV_DispatchThreadID)
{
	StructuredBuffer<MovieSpriteStructuredBuffer> movie_sprite = GetMovieSpriteStructuredBuffer(shader_resource_indices.movie_.sprite_index_);
	SceneConstantBuffer scene_constant = GetSceneConstantBuffer();

	if (gtid.x == 0)
	{
		survived_count = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	bool is_visible = false;
	bool is_ranged = dispatch_buffer_index_ != 0xFFFFFFFFu;
	uint range_start = is_ranged ? dispatch_buffer_index_ : 0u;
	uint range_batch = movie_sprite[range_start].batch_;
	uint instance_id = range_start + dtid.x;

	if (instance_id < 1024)
	{
		MovieSpriteStructuredBuffer instance = movie_sprite[instance_id];

		if ((!is_ranged || instance.batch_ == range_batch) && instance.size_.x > 0.0 && instance.size_.y > 0.0)
		{
			is_visible = IsVisibleInScreen(instance.position_, instance.size_ * instance.scale_, scene_constant.display_size_);
		}
	}

	local_indices[gtid.x] = is_visible ? instance_id : 0xFFFFFFFFu;

	GroupMemoryBarrierWithGroupSync();

	if (gtid.x == 0)
	{
		for (uint index = 0; index < 32; ++index)
		{
			if (local_indices[index] != 0xFFFFFFFFu)
			{
				payload.instance_indices[survived_count] = local_indices[index];
				++survived_count;
			}
		}
	}

	GroupMemoryBarrierWithGroupSync();

	DispatchMesh(survived_count, 1, 1, payload);
}

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
	StructuredBuffer<MovieBillboardStructuredBuffer> movie_billboard = GetMovieBillboardStructuredBuffer(shader_resource_indices.movie_.billboard_index_);
	SceneConstantBuffer scene_constant = GetSceneConstantBuffer();

	if (gtid.x == 0)
	{
		survived_count = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	bool is_visible = false;
	bool is_ranged = dispatch_buffer_index_ != 0xFFFFFFFFu;
	uint range_start = is_ranged ? dispatch_buffer_index_ : 0u;
	uint range_batch = movie_billboard[range_start].batch_;
	uint instance_id = range_start + dtid.x;

	if (instance_id < 1024)
	{
		MovieBillboardStructuredBuffer instance = movie_billboard[instance_id];

		if ((!is_ranged || instance.batch_ == range_batch) && instance.scale_.x > 0.0 && instance.scale_.y > 0.0)
		{
			float radius = max(instance.scale_.x, instance.scale_.y) * 0.5f;
			is_visible = IsVisibleInFrustum(instance.position_, radius, scene_constant.current_view_projection_);
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

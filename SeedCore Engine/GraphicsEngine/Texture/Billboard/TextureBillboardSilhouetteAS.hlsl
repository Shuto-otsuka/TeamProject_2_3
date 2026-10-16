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
	StructuredBuffer<TextureBillboardStructuredBuffer> texture_billboard = GetTextureBillboardStructuredBuffer(shader_resource_indices.texture_.billboard_index_);
	SceneConstantBuffer scene_constant = GetSceneConstantBuffer();

	if (gtid.x == 0)
	{
		survived_count = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	bool is_visible = false;
	bool is_ranged = dispatch_buffer_index_ != 0xFFFFFFFFu;
	uint range_start = is_ranged ? dispatch_buffer_index_ : 0u;
	uint range_batch = texture_billboard[range_start].batch_;
	uint billboard_id = range_start + dtid.x;

	if (billboard_id < 32768)
	{
		TextureBillboardStructuredBuffer billboard = texture_billboard[billboard_id];

		if ((!is_ranged || billboard.batch_ == range_batch) && billboard.selected_ != 0 && billboard.scale_.x > 0.0 && billboard.scale_.y > 0.0)
		{
			float radius = max(billboard.scale_.x, billboard.scale_.y) * 0.5f;
			is_visible = IsVisibleInFrustum(billboard.position_, radius, scene_constant.current_view_projection_);
		}
	}

	local_indices[gtid.x] = is_visible ? billboard_id : 0xFFFFFFFFu;

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

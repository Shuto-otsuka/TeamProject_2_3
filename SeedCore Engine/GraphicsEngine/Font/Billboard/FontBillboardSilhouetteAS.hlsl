#include "../Font.hlsli"
#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Culling.hlsli"
#include "../../Shader/Dispatch.hlsli"

groupshared uint survived_count;
groupshared uint local_indices[32];
groupshared FontASPayload payload;

/**
* [JP]
* シルエット用 Amplification Shader。FontBillboardAS.hlsl の
* コピー（編集時は同期を保つこと）に selected != 0 のインスタンスだけを通す
* フィルタを追加したもの。
*/
[numthreads(32, 1, 1)]
void main(uint3 gtid : SV_GroupThreadID, uint3 dtid : SV_DispatchThreadID)
{
	StructuredBuffer<FontBillboardStructuredBuffer> font_billboard = GetFontBillboardStructuredBuffer(shader_resource_indices.font_.billboard_index_);
	SceneConstantBuffer scene_constant = GetSceneConstantBuffer();

	if (gtid.x == 0)
	{
		survived_count = 0;
	}
	GroupMemoryBarrierWithGroupSync();

	bool is_visible = false;
	bool is_ranged = dispatch_buffer_index_ != 0xFFFFFFFFu;
	uint range_start = is_ranged ? dispatch_buffer_index_ : 0u;
	uint range_batch = font_billboard[range_start].batch_;
	uint glyph_id = range_start + dtid.x;

	if (glyph_id < 65536)
	{
		FontBillboardStructuredBuffer glyph = font_billboard[glyph_id];

		if ((!is_ranged || glyph.batch_ == range_batch) && glyph.selected_ != 0 && glyph.local_size_.x > 0.0 && glyph.local_size_.y > 0.0)
		{
			float2 center_local = glyph.local_position_ + glyph.local_size_ * 0.5f;
			float3 scaled = float3(center_local.x, center_local.y, 0.0f);

			float cx = cos(glyph.rotation_.x); float sx = sin(glyph.rotation_.x);
			float cy = cos(glyph.rotation_.y); float sy = sin(glyph.rotation_.y);
			float cz = cos(glyph.rotation_.z); float sz = sin(glyph.rotation_.z);

			float3 rotated;
			rotated.x = (cy * cz) * scaled.x + (sx * sy * cz - cx * sz) * scaled.y;
			rotated.y = (cy * sz) * scaled.x + (sx * sy * sz + cx * cz) * scaled.y;
			rotated.z = (-sy)     * scaled.x + (sx * cy)                * scaled.y;

			float3 world_center = glyph.position_ + rotated;
			float radius = max(glyph.local_size_.x, glyph.local_size_.y) * 0.5f;
			is_visible = IsVisibleInFrustum(world_center, radius, scene_constant.current_view_projection_);
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

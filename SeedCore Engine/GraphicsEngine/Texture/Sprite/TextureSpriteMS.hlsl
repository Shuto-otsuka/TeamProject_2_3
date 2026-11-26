#include <GraphicsEngine/Texture/Sprite/TextureSprite.hlsli>

[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, in payload TextureSpritePayload payload, out vertices TextureSpriteOutput verts[128], out indices uint3 tris[64])
{
	uint quad_count = payload.count_;
	SetMeshOutputCounts(quad_count * 4, quad_count * 2);

	uint quad_local = gtid / 4;
	if (quad_local < quad_count)
	{
		StructuredBuffer<TextureQuadStructuredBuffer> quads = ResourceDescriptorHeap[bindings.texture_quads_];
		StructuredBuffer<TextureUnitStructuredBuffer> units = ResourceDescriptorHeap[bindings.texture_units_];
		uint quad_index = payload.quads_[quad_local];
		TextureQuadStructuredBuffer quad = quads[quad_index];
		TextureUnitStructuredBuffer unit = units[quad.unit_];
		SceneConstantBuffer scene = GetSceneConstantBuffer();

		uint corner_index = gtid % 4;
		float2 corner = float2(corner_index & 1, corner_index >> 1);
		float2 clip = float2(corner.x * 2.0 - 1.0, 1.0 - corner.y * 2.0);
		if (unit.display_ != TEXTURE_DISPLAY_FULLSCREEN)
		{
			float2 local = quad.rect_min_ + corner * quad.rect_size_;
			float rotation = unit.kind_ == TEXTURE_KIND_FONT ? 0.0 : atan2(unit.world_[0].y, unit.world_[0].x);
			float2 scale = float2(length(unit.world_[0].xyz), length(unit.world_[1].xyz));
			float2 rotated = float2(local.x * cos(rotation) - local.y * sin(rotation), local.x * sin(rotation) + local.y * cos(rotation));
			float2 canvas = rotated * scale + unit.world_[3].xy;
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
		}

		TextureSpriteOutput output;
		output.position = float4(clip, 0.0, 1.0);
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

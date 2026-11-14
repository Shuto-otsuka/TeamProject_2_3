#include <GraphicsEngine/Texture/Sprite/TextureSprite.hlsli>

/**
* [EN]
* Texture.Sprite: one thread group draws the on-screen quads TextureSpriteAS
* kept from 32 consecutive entries of the sprite section of Texture.Draws,
* 4 vertices and 2 triangles per quad, in list order so the units' drawing
* order holds. A fullscreen quad covers the whole view; a
* sprite quad is placed on the 1920x1080 canvas by its unit's world matrix
* (translation, z rotation - none for text - and scale) and then scaled to the view's display
* height, so the game and canvas views lay sprites out the same way (the
* editor view draws no sprites).
*
* ---------------------------------------------------------------------
*
* [JP]
* Texture.Sprite。1 スレッドグループが、Texture.Draws のスプライトの区間の
* 連続する 32 件から TextureSpriteAS が残した画面内の四角形を、
* 1 四角形につき 4 頂点・2 三角形で描く。並びの順に描くので、ユニットの
* 描く順番が保たれる。全画面の四角形はビュー全体を覆い、スプライトの
* 四角形はユニットの世界行列（平行移動・z 回転・拡大）で 1920x1080 の
* キャンバスに置いてから（テキストは回転しない）、ビューの表示の高さに合わせて広げる。ゲームとキャンバスの
* ビューで同じ配置になる（エディターのビューにはスプライトを描かない）。
*/
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
			float2 pixel = (rotated * scale + unit.world_[3].xy) * (scene.display_size_.y / TEXTURE_CANVAS_HEIGHT);
			clip = float2(pixel.x / scene.display_size_.x * 2.0 - 1.0, 1.0 - pixel.y / scene.display_size_.y * 2.0);
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

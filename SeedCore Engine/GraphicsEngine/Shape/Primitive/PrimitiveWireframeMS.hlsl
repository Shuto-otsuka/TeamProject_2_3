#include "PrimitiveWireframe.hlsli"
#include "../../Shader/ShaderResources.hlsli"

/**
* [EN]
* One instance spans 3 groups of 64 lines, enough for the capsule's 166
* lines (must match the C++ groups per instance), so gid splits into
* which instance and which slice of its line list this group covers. Each
* line is emitted as a quad (4 vertices, 2 triangles) so it can be drawn
* thicker than one pixel.
*
* ---------------------------------------------------------------------
*
* [JP]
* 1つのインスタンスは 64 本ずつの 3 グループにまたがる。カプセルの
* 166 本が入る数で、C++ 側のグループ数と一致させる。gid を「どの
* インスタンスか」と「その線の一覧のどの部分か」に分ける。1ピクセルより
* 太く描けるよう、各線は四角形（頂点4つ、三角形2つ）として出力する。
*/
[NumThreads(64, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices PrimitiveWireframeMSOutput verts[64 * 4], out indices uint3 triangles[64 * 2])
{
	StructuredBuffer<PrimitiveWireframeStructuredBuffer> instances = GetPrimitiveWireframeStructuredBuffer(shader_resource_indices.primitive_wireframe_.instance_index_);

	uint instance_index = gid / 3;
	uint sub_group_index = gid % 3;
	uint line_index = sub_group_index * 64 + gtid;

	PrimitiveWireframeStructuredBuffer instance = instances[instance_index];

	uint total_lines = GetPrimitiveWireframeLineCount(instance.shape_kind_);
	uint sub_group_base = sub_group_index * 64;
	uint group_line_count = (sub_group_base < total_lines) ? min(64, total_lines - sub_group_base) : 0;

	SetMeshOutputCounts(group_line_count * 4, group_line_count * 2);

	if (gtid < group_line_count)
	{
		SceneConstantBuffer scene = GetSceneConstantBuffer();

		/// [EN] The silhouette depends on where the camera is, so the camera is brought into the shape's local space. A unit quaternion q rotates v as v + w*t + cross(xyz, t) with t = 2*cross(xyz, v); the inverse rotation negates xyz.
		/// [JP] 輪郭線はカメラの位置で決まるため、カメラをこの形状のローカル空間へ移す。単位クォータニオン q による v の回転は、t = 2*cross(xyz, v) として v + w*t + cross(xyz, t)。逆回転は xyz の符号を反転する。
		float3 to_camera = scene.camera_position_.xyz - instance.position_;
		float3 inverse_axis = -instance.rotation_.xyz;
		float3 camera_twist = 2.0 * cross(inverse_axis, to_camera);
		float3 local_camera = to_camera + instance.rotation_.w * camera_twist + cross(inverse_axis, camera_twist);

		float3 local_a;
		float3 local_b;
		bool silhouette;
		GetPrimitiveWireframeLine(instance.shape_kind_, instance.dimensions_, instance.head_length_, local_camera, line_index, local_a, local_b, silhouette);

		/// [EN] Both ends go back to world space by the instance rotation, then the instance position.
		/// [JP] 両端をインスタンスの回転、続いて位置でワールド空間へ戻す。
		float3 twist_a = 2.0 * cross(instance.rotation_.xyz, local_a);
		float3 world_a = local_a + instance.rotation_.w * twist_a + cross(instance.rotation_.xyz, twist_a) + instance.position_;
		float3 twist_b = 2.0 * cross(instance.rotation_.xyz, local_b);
		float3 world_b = local_b + instance.rotation_.w * twist_b + cross(instance.rotation_.xyz, twist_b) + instance.position_;
		float4 clip_a = mul(float4(world_a, 1.0), scene.current_view_projection_);
		float4 clip_b = mul(float4(world_b, 1.0), scene.current_view_projection_);

		/// [EN] Silhouette lines are drawn halfway to white and 1.5 times the instance's line width, so the outline stands out from the rest of the shape.
		/// [JP] 輪郭線は白へ半分寄せた色で、インスタンスの線の太さの 1.5 倍で描き、形状のほかの線より目立たせる。
		float4 color = silhouette ? float4(lerp(instance.color_.rgb, float3(1.0, 1.0, 1.0), 0.5), instance.color_.a) : instance.color_;
		float width = silhouette ? instance.line_width_ * 1.5 : instance.line_width_;

		/// [EN] An end behind the camera has no screen position, so the line is cut where it crosses just in front of the eye; a line entirely behind the camera is not drawn.
		/// [JP] カメラの後ろにある端には画面上の位置が無いため、目のすぐ手前を横切る位置で線を切る。線全体がカメラの後ろにあれば描かない。
		const float near_w = 1.0e-3;
		bool visible = clip_a.w >= near_w || clip_b.w >= near_w;
		if (visible && clip_a.w < near_w)
		{
			clip_a = lerp(clip_a, clip_b, (near_w - clip_a.w) / (clip_b.w - clip_a.w));
		}
		if (visible && clip_b.w < near_w)
		{
			clip_b = lerp(clip_b, clip_a, (near_w - clip_b.w) / (clip_a.w - clip_b.w));
		}

		/// [EN] The line's direction and its perpendicular are taken in pixels, so the width stays even however the viewport is stretched. A line shorter than a pixel fraction has no direction and is not drawn.
		/// [JP] 線の向きとその垂直方向はピクセル単位で求め、ビューポートの縦横比に関係なく太さを均一に保つ。ごく短く向きの定まらない線は描かない。
		float2 pixel_direction = float2(0.0, 0.0);
		float2 pixel_normal = float2(0.0, 0.0);
		if (visible)
		{
			float2 pixel_a = (clip_a.xy / clip_a.w) * scene.display_size_;
			float2 pixel_b = (clip_b.xy / clip_b.w) * scene.display_size_;
			pixel_direction = pixel_b - pixel_a;
			float pixel_length = length(pixel_direction);
			if (pixel_length < 1.0e-6)
			{
				visible = false;
			}
			else
			{
				pixel_direction /= pixel_length;
				pixel_normal = float2(-pixel_direction.y, pixel_direction.x);
			}
		}

		/// [EN] Normalized device coordinates span 2 across display_size pixels, so half the width in pixels is width / display_size in them.
		/// [JP] 正規化デバイス座標は display_size ピクセルで 2 の幅なので、ピクセルでの太さの半分は、その座標では width / display_size になる。
		float2 to_ndc = width / scene.display_size_;

		/// [EN] Four corners of the line's quad, split into two triangles: corners 0 and 1 sit on the a end, 2 and 3 on the b end, on opposite sides of the line. Each end is also pushed out by half the width so neighbouring segments of a curve overlap at their joints, and every offset is scaled by w so it survives the perspective divide unchanged.
		/// [JP] 線の四角形の4隅を出力し、三角形2つに分ける。隅 0 と 1 は a 端、2 と 3 は b 端にあり、線をはさんで反対側に置く。曲線の隣り合う線分がつなぎ目で重なるよう各端も太さの半分だけ外へ延ばし、どのずらし量も透視除算の後に変わらないよう w を掛ける。
		for (uint corner = 0; corner < 4; corner++)
		{
			float4 clip_position = float4(0.0, 0.0, 0.0, 0.0);
			if (visible)
			{
				float2 side_offset = pixel_normal * to_ndc * (((corner & 1) != 0) ? 1.0 : -1.0);
				float2 end_offset = pixel_direction * to_ndc * ((corner < 2) ? -1.0 : 1.0);
				clip_position = (corner < 2) ? clip_a : clip_b;
				clip_position.xy += (side_offset + end_offset) * clip_position.w;
			}

			verts[gtid * 4 + corner].position = clip_position;
			verts[gtid * 4 + corner].color = color;
		}

		triangles[gtid * 2 + 0] = uint3(gtid * 4 + 0, gtid * 4 + 1, gtid * 4 + 2);
		triangles[gtid * 2 + 1] = uint3(gtid * 4 + 2, gtid * 4 + 1, gtid * 4 + 3);
	}
}

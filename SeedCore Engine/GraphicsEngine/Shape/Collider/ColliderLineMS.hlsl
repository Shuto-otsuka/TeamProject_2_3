#include "ColliderLine.hlsli"

/// [EN] One collider instance spans ColliderConstantBuffer::
///      groups_per_instance_ groups (a capsule has more lines than a
///      single group's COLLIDER_LINES_PER_GROUP) — gid decomposes into which
///      instance and which slice of that instance's line list this group
///      covers. Each line is emitted as a quad (4 vertices, 2 triangles) so
///      it can be drawn thicker than one pixel.
/// [JP] 1つのコライダーインスタンスは ColliderConstantBuffer::
///      groups_per_instance_ 個のグループにまたがる(カプセルは1グループの
///      COLLIDER_LINES_PER_GROUP 本より線が多いため) — gid を「どのインスタンスか」と
///      「そのインスタンスの線リストのうちどのスライスか」に分解する。
///      1ピクセルより太く描けるよう、各線は四角形(頂点4つ、三角形2つ)として出力する。
[NumThreads(COLLIDER_LINES_PER_GROUP, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices ColliderLineMSOutput verts[COLLIDER_LINES_PER_GROUP * 4], out indices uint3 triangles[COLLIDER_LINES_PER_GROUP * 2])
{
	ColliderConstantBuffer collider = GetColliderConstantBuffer();

	uint groups_per_instance = collider.groups_per_instance_;
	uint instance_index = gid / groups_per_instance;
	uint sub_group_index = gid % groups_per_instance;
	uint line_index = sub_group_index * COLLIDER_LINES_PER_GROUP + gtid;

	uint group_line_count = 0;
	ColliderStructuredBuffer instance = (ColliderStructuredBuffer)0;

	if (instance_index < collider.instance_count_)
	{
		StructuredBuffer<ColliderStructuredBuffer> instances = GetColliderStructuredBuffer(collider.instance_buffer_index_);
		instance = instances[instance_index];

		uint total_lines = GetColliderLineCount(instance.shape_kind_);
		uint sub_group_base = sub_group_index * COLLIDER_LINES_PER_GROUP;
		if (sub_group_base < total_lines)
		{
			group_line_count = min(COLLIDER_LINES_PER_GROUP, total_lines - sub_group_base);
		}
	}

	SetMeshOutputCounts(group_line_count * 4, group_line_count * 2);

	if (gtid < group_line_count)
	{
		SceneConstantBuffer scene = GetSceneConstantBuffer();

		/// [EN] The silhouette depends on where the camera is, so the camera is brought into the shape's local space.
		/// [JP] 輪郭線はカメラの位置で決まるため、カメラをこの形状のローカル空間へ移す。
		float4 inverse_rotation = float4(-instance.rotation_.xyz, instance.rotation_.w);
		float3 local_camera = RotateByQuaternion(inverse_rotation, scene.camera_position_.xyz - instance.position_);

		float3 local_a;
		float3 local_b;
		bool silhouette;
		GetColliderLine(instance.shape_kind_, instance.dimensions_, local_camera, line_index, local_a, local_b, silhouette);

		float3 world_a = RotateByQuaternion(instance.rotation_, local_a) + instance.position_;
		float3 world_b = RotateByQuaternion(instance.rotation_, local_b) + instance.position_;
		float4 clip_a = mul(float4(world_a, 1.0), scene.current_view_projection_);
		float4 clip_b = mul(float4(world_b, 1.0), scene.current_view_projection_);

		float4 color = GetColliderLineColor(instance.color_, silhouette);
		float width = silhouette ? COLLIDER_SILHOUETTE_LINE_WIDTH : COLLIDER_LINE_WIDTH;

		/// [EN] Four corners of the line's quad, split into two triangles.
		/// [JP] 線の四角形の4隅を出力し、三角形2つに分ける。
		for (uint corner = 0; corner < 4; corner++)
		{
			verts[gtid * 4 + corner].position = ExpandColliderLine(clip_a, clip_b, scene.display_size_, width, corner);
			verts[gtid * 4 + corner].color = color;
		}

		triangles[gtid * 2 + 0] = uint3(gtid * 4 + 0, gtid * 4 + 1, gtid * 4 + 2);
		triangles[gtid * 2 + 1] = uint3(gtid * 4 + 2, gtid * 4 + 1, gtid * 4 + 3);
	}
}

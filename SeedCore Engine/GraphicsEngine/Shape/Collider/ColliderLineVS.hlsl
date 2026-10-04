#include "ColliderLine.hlsli"

/// [EN] Six vertices per line draw its quad as two triangles; this maps each vertex to a quad corner (0/1 at the a end, 2/3 at the b end).
/// [JP] 1本の線につき頂点6つで、四角形を三角形2つとして描く。各頂点を四角形の隅(a 端が 0/1、b 端が 2/3)へ対応させる。
static const uint quad_corners[6] = { 0, 1, 2, 2, 1, 3 };

ColliderLineMSOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	ColliderConstantBuffer collider = GetColliderConstantBuffer();

	ColliderLineMSOutput output = (ColliderLineMSOutput)0;
	output.position = float4(2.0, 2.0, 2.0, 1.0);

	if (instance_id >= collider.instance_count_)
	{
		return output;
	}

	StructuredBuffer<ColliderStructuredBuffer> instances = GetColliderStructuredBuffer(collider.instance_buffer_index_);
	ColliderStructuredBuffer instance = instances[instance_id];

	uint line_index = vertex_id / 6;
	if (line_index >= GetColliderLineCount(instance.shape_kind_))
	{
		return output;
	}

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

	float width = silhouette ? COLLIDER_SILHOUETTE_LINE_WIDTH : COLLIDER_LINE_WIDTH;

	output.position = ExpandColliderLine(clip_a, clip_b, scene.display_size_, width, quad_corners[vertex_id % 6]);
	output.color = GetColliderLineColor(instance.color_, silhouette);

	return output;
}

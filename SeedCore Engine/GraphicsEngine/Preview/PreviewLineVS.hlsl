#include <GraphicsEngine/Preview/Preview.hlsli>

/**
* [EN]
* Preview.Draw.Line for devices without mesh shaders: one DrawInstanced
* instance per line of Preview.Lines, 6 vertices (two triangles) each,
* placed exactly like PreviewLineMS.
*
* ---------------------------------------------------------------------
*
* [JP]
* メッシュシェーダーが使えないデバイス向けの Preview.Draw.Line。Preview.Lines の
* 1 本につき DrawInstanced の 1 インスタンス、6 頂点（2 三角形）。配置は
* PreviewLineMS と同じ。
*/
PreviewLineOutput main(uint vertex_id : SV_VertexID, uint instance_id : SV_InstanceID)
{
	StructuredBuffer<PreviewLineStructuredBuffer> lines = ResourceDescriptorHeap[line_bindings.preview_lines_];
	PreviewLineStructuredBuffer line_data = lines[instance_id];
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	float4 clip_start = mul(float4(line_data.start_, 1.0), scene.current_view_projection_);
	float4 clip_end = mul(float4(line_data.end_, 1.0), scene.current_view_projection_);

	PreviewLineOutput output;
	output.position = float4(2.0, 2.0, 2.0, 1.0);
	output.color = line_data.color_;
	if (clip_start.w <= 0.0 || clip_end.w <= 0.0)
	{
		return output;
	}

	float2 half_size = scene.screen_size_ * 0.5;
	float2 screen_start = clip_start.xy / clip_start.w * half_size;
	float2 screen_end = clip_end.xy / clip_end.w * half_size;
	float2 direction = screen_end - screen_start;
	direction = dot(direction, direction) > 1e-8 ? normalize(direction) : float2(1.0, 0.0);
	float2 side = float2(-direction.y, direction.x) * line_data.width_ * 0.5 / half_size;

	uint corners[6] = { 0, 1, 2, 1, 3, 2 };
	uint corner_index = corners[vertex_id % 6];
	float4 clip = (corner_index & 1) != 0 ? clip_end : clip_start;
	clip.xy += side * ((corner_index & 2) != 0 ? -1.0 : 1.0) * clip.w;
	output.position = clip;
	return output;
}

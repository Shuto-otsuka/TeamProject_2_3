#include <GraphicsEngine/Preview/Preview.hlsli>

/**
* [EN]
* Preview.Draw.Line: one thread group draws 32 lines of Preview.Lines, each
* a quad of 4 vertices and 2 triangles widened to width_ pixels across the
* screen-space direction of the line. A line with an end behind the camera is
* placed outside the clip volume.
*
* ---------------------------------------------------------------------
*
* [JP]
* Preview.Draw.Line。1 スレッドグループが Preview.Lines の 32 本を描く。各線は
* 4 頂点・2 三角形の四角形で、画面上の線の向きと直交する方向へ width_ ピクセル
* 分だけ広げる。端がカメラの後ろにある線はクリップ空間の外へ置く。
*/
[NumThreads(128, 1, 1)]
[OutputTopology("triangle")]
void main(uint gtid : SV_GroupThreadID, uint gid : SV_GroupID, out vertices PreviewLineOutput verts[128], out indices uint3 tris[64])
{
	StructuredBuffer<PreviewLineStructuredBuffer> lines = ResourceDescriptorHeap[line_bindings.preview_lines_];
	uint line_total;
	uint stride;
	lines.GetDimensions(line_total, stride);
	uint first = gid * 32;
	uint line_count = first < line_total ? min(32, line_total - first) : 0;
	SetMeshOutputCounts(line_count * 4, line_count * 2);
	if (line_count == 0)
	{
		return;
	}

	uint line_local = gtid / 4;
	if (line_local < line_count)
	{
		PreviewLineStructuredBuffer line_data = lines[first + line_local];
		SceneConstantBuffer scene = GetSceneConstantBuffer();
		float4 clip_start = mul(float4(line_data.start_, 1.0), scene.current_view_projection_);
		float4 clip_end = mul(float4(line_data.end_, 1.0), scene.current_view_projection_);

		PreviewLineOutput output;
		output.position = float4(2.0, 2.0, 2.0, 1.0);
		output.color = line_data.color_;
		if (clip_start.w > 0.0 && clip_end.w > 0.0)
		{
			float2 half_size = scene.screen_size_ * 0.5;
			float2 screen_start = clip_start.xy / clip_start.w * half_size;
			float2 screen_end = clip_end.xy / clip_end.w * half_size;
			float2 direction = screen_end - screen_start;
			direction = dot(direction, direction) > 1e-8 ? normalize(direction) : float2(1.0, 0.0);
			float2 side = float2(-direction.y, direction.x) * line_data.width_ * 0.5 / half_size;

			uint corner_index = gtid % 4;
			float4 clip = (corner_index & 1) != 0 ? clip_end : clip_start;
			clip.xy += side * ((corner_index & 2) != 0 ? -1.0 : 1.0) * clip.w;
			output.position = clip;
		}
		verts[gtid] = output;
	}

	if (gtid < line_count)
	{
		uint base = gtid * 4;
		tris[gtid * 2] = uint3(base, base + 1, base + 2);
		tris[gtid * 2 + 1] = uint3(base + 1, base + 3, base + 2);
	}
}

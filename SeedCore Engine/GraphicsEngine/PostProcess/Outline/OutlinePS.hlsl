#include <GraphicsEngine/Shape/Fullscreen/Fullscreen.hlsli>
#include <GraphicsEngine/PostProcess/Outline/Outline.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct OutlineComposeBindings
{
	uint outline_source_;
	uint outline_widest_;
};
ConstantBuffer<OutlineComposeBindings> bindings : register(b0, space1);

/**
* [EN]
* Outline.Compose: draws the outlines fullscreen onto HUD.Display
* with alpha blending, at display resolution. A pixel outside every
* outlined actor looks for the nearest outlined pixel within the widest
* outline on screen and, when that pixel's own width reaches this far,
* takes its color - so each actor gets a band of its own width just
* outside its visible silhouette, including where it meets an occluder in
* front of it. A pixel inside an actor that follows its shape looks for an
* inner edge of the same actor within half its width and takes the
* actor's color there; every other pixel inside an actor is left alone.
* Outline.Source is render resolution, so every lookup scales the display
* pixel by the ratio of the two sizes, and widths stay in display pixels.
*
* ---------------------------------------------------------------------
*
* [JP]
* Outline.Compose。縁取りを HUD.Display へ、表示解像度でフルスクリーンに
* アルファブレンドして描く。どの縁取るアクターにも入っていないピクセルは、画面上で
* 一番太い縁取りの範囲内で一番近い縁取るピクセルを探し、そのピクセル自身の幅が
* ここまで届いていれば、その色を使う。こうして各アクターの、見えているシルエットの
* すぐ外側に、それぞれの幅の帯ができる（手前に遮蔽物がある所も含む）。形に沿う
* アクターの内側のピクセルは、幅の半分の範囲で同じアクターの内側の境目を探し、
* 見つかればアクターの色を使う。それ以外のアクターの内側のピクセルはそのまま残す。
* Outline.Source は描画解像度なので、参照のたびに表示上のピクセルを 2 つの大きさの
* 比でスケールする。幅は表示解像度のピクセル数のまま扱う。
*/
float4 main(FullscreenOutput input) : SV_Target0
{
	StructuredBuffer<uint> widest_buffer = ResourceDescriptorHeap[bindings.outline_widest_];
	float widest = asfloat(widest_buffer[0]);
	if (widest <= 0.0)
	{
		discard;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture2D<uint4> source = ResourceDescriptorHeap[bindings.outline_source_];
	uint source_width;
	uint source_height;
	source.GetDimensions(source_width, source_height);
	float2 display_to_source = float2(source_width, source_height) / scene.display_size_;
	int2 source_last = int2(source_width, source_height) - 1;
	int2 display_pixel = int2(input.position.xy);

	uint4 own = source.Load(int3(clamp(int2((float2(display_pixel) + 0.5) * display_to_source), int2(0, 0), source_last), 0));
	bool covered = own.z != 0;
	if (covered && (own.w & OUTLINE_SOURCE_CONTOUR) == 0)
	{
		discard;
	}

	float reach = covered ? asfloat(own.y) * 0.5 : widest;
	int search_radius = (int)ceil(reach);
	bool found = false;
	float nearest_distance = reach;
	uint nearest_color = 0;
	for (int offset_y = -search_radius; offset_y <= search_radius; offset_y++)
	{
		for (int offset_x = -search_radius; offset_x <= search_radius; offset_x++)
		{
			float offset_distance = length(float2(offset_x, offset_y));
			if (offset_distance > reach || (found && offset_distance >= nearest_distance))
			{
				continue;
			}
			uint4 candidate = source.Load(int3(clamp(int2((float2(display_pixel + int2(offset_x, offset_y)) + 0.5) * display_to_source), int2(0, 0), source_last), 0));
			bool hit = covered ? candidate.z == own.z && (candidate.w & OUTLINE_SOURCE_EDGE) != 0 : candidate.z != 0 && offset_distance <= asfloat(candidate.y);
			if (hit)
			{
				found = true;
				nearest_distance = offset_distance;
				nearest_color = candidate.x;
			}
		}
	}
	if (!found)
	{
		discard;
	}
	return float4((nearest_color >> uint4(0, 8, 16, 24)) & 0xFF) / 255.0;
}

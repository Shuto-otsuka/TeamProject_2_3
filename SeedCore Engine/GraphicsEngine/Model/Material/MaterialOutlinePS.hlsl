#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Scene.hlsli"
#include "MaterialOutline.hlsli"

struct MaterialOutlineInput
{
	float4 position : SV_Position;
	float2 texcoord : TEXCOORD0;
};

/**
* [EN]
* Material outline composite, drawn fullscreen onto the tone-mapped display
* texture with alpha blending. A pixel inside an outlined mesh is left
* alone; any other pixel looks for the nearest outlined pixel within the
* widest outline on screen and, when that pixel's own outline width reaches
* this far, takes its outline color - so each mesh gets a band of its own
* width just outside its visible silhouette, including where it meets an
* occluder in front of it. The resolve texture is native resolution while
* this pass draws at display resolution, so every lookup scales the display
* pixel by screen_size_/display_size_ (ratio 1 when the two match), and
* widths stay in display pixels.
*
* ---------------------------------------------------------------------
*
* [JP]
* マテリアルのアウトラインの合成。トーンマップ済みの表示用テクスチャへ、
* フルスクリーンでアルファブレンドして描く。縁取るメッシュの内側の
* ピクセルはそのまま残し、それ以外のピクセルは、画面上で一番太い縁取りの
* 範囲内で一番近い縁取りのピクセルを探し、そのピクセル自身の縁取りの幅が
* ここまで届いていれば、その縁取りの色を使う。こうして各メッシュの、
* 見えているシルエットのすぐ外側に、それぞれの幅の帯ができる(手前に
* 遮蔽物がある所も含む)。resolve テクスチャはネイティブ解像度で、この
* パスは表示解像度で描くので、参照のたびに表示上のピクセルを
* screen_size_/display_size_ でスケールする(両者が同じなら比率 1)。
* 幅は表示解像度のピクセル数のまま扱う。
*/
float4 main(MaterialOutlineInput input) : SV_Target0
{
	MaterialOutlineConstantBuffer constants = GetMaterialOutlineConstantBuffer();

	/// [EN] Widest outline on screen this frame; 0 means nothing is outlined, so the whole pass is a no-op.
	/// [JP] このフレームの画面上で一番太い縁取り。0 なら縁取る物が無いので、このパスは何もしない。
	StructuredBuffer<uint> max_width_buffer = ResourceDescriptorHeap[constants.max_width_shader_resource_index_];
	float max_width = asfloat(max_width_buffer[0]);
	if (max_width <= 0.0)
	{
		discard;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture2D<uint2> resolve = ResourceDescriptorHeap[constants.resolve_shader_resource_index_];

	/// [EN] Display-to-native scale and the last valid native pixel, for clamping lookups near the screen edge.
	/// [JP] 表示からネイティブへのスケールと、画面端付近の参照を収めるための、有効な最後のネイティブのピクセル。
	float2 display_to_native = scene.screen_size_ / scene.display_size_;
	int2 native_last = int2(scene.screen_size_) - 1;
	int2 display_pixel = int2(input.position.xy);

	/// [EN] Pixels inside an outlined mesh keep the scene color; the band is drawn only outside it.
	/// [JP] 縁取るメッシュの内側のピクセルはシーンの色のまま残す。帯はその外側にだけ描く。
	int2 own_native_pixel = clamp(int2((float2(display_pixel) + 0.5) * display_to_native), int2(0, 0), native_last);
	if (asfloat(resolve.Load(int3(own_native_pixel, 0)).y) > 0.0)
	{
		discard;
	}

	int search_radius = (int)ceil(max_width);

	/// [EN] Nearest outlined pixel whose own width reaches this pixel.
	/// [JP] 自身の幅がこのピクセルまで届く、一番近い縁取りのピクセル。
	bool found = false;
	float nearest_distance = max_width;
	uint nearest_color = 0;

	for (int offset_y = -search_radius; offset_y <= search_radius; offset_y++)
	{
		for (int offset_x = -search_radius; offset_x <= search_radius; offset_x++)
		{
			float offset_distance = length(float2(offset_x, offset_y));
			if (offset_distance > max_width || (found && offset_distance >= nearest_distance))
			{
				continue;
			}

			int2 sample_native_pixel = clamp(int2((float2(display_pixel + int2(offset_x, offset_y)) + 0.5) * display_to_native), int2(0, 0), native_last);
			uint2 sample_outline = resolve.Load(int3(sample_native_pixel, 0));
			float sample_width = asfloat(sample_outline.y);

			if (sample_width > 0.0 && offset_distance <= sample_width)
			{
				found = true;
				nearest_distance = offset_distance;
				nearest_color = sample_outline.x;
			}
		}
	}

	if (!found)
	{
		discard;
	}

	/// [EN] Unpack the RGBA8 color (R in the lowest byte); its alpha drives the blend.
	/// [JP] RGBA8 の色を戻す(最下位バイトが R)。そのアルファでブレンドする。
	return float4((nearest_color >> uint4(0, 8, 16, 24)) & 0xFF) / 255.0;
}

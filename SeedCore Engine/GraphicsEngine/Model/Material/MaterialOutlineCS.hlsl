#include "../Model.hlsli"
#include "../../Shader/ShaderResources.hlsli"
#include "../../Shader/Scene.hlsli"
#include "MaterialOutline.hlsli"

/**
* [EN]
* Empties the transparent record before the transparent pass writes this
* frame's outlined fragments into it; 0 means "no transparent fragment".
*
* ---------------------------------------------------------------------
*
* [JP]
* 半透明のパスがこのフレームの縁取りのあるフラグメントを書き込む前に、
* 半透明の記録を空にする。0 は「半透明のフラグメント無し」を表す。
*/
[numthreads(8, 8, 1)]
void ClearTransparent(uint3 dtid : SV_DispatchThreadID)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	if (dtid.x >= (uint)scene.screen_size_.x || dtid.y >= (uint)scene.screen_size_.y)
	{
		return;
	}

	RWTexture2D<uint> transparent = ResourceDescriptorHeap[GetMaterialOutlineConstantBuffer().transparent_unordered_access_index_];
	transparent[dtid.xy] = 0;
}

/**
* [EN]
* Clears the max-width buffer before Resolve accumulates into it. A single
* thread is enough because the buffer is one element.
*
* ---------------------------------------------------------------------
*
* [JP]
* Resolve が最大値を積み上げる前に、最大幅のバッファを 0 に戻す。バッファは
* 要素 1 つなので、スレッドも 1 つで足りる。
*/
[numthreads(1, 1, 1)]
void ResetMaxWidth()
{
	RWStructuredBuffer<uint> max_width = ResourceDescriptorHeap[GetMaterialOutlineConstantBuffer().max_width_unordered_access_index_];
	max_width[0] = 0;
}

/**
* [EN]
* Writes, for every native-resolution pixel, the outline of the material
* covering it, from the instance's shading (the material's packed outline
* color and width). The instance comes from the transparent record when
* the transparent pass left one there - it passed the depth test, so it
* lies in front of the opaque surface - and from the VisibilityBuffer id
* otherwise. Background pixels (reverse-Z depth 0) with no transparent
* record get width 0. The widest width on screen
* is reduced across the wave first and then into the max-width buffer, so
* only one atomic per wave reaches memory; widths are non-negative floats,
* whose bit patterns order the same way as the values, so an unsigned
* atomic max over asuint() is a float max.
*
* ---------------------------------------------------------------------
*
* [JP]
* ネイティブ解像度の各ピクセルに、そこを覆うマテリアルの縁取りを、
* インスタンスのシェーディングの値(マテリアルの縁取りの色(詰めたもの)と
* 幅)から書く。インスタンスは、半透明のパスが記録を残していればそれを
* 使い(深度テストを通っているので不透明の面より手前にある)、無ければ
* VisibilityBuffer の id から取る。半透明の記録も無い背景(reverse-Z の
* 深度 0)は幅 0 になる。画面上で一番太い幅は、まずウェーブ内で最大を取ってから
* 最大幅のバッファへ反映するので、メモリへのアトミック操作はウェーブ
* ごとに 1 回で済む。幅は負にならない float で、そのビット列は値と同じ
* 順に並ぶため、asuint() に対する符号なしのアトミック最大がそのまま
* float の最大になる。
*/
[numthreads(8, 8, 1)]
void Resolve(uint3 dtid : SV_DispatchThreadID)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	bool inside =dtid.x < (uint)scene.screen_size_.x && dtid.y < (uint)scene.screen_size_.y;

	/// [EN] Outline of the material covering this pixel; stays empty for background and for threads past the screen edge.
	/// [JP] このピクセルを覆うマテリアルの縁取り。背景と、画面の外にはみ出したスレッドでは空のまま。
	uint outline_color = 0;
	float outline_width = 0.0;

	if (inside)
	{
		RWTexture2D<uint> transparent = ResourceDescriptorHeap[GetMaterialOutlineConstantBuffer().transparent_unordered_access_index_];
		uint transparent_record = transparent[dtid.xy];

		Texture2D<float> depth_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.depth_index_];
		float depth = depth_texture.Load(int3(dtid.xy, 0));

		/// [EN] Instance whose material outlines this pixel: the nearest outlined transparent fragment first, then the opaque surface.
		/// [JP] このピクセルを縁取るマテリアルのインスタンス。一番手前の縁取りのある半透明のフラグメントを優先し、無ければ不透明の面。
		bool has_instance = false;
		uint instance_index = 0;

		if (transparent_record != 0)
		{
			has_instance = true;
			instance_index = (transparent_record & 0xFFFF) - 1;
		}
		else if (depth != 0.0)
		{
			Texture2D<uint4> visibility_texture = ResourceDescriptorHeap[shader_resource_indices.geometry_buffer_.index_4_];
			uint4 visibility_id = visibility_texture.Load(int3(dtid.xy, 0));

			uint meshlet_index;
			uint triangle_in_meshlet_index;
			UnpackVisibilityID(visibility_id, instance_index, meshlet_index, triangle_in_meshlet_index);
			has_instance = true;
		}

		if (has_instance)
		{
			StructuredBuffer<ModelStructuredBuffer> instances = GetModelStructuredBuffer(shader_resource_indices.model_.instance_index_);
			ModelShading shading = instances[instance_index].shading_;

			outline_color = shading.outline_color_;
			outline_width = max(shading.outline_width_, 0.0);
		}

		RWTexture2D<uint2> resolve = ResourceDescriptorHeap[GetMaterialOutlineConstantBuffer().resolve_unordered_access_index_];
		resolve[dtid.xy] = uint2(outline_color, asuint(outline_width));
	}

	/// [EN] Wave-wide maximum first, so only the first lane of each wave issues the atomic.
	/// [JP] 先にウェーブ全体で最大を取り、アトミック操作は各ウェーブの先頭レーンだけが行う。
	float wave_max_width = WaveActiveMax(outline_width);
	if (WaveIsFirstLane() && wave_max_width > 0.0)
	{
		RWStructuredBuffer<uint> max_width = ResourceDescriptorHeap[GetMaterialOutlineConstantBuffer().max_width_unordered_access_index_];
		InterlockedMax(max_width[0], asuint(wave_max_width));
	}
}

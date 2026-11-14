#ifndef __SCENE_HLSL__
#define __SCENE_HLSL__

#include <GraphicsEngine/Shader/RootSignature.hlsli>

struct SceneConstantBuffer
{
	row_major float4x4 view_;
	row_major float4x4 inverse_view_;

	row_major float4x4 projection_;
	row_major float4x4 inverse_projection_;
	row_major float4x4 non_jitter_projection_;

	row_major float4x4 current_view_projection_;
	row_major float4x4 previous_view_projection_;
	row_major float4x4 inverse_view_projection_;
	row_major float4x4 non_jitter_view_projection_;
	row_major float4x4 previous_non_jitter_view_projection_;

	float4 camera_position_;
	float4 camera_focus_;

	float field_of_view_;
	float near_plane_;
	float far_plane_;
	uint view_mode_;

	float total_time_;
	float delta_time_;

	// Resolution of the view's display output, which the post-tonemap passes (overlays, outlines) draw at - screen_size_ when nothing upscales, larger while an upscaler is active. Outline.Compose scales SV_Position by the render-to-display ratio before reading the render-resolution Outline.Source.
	float2 display_size_;

	float2 screen_size_;
	float2 inverse_screen_size_;

	uint show_flags_low_;
	uint show_flags_high_;
	uint debug_source_;
	uint scene_constant_buffer_padding_;
};

ConstantBuffer<SceneConstantBuffer> GetSceneConstantBuffer()
{
	return ResourceDescriptorHeap[view_bindings.scene_];
}

#endif // __SCENE_HLSL__

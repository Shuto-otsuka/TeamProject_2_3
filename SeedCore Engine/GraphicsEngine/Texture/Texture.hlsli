#ifndef __TEXTURE_HLSL__
#define __TEXTURE_HLSL__

#define TEXTURE_KIND_TEXTURE 0
#define TEXTURE_KIND_FONT 1
#define TEXTURE_KIND_MOVIE 2

#define TEXTURE_DISPLAY_SPRITE 0
#define TEXTURE_DISPLAY_BILLBOARD 1
#define TEXTURE_DISPLAY_FULLSCREEN 2

#define TEXTURE_UNIT_DRAWN 1
#define TEXTURE_UNIT_SELECTED 2
#define TEXTURE_UNIT_FACE_CAMERA 4
#define TEXTURE_UNIT_SCROLL 8

#define TEXTURE_QUAD_SHADOW 1

#define TEXTURE_INVALID 0xFFFFFFFF

#define TEXTURE_CANVAS_HEIGHT 1080.0

struct TextureUnitStructuredBuffer
{
	row_major float4x4 world_;

	float4 color_;

	float4 outline_color_;

	float4 glow_color_;

	float4 shadow_color_;

	float2 unit_range_;
	float outline_width_;
	float glow_power_;

	float2 scroll_direction_;
	float scroll_speed_;
	uint texture_index_;

	uint kind_;
	uint display_;
	uint flags_;
	uint quad_offset_;

	uint quad_count_;
	uint draw_base_;
	uint actor_;
	uint texture_unit_padding_0_;
};

struct TextureQuadStructuredBuffer
{
	float2 rect_min_;
	float2 rect_size_;

	float2 uv_min_;
	float2 uv_max_;

	uint unit_;
	uint flags_;
	uint2 texture_quad_padding_0_;
};

struct TextureUnitUpdate
{
	uint index_;
	uint3 texture_unit_update_padding_0_;

	TextureUnitStructuredBuffer data_;
};

struct TextureQuadUpdate
{
	uint index_;
	uint3 texture_quad_update_padding_0_;

	TextureQuadStructuredBuffer data_;
};

struct TextureConstantBuffer
{
	uint sprite_base_;
	uint sprite_count_;
	uint billboard_base_;
	uint billboard_count_;

	uint quad_count_;
	uint3 texture_padding_0_;
};

#endif

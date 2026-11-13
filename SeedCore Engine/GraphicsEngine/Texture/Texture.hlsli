#ifndef __TEXTURE_HLSL__
#define __TEXTURE_HLSL__

// Mirrors the C++ TextureKind / TextureDisplay enums (Texture/TextureDriven.h).
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

// Sprites are laid out on a 1920x1080 canvas and scaled to the view's display.
#define TEXTURE_CANVAS_HEIGHT 1080.0

/**
* [EN]
* One actor's Image, Text or Movie component. world_ is the actor's world
* matrix: sprites use its translation, z rotation and scale on the canvas,
* billboards transform their quads by it. color_ tints everything; the font
* style (outline, glow, shadow, unit_range_ = the atlas distance range in UV)
* only matters to fonts, the scroll only to textures with
* TEXTURE_UNIT_SCROLL. texture_index_ is the image, the font atlas or the
* movie frame. The unit owns quads [quad_offset_, quad_offset_ +
* quad_count_) and, when TEXTURE_UNIT_DRAWN is set, draws them at
* Texture.DrawList[draw_base_ + n].
*/
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

/**
* [EN]
* One quad of a unit. Corner c in {0,1}^2 lies at rect_min_ + c *
* rect_size_ with c = (0,0) at the top-left of uv_min_..uv_max_: sprites
* measure in canvas pixels with y down, billboards in local units with y up
* (so their rect_size_.y is negative). TEXTURE_QUAD_SHADOW marks a text
* shadow, drawn in the shadow colour.
*/
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

/**
* [EN]
* sprite_count_ quads are drawn first from Texture.DrawList[sprite_base_],
* then billboard_count_ from billboard_base_. Texture.Expand walks quad_count_
* quad slots.
*/
struct TextureConstantBuffer
{
	uint sprite_base_;
	uint sprite_count_;
	uint billboard_base_;
	uint billboard_count_;

	uint quad_count_;
	uint3 texture_padding_0_;
};

#endif // __TEXTURE_HLSL__

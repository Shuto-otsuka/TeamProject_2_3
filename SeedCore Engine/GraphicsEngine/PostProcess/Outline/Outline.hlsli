#ifndef __OUTLINE_HLSL__
#define __OUTLINE_HLSL__

#define OUTLINE_STYLE_OUTLINE 1
#define OUTLINE_STYLE_CONTOUR 2
#define OUTLINE_STYLE_SELECTED 4

#define OUTLINE_SOURCE_EDGE 1
#define OUTLINE_SOURCE_CONTOUR 2

struct OutlineStyleStructuredBuffer
{
	uint color_;
	float width_;
	uint flags_;
	uint outline_style_structured_buffer_padding_0_;
};

struct OutlineStyleUpdate
{
	uint index_;
	uint3 outline_style_update_padding_0_;
	OutlineStyleStructuredBuffer data_;
};

#endif

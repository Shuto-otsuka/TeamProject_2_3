#ifndef __OUTLINE_HLSL__
#define __OUTLINE_HLSL__

#define OUTLINE_STYLE_OUTLINE 1
#define OUTLINE_STYLE_CONTOUR 2
#define OUTLINE_STYLE_SELECTED 4

#define OUTLINE_SOURCE_EDGE 1
#define OUTLINE_SOURCE_CONTOUR 2

/**
* [EN]
* How one actor is outlined, indexed by its entity index. color_ is RGBA8
* with R in the lowest byte, width_ is in display pixels, and flags_ holds
* OUTLINE_STYLE_*: OUTLINE_STYLE_OUTLINE when the actor has an Outline
* component (OUTLINE_STYLE_CONTOUR when it also follows the inner edges),
* OUTLINE_STYLE_SELECTED when the editor has it selected. 0 means the actor
* is not outlined at all.
*/
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

#endif // __OUTLINE_HLSL__

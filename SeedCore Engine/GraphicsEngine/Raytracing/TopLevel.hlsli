#ifndef __TOP_LEVEL_HLSLI__
#define __TOP_LEVEL_HLSLI__

struct TopLevelHitStructuredBuffer
{
	uint vertex_buffer_index_;
	uint index_buffer_index_;
	uint surface_buffer_index_;
	uint material_buffer_index_;

	uint material_offset_;
	uint top_level_hit_structured_buffer_padding_0_;
	float2 texcoord_min_;

	float2 texcoord_extent_;
	uint top_level_hit_structured_buffer_padding_1_;
	uint top_level_hit_structured_buffer_padding_2_;
};

struct TopLevelInstanceStructuredBuffer
{
	float4 transform_[3];
	uint2 bottom_level_;
	uint instance_mask_;
	uint flags_;
	TopLevelHitStructuredBuffer hit_;
};

#endif // __TOP_LEVEL_HLSLI__

#ifndef __BOTTOM_LEVEL_HLSLI__
#define __BOTTOM_LEVEL_HLSLI__

struct BottomLevelTableStructuredBuffer
{
	uint2 key_;
	uint2 address_;
	uint vertex_offset_;
	uint bottom_level_table_structured_buffer_padding_0_;
	uint bottom_level_table_structured_buffer_padding_1_;
	uint bottom_level_table_structured_buffer_padding_2_;
};

BottomLevelTableStructuredBuffer BottomLevelFind(StructuredBuffer<BottomLevelTableStructuredBuffer> table, uint2 key)
{
	uint capacity;
	uint stride;
	table.GetDimensions(capacity, stride);

	uint hash = (key.x * 0x9E3779B1u) ^ (key.y * 0x85EBCA77u);
	hash ^= hash >> 16;
	uint slot = hash & (capacity - 1);
	for (uint probe = 0; probe < capacity; probe++)
	{
		BottomLevelTableStructuredBuffer row = table[slot];
		if (all(row.key_ == key))
		{
			return row;
		}
		if (all(row.key_ == 0))
		{
			break;
		}
		slot = (slot + 1) & (capacity - 1);
	}

	BottomLevelTableStructuredBuffer missing = (BottomLevelTableStructuredBuffer)0;
	missing.vertex_offset_ = 0xFFFFFFFF;
	return missing;
}

#endif // __BOTTOM_LEVEL_HLSLI__

#ifndef __DISPATCH_HLSL__
#define __DISPATCH_HLSL__

cbuffer DispatchIndices : register(b4, space1)
{
	uint dispatch_buffer_index_;
};

#endif // __DISPATCH_HLSL__

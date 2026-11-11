#ifndef __CULLING_HLSL__
#define __CULLING_HLSL__

// Layout of CullingDrawArguments (uint elements). The two indirect commands
// sit at the front, indexed by command number with the signature's stride
// (3 uints for DispatchMesh, 4 for the DrawInstanced fallback); the counts
// follow at fixed elements.
#define CULLING_EARLY_ARGUMENTS 0
#define CULLING_LATE_ARGUMENTS 1
#define CULLING_EARLY_COUNT 8
#define CULLING_LATE_COUNT 9
#define CULLING_REJECTED_COUNT 10
#define CULLING_OVERFLOW 11

#endif // __CULLING_HLSL__

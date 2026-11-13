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

// Layout of CullingTransparentArguments (uint elements): the indirect command
// at the front (3 uints for DispatchMesh, 4 for the DrawInstanced fallback),
// then the count of transparent entries and the ones that did not fit.
#define CULLING_TRANSPARENT_COUNT 4
#define CULLING_TRANSPARENT_OVERFLOW 5

#endif // __CULLING_HLSL__

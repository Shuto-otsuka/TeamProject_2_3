#ifndef __CULLING_HLSL__
#define __CULLING_HLSL__

// Layout of Culling.MeshletArguments (uint elements). The two indirect commands
// sit at the front, indexed by command number with the signature's stride
// (3 uints for DispatchMesh, 4 for the DrawInstanced fallback); the counts
// follow at fixed elements.
#define CULLING_EARLY_ARGUMENTS 0
#define CULLING_LATE_ARGUMENTS 1
#define CULLING_EARLY_COUNT 8
#define CULLING_LATE_COUNT 9
#define CULLING_OCCLUDED_COUNT 10
#define CULLING_OVERFLOW 11

// Third draw of the mesh path: the meshlets the early amplification pass hid
// behind the previous Hi-Z, read straight from Geometry.OccludedMeshlets.
#define CULLING_OCCLUDED_ARGUMENTS 2

// The low byte of the dispatch constant of a geometry draw is the command;
// the replay bit redraws the depth passes' decisions without recording any.
#define CULLING_COMMAND_MASK 0xFF
#define CULLING_REPLAY 0x100

// Layout of CullingChunkArguments (uint elements): the early and late
// DispatchMesh commands of the amplification path at the front (3 uints
// each), then the counts of Culling.OpaqueChunks and Culling.OccludedChunks.
#define CULLING_OPAQUE_CHUNK_COUNT 8
#define CULLING_OCCLUDED_CHUNK_COUNT 9

// Layout of CullingTransparentArguments (uint elements): the indirect command
// at the front (3 uints for DispatchMesh, 4 for the DrawInstanced fallback),
// then the count of transparent entries and the ones that did not fit.
#define CULLING_TRANSPARENT_COUNT 4
#define CULLING_TRANSPARENT_OVERFLOW 5

#endif // __CULLING_HLSL__

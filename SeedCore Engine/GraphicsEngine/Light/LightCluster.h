#ifndef __LIGHT_CLUSTER_H__
#define __LIGHT_CLUSTER_H__

// Shared by C++ and HLSL. The view is cut into screen tiles of
// LIGHT_CLUSTER_TILE_SIZE pixels and LIGHT_CLUSTER_DEPTH_SLICES logarithmic
// depth slices; each cluster of LightClusterGrid holds its light count
// followed by up to LIGHT_CLUSTER_MAX_LIGHTS indices into LightList.
#define LIGHT_CLUSTER_TILE_SIZE 64
#define LIGHT_CLUSTER_DEPTH_SLICES 16
#define LIGHT_CLUSTER_MAX_LIGHTS 128

#endif // __LIGHT_CLUSTER_H__

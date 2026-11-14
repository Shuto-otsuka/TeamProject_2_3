#include <GraphicsEngine/Model/Opaque/Geometry.hlsli>

/**
* [EN]
* Wireframe view mode: GeometryMS rasterised with a wireframe rasterizer state
* over a cleared HUD.Display, every edge drawn in a color hashed from its
* instance so neighbouring objects stay apart.
*/
float4 Wireframe(GeometryOutput input, GeometryPrimitive primitive) : SV_Target0
{
	uint hash = (input.instance_index + 1) * 747796405u + 2891336453u;
	hash = ((hash >> ((hash >> 28) + 4)) ^ hash) * 277803737u;
	hash = (hash >> 22) ^ hash;
	return float4(lerp(float3(0.35, 0.35, 0.35), float3(hash & 0xFF, (hash >> 8) & 0xFF, (hash >> 16) & 0xFF) / 255.0, 0.75), 1.0);
}

/**
* [EN]
* Meshlet overdraw view mode: GeometryMS rasterised again with depth testing
* off, every fragment adding one to its pixel in Geometry.Debug, so the
* resolve can show how many times each pixel was covered.
*/
void Overdraw(GeometryOutput input, GeometryPrimitive primitive)
{
	RWTexture2D<uint> overdraw = ResourceDescriptorHeap[bindings.geometry_debug_];
	InterlockedAdd(overdraw[uint2(input.position.xy)], 1);
}

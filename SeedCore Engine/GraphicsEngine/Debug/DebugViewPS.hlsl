#include <GraphicsEngine/Model/Opaque/Geometry.hlsli>

float4 Wireframe(GeometryOutput input, GeometryPrimitive primitive) : SV_Target0
{
	uint hash = (input.instance_index + 1) * 747796405u + 2891336453u;
	hash = ((hash >> ((hash >> 28) + 4)) ^ hash) * 277803737u;
	hash = (hash >> 22) ^ hash;
	return float4(lerp(float3(0.35, 0.35, 0.35), float3(hash & 0xFF, (hash >> 8) & 0xFF, (hash >> 16) & 0xFF) / 255.0, 0.75), 1.0);
}

void Overdraw(GeometryOutput input, GeometryPrimitive primitive)
{
	RWTexture2D<uint> overdraw = ResourceDescriptorHeap[bindings.geometry_debug_];
	InterlockedAdd(overdraw[uint2(input.position.xy)], 1);
}

#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLight.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct RaytracingVolumetricLightInjectBindings
{
	uint raytracing_volumetric_light_constants_;
	uint raytracing_volumetric_light_density_;
};
ConstantBuffer<RaytracingVolumetricLightInjectBindings> bindings : register(b0, space1);

[numthreads(4, 4, 4)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture3D<float4> density_volume = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_density_];
	uint3 froxel_dimensions;
	density_volume.GetDimensions(froxel_dimensions.x, froxel_dimensions.y, froxel_dimensions.z);
	if (any(id >= froxel_dimensions))
	{
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingVolumetricLightConstantBuffer> constants = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_constants_];
	float3 world_position = RaytracingVolumetricLightFroxelToWorld(float3(id) + 0.5 + constants.jitter_, froxel_dimensions, scene.near_plane_, scene.far_plane_, scene.projection_, scene.inverse_view_);
	float density = constants.density_ * exp(-constants.height_falloff_ * max(world_position.y - constants.height_reference_, 0.0));
	density_volume[id] = float4(density * constants.fog_albedo_, density + constants.absorption_);
}

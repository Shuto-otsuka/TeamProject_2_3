#include <GraphicsEngine/Illumination/VolumetricLight/Raytracing/RaytracingVolumetricLight.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>

struct RaytracingVolumetricLightIntegrateBindings
{
	uint raytracing_volumetric_light_scattering_;
	uint raytracing_volumetric_light_fog_volume_;
};
ConstantBuffer<RaytracingVolumetricLightIntegrateBindings> bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void main(uint3 id : SV_DispatchThreadID)
{
	RWTexture3D<float4> fog_volume = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_fog_volume_];
	uint3 froxel_dimensions;
	fog_volume.GetDimensions(froxel_dimensions.x, froxel_dimensions.y, froxel_dimensions.z);
	if (id.x >= froxel_dimensions.x || id.y >= froxel_dimensions.y)
	{
		return;
	}

	if (dispatch_bindings.constants_ == 0)
	{
		for (uint clear_slice = 0; clear_slice < froxel_dimensions.z; clear_slice++)
		{
			fog_volume[uint3(id.xy, clear_slice)] = float4(0.0, 0.0, 0.0, 1.0);
		}
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	Texture3D<float4> scattering_volume = ResourceDescriptorHeap[bindings.raytracing_volumetric_light_scattering_];
	float3 accumulated_scattering = float3(0.0, 0.0, 0.0);
	float transmittance = 1.0;
	for (uint slice = 0; slice < froxel_dimensions.z; slice++)
	{
		float4 cell = scattering_volume.Load(int4(id.xy, slice, 0));
		float extinction = cell.a;
		float slice_depth = FroxelSliceToViewZ(float(slice + 1) / float(froxel_dimensions.z), scene.near_plane_, scene.far_plane_) - FroxelSliceToViewZ(float(slice) / float(froxel_dimensions.z), scene.near_plane_, scene.far_plane_);
		float slice_transmittance = exp(-extinction * slice_depth);
		accumulated_scattering += transmittance * (cell.rgb - cell.rgb * slice_transmittance) / max(extinction, 0.00001);
		transmittance *= slice_transmittance;
		fog_volume[uint3(id.xy, slice)] = float4(accumulated_scattering, transmittance);
	}
}

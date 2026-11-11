#ifndef __IMAGE_BASED_LIGHTING_HLSL__
#define __IMAGE_BASED_LIGHTING_HLSL__

// Bindless IBL sampling. The environment / irradiance / prefilter cubes and the
// BRDF lookup table are passed in as descriptor indices (SkyCubemap,
// DiffuseIrradiance, SpecularPrefiltered and BrdfLut of the Sky feature).

#include <GraphicsEngine/Shader/Sampler.hlsli>

float4 SampleGgxLookupTable(uint brdf_lut, float2 sample_point)
{
	Texture2D<float2> ggx_lookup_table = ResourceDescriptorHeap[brdf_lut];
	return float4(ggx_lookup_table.Sample(sampler_linear_clamp, sample_point), 0.0, 0.0);
}

float4 SampleDiffuseIrradianceEnvironmentMap(uint diffuse_irradiance_cube, float3 direction)
{
	TextureCube<float4> diffuse_irradiance = ResourceDescriptorHeap[diffuse_irradiance_cube];
	return diffuse_irradiance.Sample(sampler_linear_clamp, direction);
}

float4 SampleSpecularPrefilteredRadianceEnvironmentMap(uint specular_prefiltered_cube, float3 direction, float roughness)
{
	TextureCube<float4> specular_prefiltered = ResourceDescriptorHeap[specular_prefiltered_cube];

	uint width, height, number_of_levels;
	specular_prefiltered.GetDimensions(0, width, height, number_of_levels);

	float lod = roughness * float(number_of_levels - 1);
	return specular_prefiltered.SampleLevel(sampler_linear_clamp, direction, lod);
}

// Environment cube sampled for the skybox background (mip 0).
float4 SampleSkyboxEnvironment(uint sky_cubemap, float3 direction)
{
	TextureCube<float4> environment = ResourceDescriptorHeap[sky_cubemap];
	return environment.SampleLevel(sampler_linear_clamp, direction, 0);
}

#endif // __IMAGE_BASED_LIGHTING_HLSL__

#ifndef __MATERIAL_MAPPING_HLSL__
#define __MATERIAL_MAPPING_HLSL__

#include <GraphicsEngine/Model/Model.hlsli>

struct MaterialMappingCoordinate
{
    float2 texcoord_x_;
    float2 texcoord_y_;
    float2 texcoord_z_;
    float3 weight_;
    float3 projection_normal_;
};

MaterialMappingCoordinate ResolveMaterialMapping(uint material_mapping, float tiling, float triplanar_sharpness, float2 texcoord, float3 object_position, float3 object_normal, float3 object_tangent, float3 object_bitangent, float3 object_scale, float3 world_position, float3 world_normal)
{
    MaterialMappingCoordinate coordinate;
    coordinate.texcoord_x_ = texcoord;
    coordinate.texcoord_y_ = texcoord;
    coordinate.texcoord_z_ = texcoord;
    coordinate.weight_ = float3(1.0, 0.0, 0.0);
    coordinate.projection_normal_ = world_normal;

    if (material_mapping == MATERIAL_MAPPING_TILE)
    {
        float tangent_length = length(object_tangent);
        float bitangent_length = length(object_bitangent);
        float tile_u = tangent_length > 1e-6 ? length(object_tangent * object_scale) / tangent_length : 1.0;
        float tile_v = bitangent_length > 1e-6 ? length(object_bitangent * object_scale) / bitangent_length : 1.0;
        coordinate.texcoord_x_ = texcoord * float2(tile_u, tile_v) * tiling;
    }
    else if (material_mapping == MATERIAL_MAPPING_TRIPLANAR_OBJECT || material_mapping == MATERIAL_MAPPING_TRIPLANAR_WORLD)
    {
        float3 position = world_position;
        float3 normal = world_normal;
        if (material_mapping == MATERIAL_MAPPING_TRIPLANAR_OBJECT)
        {
            position = object_position * object_scale;
            normal = normalize(object_normal / object_scale);
        }
        position *= tiling;

        coordinate.texcoord_x_ = position.zy;
        coordinate.texcoord_y_ = position.xz;
        coordinate.texcoord_z_ = position.xy;

        float3 weight = pow(abs(normal), triplanar_sharpness);
        coordinate.weight_ = weight / (weight.x + weight.y + weight.z);
        coordinate.projection_normal_ = normal;
    }

    return coordinate;
}

float4 SampleMaterialMappingGrad(Texture2D source_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate)
{
    float2 ddx_x = ddx(coordinate.texcoord_x_);
    float2 ddy_x = ddy(coordinate.texcoord_x_);
    float2 ddx_y = ddx(coordinate.texcoord_y_);
    float2 ddy_y = ddy(coordinate.texcoord_y_);
    float2 ddx_z = ddx(coordinate.texcoord_z_);
    float2 ddy_z = ddy(coordinate.texcoord_z_);

    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    if (coordinate.weight_.x > 0.0)
    {
        color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_x_, ddx_x, ddy_x) * coordinate.weight_.x;
    }
    if (coordinate.weight_.y > 0.0)
    {
        color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_y_, ddx_y, ddy_y) * coordinate.weight_.y;
    }
    if (coordinate.weight_.z > 0.0)
    {
        color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_z_, ddx_z, ddy_z) * coordinate.weight_.z;
    }
    return color;
}

/**
* [EN]
* Same as above for compute shaders, which have no ddx/ddy: the derivatives
* are the differences to the mapping coordinates of the pixel one step right
* (coordinate_x) and one step down (coordinate_y).
*/
float4 SampleMaterialMappingGrad(Texture2D source_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate, MaterialMappingCoordinate coordinate_x, MaterialMappingCoordinate coordinate_y)
{
	float4 color = float4(0.0, 0.0, 0.0, 0.0);
	if (coordinate.weight_.x > 0.0)
	{
		color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_x_, coordinate_x.texcoord_x_ - coordinate.texcoord_x_, coordinate_y.texcoord_x_ - coordinate.texcoord_x_) * coordinate.weight_.x;
	}
	if (coordinate.weight_.y > 0.0)
	{
		color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_y_, coordinate_x.texcoord_y_ - coordinate.texcoord_y_, coordinate_y.texcoord_y_ - coordinate.texcoord_y_) * coordinate.weight_.y;
	}
	if (coordinate.weight_.z > 0.0)
	{
		color += source_texture.SampleGrad(texture_sampler, coordinate.texcoord_z_, coordinate_x.texcoord_z_ - coordinate.texcoord_z_, coordinate_y.texcoord_z_ - coordinate.texcoord_z_) * coordinate.weight_.z;
	}
	return color;
}

float3 SampleNormalMappingGrad(Texture2D normal_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate)
{
    float2 ddx_x = ddx(coordinate.texcoord_x_);
    float2 ddy_x = ddy(coordinate.texcoord_x_);
    float2 ddx_y = ddx(coordinate.texcoord_y_);
    float2 ddy_y = ddy(coordinate.texcoord_y_);
    float2 ddx_z = ddx(coordinate.texcoord_z_);
    float2 ddy_z = ddy(coordinate.texcoord_z_);

    float3 normal = coordinate.projection_normal_;
    float3 result = float3(0.0, 0.0, 0.0);
    if (coordinate.weight_.x > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_x_, ddx_x, ddy_x).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.zy, abs(tangent_normal.z) * normal.x);
        result += blended.zyx * coordinate.weight_.x;
    }
    if (coordinate.weight_.y > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_y_, ddx_y, ddy_y).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.xz, abs(tangent_normal.z) * normal.y);
        result += blended.xzy * coordinate.weight_.y;
    }
    if (coordinate.weight_.z > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_z_, ddx_z, ddy_z).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.xy, abs(tangent_normal.z) * normal.z);
        result += blended.xyz * coordinate.weight_.z;
    }
    return normalize(result);
}

/**
* [EN]
* Compute-shader form of the triplanar normal sample, with derivatives taken
* from the neighbouring pixels' mapping coordinates as in
* SampleMaterialMappingGrad.
*/
float3 SampleNormalMappingGrad(Texture2D normal_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate, MaterialMappingCoordinate coordinate_x, MaterialMappingCoordinate coordinate_y)
{
	float3 normal = coordinate.projection_normal_;
	float3 result = float3(0.0, 0.0, 0.0);
	if (coordinate.weight_.x > 0.0)
	{
		float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_x_, coordinate_x.texcoord_x_ - coordinate.texcoord_x_, coordinate_y.texcoord_x_ - coordinate.texcoord_x_).xyz * 2.0 - 1.0;
		float3 blended = float3(tangent_normal.xy + normal.zy, abs(tangent_normal.z) * normal.x);
		result += blended.zyx * coordinate.weight_.x;
	}
	if (coordinate.weight_.y > 0.0)
	{
		float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_y_, coordinate_x.texcoord_y_ - coordinate.texcoord_y_, coordinate_y.texcoord_y_ - coordinate.texcoord_y_).xyz * 2.0 - 1.0;
		float3 blended = float3(tangent_normal.xy + normal.xz, abs(tangent_normal.z) * normal.y);
		result += blended.xzy * coordinate.weight_.y;
	}
	if (coordinate.weight_.z > 0.0)
	{
		float3 tangent_normal = normal_texture.SampleGrad(texture_sampler, coordinate.texcoord_z_, coordinate_x.texcoord_z_ - coordinate.texcoord_z_, coordinate_y.texcoord_z_ - coordinate.texcoord_z_).xyz * 2.0 - 1.0;
		float3 blended = float3(tangent_normal.xy + normal.xy, abs(tangent_normal.z) * normal.z);
		result += blended.xyz * coordinate.weight_.z;
	}
	return normalize(result);
}

float4 SampleMaterialMappingLevel(Texture2D source_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate, float level)
{
    float4 color = float4(0.0, 0.0, 0.0, 0.0);
    if (coordinate.weight_.x > 0.0)
    {
        color += source_texture.SampleLevel(texture_sampler, coordinate.texcoord_x_, level) * coordinate.weight_.x;
    }
    if (coordinate.weight_.y > 0.0)
    {
        color += source_texture.SampleLevel(texture_sampler, coordinate.texcoord_y_, level) * coordinate.weight_.y;
    }
    if (coordinate.weight_.z > 0.0)
    {
        color += source_texture.SampleLevel(texture_sampler, coordinate.texcoord_z_, level) * coordinate.weight_.z;
    }
    return color;
}

float3 SampleNormalMappingLevel(Texture2D normal_texture, SamplerState texture_sampler, MaterialMappingCoordinate coordinate, float level)
{
    float3 normal = coordinate.projection_normal_;
    float3 result = float3(0.0, 0.0, 0.0);
    if (coordinate.weight_.x > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleLevel(texture_sampler, coordinate.texcoord_x_, level).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.zy, abs(tangent_normal.z) * normal.x);
        result += blended.zyx * coordinate.weight_.x;
    }
    if (coordinate.weight_.y > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleLevel(texture_sampler, coordinate.texcoord_y_, level).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.xz, abs(tangent_normal.z) * normal.y);
        result += blended.xzy * coordinate.weight_.y;
    }
    if (coordinate.weight_.z > 0.0)
    {
        float3 tangent_normal = normal_texture.SampleLevel(texture_sampler, coordinate.texcoord_z_, level).xyz * 2.0 - 1.0;
        float3 blended = float3(tangent_normal.xy + normal.xy, abs(tangent_normal.z) * normal.z);
        result += blended.xyz * coordinate.weight_.z;
    }
    return normalize(result);
}

#endif // __MATERIAL_MAPPING_HLSL__

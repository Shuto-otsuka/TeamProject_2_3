#ifndef __RAYTRACING_HIT_HLSLI__
#define __RAYTRACING_HIT_HLSLI__

#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Light/LightCluster.h>
#include <GraphicsEngine/Model/Model.hlsli>
#include <GraphicsEngine/Model/Material/MaterialMapping.hlsli>
#include <GraphicsEngine/Raytracing/TopLevel.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Shader/Vertex.hlsli>

struct RaytracingHitTables
{
	uint top_level_;
	uint hits_;
};

struct RaytracingHitLights
{
	uint list_;
	uint constants_;
	uint cluster_grid_;
};

struct RaytracingHitSurface
{
	float3 normal_;
	float3 albedo_;
	float ior_;
	float transmission_factor_;
	float thickness_;
	float volume_attenuation_distance_;
	float3 volume_attenuation_color_;
};

MaterialMappingCoordinate RaytracingHitMapping(ModelMaterialStructuredBuffer material, CompressedVertex vertex0, CompressedVertex vertex1, CompressedVertex vertex2, float3 weights, float2 texcoord, float3x4 object_to_world, float3x4 world_to_object, float3 world_position)
{
	MaterialMappingCoordinate mapping;
	mapping.texcoord_x_ = texcoord;
	mapping.texcoord_y_ = texcoord;
	mapping.texcoord_z_ = texcoord;
	mapping.weight_ = float3(1.0, 0.0, 0.0);
	mapping.projection_normal_ = float3(0.0, 0.0, 1.0);
	if (material.shading_.material_mapping_ != MATERIAL_MAPPING_STRETCH)
	{
		float3 object_normal = normalize(DecodeCompressedVertexNormal(vertex0) * weights.x + DecodeCompressedVertexNormal(vertex1) * weights.y + DecodeCompressedVertexNormal(vertex2) * weights.z);
		float4 object_tangent = DecodeCompressedVertexTangent(vertex0) * weights.x + DecodeCompressedVertexTangent(vertex1) * weights.y + DecodeCompressedVertexTangent(vertex2) * weights.z;
		float3 object_bitangent = cross(object_normal, object_tangent.xyz) * DecodeCompressedVertexTangent(vertex0).w;
		float3 object_position = mul(world_to_object, float4(world_position, 1.0));
		float3 object_scale = float3(length(float3(object_to_world[0][0], object_to_world[1][0], object_to_world[2][0])), length(float3(object_to_world[0][1], object_to_world[1][1], object_to_world[2][1])), length(float3(object_to_world[0][2], object_to_world[1][2], object_to_world[2][2])));
		float3 world_normal = normalize(mul((float3x3)object_to_world, object_normal));
		mapping = ResolveMaterialMapping(material.shading_.material_mapping_, material.shading_.tiling_, material.shading_.triplanar_sharpness_, texcoord, object_position, object_normal, object_tangent.xyz, object_bitangent, object_scale, world_position, world_normal);
	}
	return mapping;
}

bool RaytracingHitIsPassthrough(RaytracingHitTables tables, uint instance_id, uint primitive_index, float2 barycentrics, float3x4 object_to_world, float3x4 world_to_object, float3 world_position)
{
	StructuredBuffer<TopLevelHitStructuredBuffer> hits = ResourceDescriptorHeap[tables.hits_];
	TopLevelHitStructuredBuffer geometry = hits[instance_id];
	if (geometry.material_buffer_index_ == MODEL_INVALID || geometry.material_offset_ == MODEL_INVALID)
	{
		return false;
	}

	StructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.material_buffer_index_)];
	uint surface_index = 0;
	if (geometry.surface_buffer_index_ != MODEL_INVALID)
	{
		StructuredBuffer<uint> surfaces = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.surface_buffer_index_)];
		surface_index = surfaces[primitive_index];
	}
	ModelMaterialStructuredBuffer material = materials[geometry.material_offset_ + surface_index];
	if (material.shading_.blend_ != 0)
	{
		return true;
	}
	if (material.texture_.alpha_cutoff_ <= 0.0)
	{
		return false;
	}

	float alpha = material.texture_.base_color_.a;
	if (material.texture_.base_color_texture_index_ != MODEL_INVALID)
	{
		StructuredBuffer<uint> triangle_indices = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.index_buffer_index_)];
		StructuredBuffer<CompressedVertex> vertices = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.vertex_buffer_index_)];
		CompressedVertex vertex0 = vertices[triangle_indices[primitive_index * 3 + 0]];
		CompressedVertex vertex1 = vertices[triangle_indices[primitive_index * 3 + 1]];
		CompressedVertex vertex2 = vertices[triangle_indices[primitive_index * 3 + 2]];
		float3 weights = float3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
		float2 texcoord =
			DecodeCompressedVertexTexcoord(vertex0, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.x +
			DecodeCompressedVertexTexcoord(vertex1, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.y +
			DecodeCompressedVertexTexcoord(vertex2, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.z;
		MaterialMappingCoordinate mapping = RaytracingHitMapping(material, vertex0, vertex1, vertex2, weights, texcoord, object_to_world, world_to_object, world_position);
		Texture2D<float4> base_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(material.texture_.base_color_texture_index_)];
		alpha *= SampleMaterialMappingLevel(base_color_texture, sampler_linear_wrap, mapping, 0).a;
	}
	return alpha < material.texture_.alpha_cutoff_;
}

RaytracingHitSurface RaytracingHitResolve(RaytracingHitTables tables, uint instance_id, uint primitive_index, float2 barycentrics, float3x4 object_to_world, float3x4 world_to_object, float3 world_position)
{
	StructuredBuffer<TopLevelHitStructuredBuffer> hits = ResourceDescriptorHeap[tables.hits_];
	TopLevelHitStructuredBuffer geometry = hits[instance_id];

	StructuredBuffer<uint> triangle_indices = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.index_buffer_index_)];
	StructuredBuffer<CompressedVertex> vertices = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.vertex_buffer_index_)];
	CompressedVertex vertex0 = vertices[triangle_indices[primitive_index * 3 + 0]];
	CompressedVertex vertex1 = vertices[triangle_indices[primitive_index * 3 + 1]];
	CompressedVertex vertex2 = vertices[triangle_indices[primitive_index * 3 + 2]];
	float3 weights = float3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
	float3 object_normal = DecodeCompressedVertexNormal(vertex0) * weights.x + DecodeCompressedVertexNormal(vertex1) * weights.y + DecodeCompressedVertexNormal(vertex2) * weights.z;
	float2 texcoord =
		DecodeCompressedVertexTexcoord(vertex0, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.x +
		DecodeCompressedVertexTexcoord(vertex1, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.y +
		DecodeCompressedVertexTexcoord(vertex2, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.z;

	RaytracingHitSurface surface;
	surface.normal_ = normalize(mul((float3x3)object_to_world, object_normal));
	surface.albedo_ = float3(1.0, 1.0, 1.0);
	surface.ior_ = 1.5;
	surface.transmission_factor_ = 0.0;
	surface.thickness_ = 0.0;
	surface.volume_attenuation_distance_ = 0.0;
	surface.volume_attenuation_color_ = float3(1.0, 1.0, 1.0);
	if (geometry.material_buffer_index_ == MODEL_INVALID || geometry.material_offset_ == MODEL_INVALID)
	{
		return surface;
	}

	StructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.material_buffer_index_)];
	uint surface_index = 0;
	if (geometry.surface_buffer_index_ != MODEL_INVALID)
	{
		StructuredBuffer<uint> surfaces = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.surface_buffer_index_)];
		surface_index = surfaces[primitive_index];
	}
	ModelMaterialStructuredBuffer material = materials[geometry.material_offset_ + surface_index];
	surface.albedo_ = material.texture_.base_color_.rgb;
	surface.ior_ = material.texture_.ior_;
	surface.transmission_factor_ = material.extension_.transmission_factor_;
	surface.thickness_ = material.extension_.volume_thickness_factor_;
	surface.volume_attenuation_distance_ = material.extension_.volume_attenuation_distance_;
	surface.volume_attenuation_color_ = material.extension_.volume_attenuation_color_;

	if (material.texture_.base_color_texture_index_ != MODEL_INVALID || (surface.thickness_ > 0.0 && material.extension_.thickness_texture_index_ != MODEL_INVALID))
	{
		MaterialMappingCoordinate mapping = RaytracingHitMapping(material, vertex0, vertex1, vertex2, weights, texcoord, object_to_world, world_to_object, world_position);
		if (material.texture_.base_color_texture_index_ != MODEL_INVALID)
		{
			Texture2D<float4> base_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(material.texture_.base_color_texture_index_)];
			surface.albedo_ *= SampleMaterialMappingLevel(base_color_texture, sampler_linear_wrap, mapping, 0).rgb;
		}
		if (surface.thickness_ > 0.0 && material.extension_.thickness_texture_index_ != MODEL_INVALID)
		{
			Texture2D<float4> thickness_texture = ResourceDescriptorHeap[NonUniformResourceIndex(material.extension_.thickness_texture_index_)];
			surface.thickness_ *= SampleMaterialMappingLevel(thickness_texture, sampler_linear_wrap, mapping, 0).g;
		}
	}
	return surface;
}

bool RaytracingHitIsOccluded(RaytracingHitTables tables, RayDesc ray)
{
	RaytracingAccelerationStructure top_level = ResourceDescriptorHeap[tables.top_level_];
	RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
	query.TraceRayInline(top_level, RAY_FLAG_NONE, 0xFF, ray);
	while (query.Proceed())
	{
		if (query.CandidateType() != CANDIDATE_NON_OPAQUE_TRIANGLE)
		{
			continue;
		}
		if (!RaytracingHitIsPassthrough(tables, query.CandidateInstanceID(), query.CandidatePrimitiveIndex(), query.CandidateTriangleBarycentrics(), query.CandidateObjectToWorld3x4(), query.CandidateWorldToObject3x4(), query.WorldRayOrigin() + query.WorldRayDirection() * query.CandidateTriangleRayT()))
		{
			query.CommitNonOpaqueTriangleHit();
		}
	}
	return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT;
}

float3 RaytracingHitTransmittance(RaytracingHitTables tables, RayDesc ray)
{
	RaytracingAccelerationStructure top_level = ResourceDescriptorHeap[tables.top_level_];
	StructuredBuffer<TopLevelHitStructuredBuffer> hits = ResourceDescriptorHeap[tables.hits_];
	RayQuery<RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH | RAY_FLAG_SKIP_PROCEDURAL_PRIMITIVES> query;
	query.TraceRayInline(top_level, RAY_FLAG_NONE, 0xFF, ray);
	float3 transmittance = float3(1.0, 1.0, 1.0);
	while (query.Proceed())
	{
		if (query.CandidateType() != CANDIDATE_NON_OPAQUE_TRIANGLE)
		{
			continue;
		}

		uint instance_id = query.CandidateInstanceID();
		uint primitive_index = query.CandidatePrimitiveIndex();
		float3x4 object_to_world = query.CandidateObjectToWorld3x4();
		float3x4 world_to_object = query.CandidateWorldToObject3x4();
		float3 world_position = query.WorldRayOrigin() + query.WorldRayDirection() * query.CandidateTriangleRayT();
		TopLevelHitStructuredBuffer geometry = hits[instance_id];
		if (geometry.material_buffer_index_ == MODEL_INVALID || geometry.material_offset_ == MODEL_INVALID)
		{
			query.CommitNonOpaqueTriangleHit();
			continue;
		}

		StructuredBuffer<ModelMaterialStructuredBuffer> materials = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.material_buffer_index_)];
		uint surface_index = 0;
		if (geometry.surface_buffer_index_ != MODEL_INVALID)
		{
			StructuredBuffer<uint> surfaces = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.surface_buffer_index_)];
			surface_index = surfaces[primitive_index];
		}
		ModelMaterialStructuredBuffer material = materials[geometry.material_offset_ + surface_index];
		if (material.shading_.blend_ == 0)
		{
			if (!RaytracingHitIsPassthrough(tables, instance_id, primitive_index, query.CandidateTriangleBarycentrics(), object_to_world, world_to_object, world_position))
			{
				query.CommitNonOpaqueTriangleHit();
			}
			continue;
		}

		float4 base_color = material.texture_.base_color_;
		if (material.texture_.base_color_texture_index_ != MODEL_INVALID)
		{
			StructuredBuffer<uint> triangle_indices = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.index_buffer_index_)];
			StructuredBuffer<CompressedVertex> vertices = ResourceDescriptorHeap[NonUniformResourceIndex(geometry.vertex_buffer_index_)];
			CompressedVertex vertex0 = vertices[triangle_indices[primitive_index * 3 + 0]];
			CompressedVertex vertex1 = vertices[triangle_indices[primitive_index * 3 + 1]];
			CompressedVertex vertex2 = vertices[triangle_indices[primitive_index * 3 + 2]];
			float2 barycentrics = query.CandidateTriangleBarycentrics();
			float3 weights = float3(1.0 - barycentrics.x - barycentrics.y, barycentrics.x, barycentrics.y);
			float2 texcoord =
				DecodeCompressedVertexTexcoord(vertex0, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.x +
				DecodeCompressedVertexTexcoord(vertex1, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.y +
				DecodeCompressedVertexTexcoord(vertex2, geometry.texcoord_min_, geometry.texcoord_extent_) * weights.z;
			MaterialMappingCoordinate mapping = RaytracingHitMapping(material, vertex0, vertex1, vertex2, weights, texcoord, object_to_world, world_to_object, world_position);
			Texture2D<float4> base_color_texture = ResourceDescriptorHeap[NonUniformResourceIndex(material.texture_.base_color_texture_index_)];
			base_color *= SampleMaterialMappingLevel(base_color_texture, sampler_linear_wrap, mapping, 0);
		}
		transmittance *= base_color.rgb * (1.0 - saturate(base_color.a));
	}
	return query.CommittedStatus() == COMMITTED_TRIANGLE_HIT ? float3(0.0, 0.0, 0.0) : transmittance;
}

float3 RaytracingHitLighting(RaytracingHitTables tables, RaytracingHitLights lights, float3 position, float3 normal, float normal_bias, float ray_t_max)
{
	const float lambert_normalization = 1.0 / 3.14159265358979;
	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<LightConstantBuffer> light_constants = ResourceDescriptorHeap[lights.constants_];

	float3 lighting = float3(0.0, 0.0, 0.0);
	if (light_constants.sun_intensity_ > 0.0)
	{
		float3 sun_direction = normalize(-light_constants.sun_direction_);
		float normal_dot_light = saturate(dot(normal, sun_direction));
		if (normal_dot_light > 0.0)
		{
			RayDesc shadow_ray;
			shadow_ray.Origin = position + normal * normal_bias;
			shadow_ray.Direction = sun_direction;
			shadow_ray.TMin = 0.001;
			shadow_ray.TMax = ray_t_max;
			if (!RaytracingHitIsOccluded(tables, shadow_ray))
			{
				lighting += light_constants.sun_color_ * light_constants.sun_intensity_ * normal_dot_light * lambert_normalization;
			}
		}
	}

	float4 clip = mul(float4(position, 1.0), scene.non_jitter_view_projection_);
	if (clip.w <= 0.0)
	{
		return lighting;
	}
	float2 uv = float2(clip.x / clip.w * 0.5 + 0.5, 0.5 - clip.y / clip.w * 0.5);
	uint2 screen_size = uint2(scene.screen_size_);
	uint2 pixel = min(uint2(saturate(uv) * float2(screen_size)), screen_size - 1);
	float view_depth = mul(float4(position, 1.0), scene.view_).z;
	uint2 tile_count = (screen_size + LIGHT_CLUSTER_TILE_SIZE - 1) / LIGHT_CLUSTER_TILE_SIZE;
	uint2 tile = min(pixel / LIGHT_CLUSTER_TILE_SIZE, tile_count - 1);
	uint slice = 0;
	if (view_depth >= scene.far_plane_)
	{
		slice = LIGHT_CLUSTER_DEPTH_SLICES - 1;
	}
	else if (view_depth > scene.near_plane_)
	{
		float log_near = log(scene.near_plane_);
		slice = min(uint((log(view_depth) - log_near) / (log(scene.far_plane_) - log_near) * LIGHT_CLUSTER_DEPTH_SLICES), LIGHT_CLUSTER_DEPTH_SLICES - 1);
	}
	uint cluster = (slice * tile_count.y + tile.y) * tile_count.x + tile.x;

	StructuredBuffer<uint> grid = ResourceDescriptorHeap[lights.cluster_grid_];
	StructuredBuffer<LightStructuredBuffer> light_list = ResourceDescriptorHeap[lights.list_];
	uint base = cluster * (1 + LIGHT_CLUSTER_MAX_LIGHTS);
	uint light_count = grid[base];
	float3 punctual = float3(0.0, 0.0, 0.0);
	for (uint light_slot = 0; light_slot < light_count; light_slot++)
	{
		LightStructuredBuffer light = light_list[grid[base + 1 + light_slot]];

		float3 light_position = light.position_;
		if (light.type_ == LIGHT_TYPE_RECT)
		{
			float3 offset = position - light.position_;
			if (dot(offset, light.direction_) <= 0.0)
			{
				continue;
			}
			light_position += light.right_ * clamp(dot(offset, light.right_), -light.half_width_, light.half_width_);
			light_position += light.up_ * clamp(dot(offset, light.up_), -light.half_height_, light.half_height_);
		}

		float3 to_light = light_position - position;
		float distance_to_light = length(to_light);
		float3 light_direction = to_light / max(distance_to_light, 0.0001);
		float normal_dot_light = saturate(dot(normal, light_direction));
		if (normal_dot_light <= 0.0)
		{
			continue;
		}

		float attenuation_ratio = saturate(distance_to_light / light.range_);
		float attenuation_ratio2 = attenuation_ratio * attenuation_ratio;
		float attenuation = saturate(1.0 - attenuation_ratio2 * attenuation_ratio2);
		attenuation = attenuation * attenuation / max(distance_to_light * distance_to_light, 0.0001);
		if (light.type_ == LIGHT_TYPE_SPOT)
		{
			float cos_angle = dot(-light_direction, light.direction_);
			attenuation *= saturate((cos_angle - light.cos_half_angle_) / max(light.softness_ * (1.0 - light.cos_half_angle_), 0.0001));
		}
		punctual += light.color_ * light.intensity_ * attenuation * normal_dot_light;
	}
	return lighting + punctual * lambert_normalization;
}

#endif // __RAYTRACING_HIT_HLSLI__

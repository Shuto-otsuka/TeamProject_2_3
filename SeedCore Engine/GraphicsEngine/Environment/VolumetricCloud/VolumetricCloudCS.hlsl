#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloud.hlsli>
#include <GraphicsEngine/Environment/VolumetricCloud/VolumetricCloudNoise.hlsli>
#include <GraphicsEngine/Light/Light.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Shader/Sampler.hlsli>
#include <GraphicsEngine/Shader/Noise.hlsli>

struct VolumetricCloudShapeBindings
{
	uint volumetric_cloud_shape_;
};
ConstantBuffer<VolumetricCloudShapeBindings> shape_bindings : register(b0, space1);

struct VolumetricCloudDetailBindings
{
	uint volumetric_cloud_detail_;
};
ConstantBuffer<VolumetricCloudDetailBindings> detail_bindings : register(b0, space1);

struct VolumetricCloudTraceBindings
{
	uint volumetric_cloud_constants_;
	uint volumetric_cloud_shape_;
	uint volumetric_cloud_detail_;
	uint geometry_depth_;
	uint light_constants_;
	uint volumetric_cloud_raw_;
};
ConstantBuffer<VolumetricCloudTraceBindings> trace_bindings : register(b0, space1);

struct VolumetricCloudResolveBindings
{
	uint volumetric_cloud_raw_;
	uint geometry_depth_;
	uint volumetric_cloud_accumulated_history_;
	uint volumetric_cloud_accumulated_;
	uint volumetric_cloud_radiance_;
};
ConstantBuffer<VolumetricCloudResolveBindings> resolve_bindings : register(b0, space1);

[numthreads(4, 4, 4)]
void Shape(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture3D<float> shape_output = ResourceDescriptorHeap[shape_bindings.volumetric_cloud_shape_];

	uint width, height, depth;
	shape_output.GetDimensions(width, height, depth);

	if (dtid.x >= width || dtid.y >= height || dtid.z >= depth)
	{
		return;
	}

	float3 uvw = (float3(dtid) + 0.5) / float3(width, height, depth);

	shape_output[dtid] = TileablePerlinWorley(uvw, 4.0, 4);
}

[numthreads(4, 4, 4)]
void Detail(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture3D<float> detail_output = ResourceDescriptorHeap[detail_bindings.volumetric_cloud_detail_];

	uint width, height, depth;
	detail_output.GetDimensions(width, height, depth);

	if (dtid.x >= width || dtid.y >= height || dtid.z >= depth)
	{
		return;
	}

	float3 uvw = (float3(dtid) + 0.5) / float3(width, height, depth);

	detail_output[dtid] = TileableWorleyFbm(uvw, 4.0, 3);
}

[numthreads(8, 8, 1)]
void Trace(uint3 dtid : SV_DispatchThreadID)
{
	SceneConstantBuffer scene = GetSceneConstantBuffer();

	RWTexture2D<float4> output = ResourceDescriptorHeap[trace_bindings.volumetric_cloud_raw_];
	uint cloud_width;
	uint cloud_height;
	output.GetDimensions(cloud_width, cloud_height);
	if (dtid.x >= cloud_width || dtid.y >= cloud_height)
	{
		return;
	}

	uint2 pixel = dtid.xy;

	Texture2D<float> depth_texture = ResourceDescriptorHeap[trace_bindings.geometry_depth_];
	if (depth_texture.Load(int3(pixel, 0)) != 0.0)
	{
		output[pixel] = float4(0, 0, 0, 0);
		return;
	}

	ConstantBuffer<VolumetricCloudConstantBuffer> tuning = ResourceDescriptorHeap[trace_bindings.volumetric_cloud_constants_];

	Texture3D<float> shape_noise = ResourceDescriptorHeap[trace_bindings.volumetric_cloud_shape_];
	Texture3D<float> detail_noise = ResourceDescriptorHeap[trace_bindings.volumetric_cloud_detail_];

	float2 uv = (float2(pixel) + 0.5) / float2(cloud_width, cloud_height);
	float2 ndc = float2(uv.x * 2 - 1, 1 - uv.y * 2);
	float3 view_direction = float3((ndc.x - scene.projection_._31) / scene.projection_._11, (ndc.y - scene.projection_._32) / scene.projection_._22, 1.0);
	float3 ray_direction = normalize(mul(view_direction, (float3x3)scene.inverse_view_));
	float3 ray_origin = scene.camera_position_.xyz;

	float t_enter;
	float t_exit;
	if (!CloudLayerInterval(ray_origin, ray_direction, tuning, t_enter, t_exit))
	{
		output[pixel] = float4(0, 0, 0, 0);
		return;
	}

	ConstantBuffer<LightConstantBuffer> light = ResourceDescriptorHeap[trace_bindings.light_constants_];
	float3 light_direction = normalize(-light.sun_direction_);
	float3 sun_radiance = light.sun_color_ * light.sun_intensity_;

	float3 cloud_albedo = lerp(tuning.cloud_albedo_, tuning.cloud_albedo_ * 0.35, saturate(tuning.rain_));

	float curvature = CloudCurvature(ray_direction, tuning.planet_radius_);
	float layer_thickness = max(tuning.cloud_top_ - tuning.cloud_bottom_, 1.0);
	float cos_theta = dot(ray_direction, light_direction);

	CloudMultiScatter multi_scatter = CloudMultiScatterSetup(cos_theta, tuning);

	float jitter = InterleavedGradientNoise(float2(pixel) + 5.588238 * float(tuning.frame_index_ & 15u));

	const uint max_iterations = 256;

	float fine_step_base = layer_thickness / float(max(tuning.step_count_, 1u));
	const float coarse_step_scale = 8.0;

	float minimum_step = (t_exit - t_enter) / (float(max_iterations) * 1.5);

	float3 scattering = float3(0, 0, 0);
	float transmittance = 1.0;

	float weighted_distance = 0.0;
	float weight_total = 0.0;

	float t = t_enter;
	bool coarse = true;
	uint empty_run = 0;

	for (uint iteration = 0; iteration < max_iterations; iteration++)
	{
		if (t >= t_exit)
		{
			break;
		}

		float lod = 1.0 + t / max(tuning.lod_distance_, 1.0);
		float fine_step = max(fine_step_base * lod, minimum_step);
		float step_length = coarse ? fine_step * coarse_step_scale : fine_step;

		float sample_t = t + step_length * (coarse ? 0.5 : jitter);
		if (sample_t >= t_exit)
		{
			break;
		}

		float altitude = CloudAltitudeAt(ray_origin.y, ray_direction.y, curvature, sample_t);
		float3 sample_position = ray_origin + ray_direction * sample_t;

		float detail_strength = coarse ? 0.0 : saturate(1.0 - (t - tuning.lod_distance_) / max(tuning.lod_distance_ * 3.0, 1.0));

		float density = CloudDensity(sample_position, altitude, tuning, scene.total_time_, shape_noise, detail_noise, sampler_linear_wrap, detail_strength);

		if (density <= 0.0)
		{
			empty_run++;
			if (!coarse && empty_run > 8)
			{
				coarse = true;
				empty_run = 0;
			}
			t += step_length;
			continue;
		}

		empty_run = 0;

		if (coarse)
		{
			coarse = false;
			continue;
		}

		uint light_steps = transmittance > 0.3 ? tuning.light_step_count_ : max(tuning.light_step_count_ / 2u, 2u);

		float light_optical_depth = CloudLightOpticalDepth(sample_position, altitude, light_direction, tuning, scene.total_time_, shape_noise, detail_noise, sampler_linear_wrap, light_steps, jitter);

		float sample_extinction = density * step_length;

		float powder = CloudPowder(light_optical_depth, cos_theta, tuning.powder_strength_);
		float sun_light = CloudSunLight(light_optical_depth, powder, multi_scatter);

		float height01 = saturate((altitude - tuning.cloud_bottom_) / layer_thickness);
		float ambient_occlusion = lerp(0.3, 1.0, height01);
		float3 sky_ambient = lerp(tuning.sky_horizon_color_, tuning.sky_zenith_color_, height01) * tuning.sky_brightness_ * tuning.ambient_strength_ * ambient_occlusion;

		float3 in_scattering = cloud_albedo * (sun_radiance * sun_light + sky_ambient);

		float sample_transmittance = exp(-sample_extinction);
		float segment_weight = (1.0 - sample_transmittance) * transmittance;

		scattering += in_scattering * segment_weight;

		weighted_distance += sample_t * segment_weight;
		weight_total += segment_weight;

		transmittance *= sample_transmittance;

		if (transmittance < 0.005)
		{
			break;
		}

		t += step_length;
	}

	float alpha = 1.0 - transmittance;

	if (alpha > 0.0)
	{
		float mean_distance = weight_total > 0.0 ? weighted_distance / weight_total : t_enter;

		float aerial = exp(-mean_distance * tuning.aerial_density_);
		float3 horizon = tuning.sky_horizon_color_ * tuning.sky_brightness_;
		scattering = lerp(horizon * alpha, scattering, aerial);

		float fade_start = tuning.max_march_distance_ * 0.6;
		float distance_fade = 1.0 - smoothstep(fade_start, tuning.max_march_distance_, mean_distance);

		alpha *= distance_fade;
		scattering *= distance_fade;
	}

	output[pixel] = float4(scattering, alpha);
}

[numthreads(8, 8, 1)]
void Resolve(uint3 dtid : SV_DispatchThreadID)
{
	RWTexture2D<float4> radiance = ResourceDescriptorHeap[resolve_bindings.volumetric_cloud_radiance_];
	uint width;
	uint height;
	radiance.GetDimensions(width, height);
	if (dtid.x >= width || dtid.y >= height)
	{
		return;
	}

	uint2 pixel = dtid.xy;
	RWTexture2D<float4> accumulated = ResourceDescriptorHeap[resolve_bindings.volumetric_cloud_accumulated_];
	Texture2D<float> depth_texture = ResourceDescriptorHeap[resolve_bindings.geometry_depth_];
	if (depth_texture.Load(int3(pixel, 0)) != 0.0)
	{
		accumulated[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		radiance[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	Texture2D<float4> raw = ResourceDescriptorHeap[resolve_bindings.volumetric_cloud_raw_];
	float4 current = raw.Load(int3(pixel, 0));
	float4 result = current;

	if (dispatch_bindings.constants_ != 0)
	{
		SceneConstantBuffer scene = GetSceneConstantBuffer();
		float2 uv = (float2(pixel) + 0.5) / float2(width, height);
		float2 ndc = float2(uv.x * 2.0 - 1.0, 1.0 - uv.y * 2.0);
		float3 view_direction = float3((ndc.x - scene.projection_._31) / scene.projection_._11, (ndc.y - scene.projection_._32) / scene.projection_._22, 1.0);
		float3 direction = normalize(mul(view_direction, (float3x3)scene.inverse_view_));
		float4 previous_clip = mul(float4(direction, 0.0), scene.previous_view_projection_);
		if (previous_clip.w > 0.0)
		{
			float2 previous_ndc = previous_clip.xy / previous_clip.w;
			float2 previous_uv = float2(previous_ndc.x * 0.5 + 0.5, 0.5 - previous_ndc.y * 0.5);
			if (all(previous_uv >= 0.0) && all(previous_uv <= 1.0))
			{
				float4 minimum = current;
				float4 maximum = current;
				int2 pixel_max = int2(width, height) - 1;
				for (int offset_y = -1; offset_y <= 1; offset_y++)
				{
					for (int offset_x = -1; offset_x <= 1; offset_x++)
					{
						float4 neighbor = raw.Load(int3(clamp(int2(pixel) + int2(offset_x, offset_y), int2(0, 0), pixel_max), 0));
						minimum = min(minimum, neighbor);
						maximum = max(maximum, neighbor);
					}
				}

				Texture2D<float4> history = ResourceDescriptorHeap[resolve_bindings.volumetric_cloud_accumulated_history_];
				float4 previous = clamp(history.SampleLevel(sampler_linear_clamp, previous_uv, 0), minimum, maximum);
				result = lerp(previous, current, 0.1);
			}
		}
	}

	accumulated[pixel] = result;
	radiance[pixel] = result;
}

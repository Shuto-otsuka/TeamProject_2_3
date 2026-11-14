#include <GraphicsEngine/Illumination/Reflection/Raytracing/RaytracingReflection.hlsli>
#include <GraphicsEngine/Shader/Denoiser.hlsli>
#include <GraphicsEngine/Shader/Normal.hlsli>
#include <GraphicsEngine/Shader/Scene.hlsli>
#include <GraphicsEngine/Debug/DebugView.hlsli>

static const float REFLECTION_TEMPORAL_ALPHA = 0.05;
static const float REFLECTION_MOMENTS_ALPHA = 0.2;
static const float REFLECTION_MAX_HISTORY_LENGTH = 32.0;
static const float REFLECTION_SPATIAL_VARIANCE_FRAMES = 4.0;
static const float REFLECTION_PHI_LUMINANCE = 24.0;
static const float REFLECTION_PHI_NORMAL = 128.0;
static const float REFLECTION_ROUGHNESS_PHI_NORMAL_SCALE = 0.08;
static const float REFLECTION_REPROJECT_DEPTH_TOLERANCE = 10.0;
static const float REFLECTION_REPROJECT_NORMAL_TOLERANCE = 16.0;
static const float REFLECTION_DEPTH_SLOPE_FLOOR = 1e-3;
static const float REFLECTION_VIRTUAL_MOTION_ROUGHNESS_CUTOFF = 0.5;

struct RaytracingReflectionTemporalBindings
{
	uint raytracing_reflection_raw_;
	uint raytracing_reflection_confidence_;
	uint geometry_depth_;
	uint material_normal_roughness_;
	uint material_velocity_;
	uint raytracing_reflection_constants_;
	uint raytracing_reflection_accumulated_history_;
	uint raytracing_reflection_moments_history_;
	uint raytracing_reflection_moments_;
	uint raytracing_reflection_age_history_;
	uint raytracing_reflection_age_;
	uint raytracing_reflection_depth_normal_history_;
	uint raytracing_reflection_depth_normal_;
	uint raytracing_reflection_ping_;
	uint raytracing_reflection_validity_;
};
ConstantBuffer<RaytracingReflectionTemporalBindings> temporal_bindings : register(b0, space1);

struct RaytracingReflectionMomentsBindings
{
	uint raytracing_reflection_ping_;
	uint raytracing_reflection_moments_;
	uint raytracing_reflection_age_;
	uint raytracing_reflection_depth_normal_;
	uint material_normal_roughness_;
	uint raytracing_reflection_pong_;
};
ConstantBuffer<RaytracingReflectionMomentsBindings> moments_bindings : register(b0, space1);

struct RaytracingReflectionATrousBindings
{
	uint raytracing_reflection_source_;
	uint raytracing_reflection_depth_normal_;
	uint material_normal_roughness_;
	uint raytracing_reflection_constants_;
	uint raytracing_reflection_destination_;
};
ConstantBuffer<RaytracingReflectionATrousBindings> atrous_bindings : register(b0, space1);

[numthreads(8, 8, 1)]
void Temporal(uint3 id : SV_DispatchThreadID)
{
	Texture2D<float> depth_texture = ResourceDescriptorHeap[temporal_bindings.geometry_depth_];
	uint width;
	uint height;
	depth_texture.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	int2 pixel = int2(id.xy);
	int2 screen_max = int2(width, height) - 1;
	RWTexture2D<float4> ping = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_ping_];
	RWTexture2D<float2> moments_output = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_moments_];
	RWTexture2D<float> age_output = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_age_];
	RWTexture2D<float4> depth_normal_output = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_depth_normal_];

	float depth = depth_texture.Load(int3(pixel, 0));
	if (depth == 0.0)
	{
		ping[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		moments_output[pixel] = float2(0.0, 0.0);
		age_output[pixel] = 0.0;
		depth_normal_output[pixel] = float4(0.0, 0.0, 0.0, 0.0);
		return;
	}

	SceneConstantBuffer scene = GetSceneConstantBuffer();
	ConstantBuffer<RaytracingReflectionConstantBuffer> constants = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_constants_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[temporal_bindings.material_normal_roughness_];
	float2 screen_size = float2(width, height);
	float2 uv = (float2(pixel) + 0.5) / screen_size;
	float view_z = abs(DenoiserViewPosition(scene.inverse_projection_, uv, depth).z);
	float3 world_position = DenoiserWorldPosition(scene.inverse_view_projection_, uv, depth);
	uint packed = surface_normal_roughness.Load(int3(pixel, 0));
	float3 normal = OctNormalDecode(float2(packed & 0xFFF, (packed >> 12) & 0xFFF) / 4095.0);
	float roughness = (packed >> 24) / 255.0;

	const int2 neighbor_offsets[4] = { int2(-1, 0), int2(1, 0), int2(0, -1), int2(0, 1) };
	float neighbor_view_z[4];
	float3 neighbor_normal[4];
	[unroll]
	for (int neighbor_index = 0; neighbor_index < 4; ++neighbor_index)
	{
		int2 neighbor_pixel = clamp(pixel + neighbor_offsets[neighbor_index], int2(0, 0), screen_max);
		float neighbor_depth = depth_texture.Load(int3(neighbor_pixel, 0));
		neighbor_view_z[neighbor_index] = abs(DenoiserViewPosition(scene.inverse_projection_, (float2(neighbor_pixel) + 0.5) / screen_size, neighbor_depth).z);
		uint neighbor_packed = surface_normal_roughness.Load(int3(neighbor_pixel, 0));
		neighbor_normal[neighbor_index] = OctNormalDecode(float2(neighbor_packed & 0xFFF, (neighbor_packed >> 12) & 0xFFF) / 4095.0);
	}
	float view_z_derivative = SvgfViewDepthDerivative(neighbor_view_z[0], neighbor_view_z[1], neighbor_view_z[2], neighbor_view_z[3]);
	float normal_derivative = max(distance(neighbor_normal[1], neighbor_normal[0]), distance(neighbor_normal[3], neighbor_normal[2])) * 0.5;
	depth_normal_output[pixel] = SvgfPackDepthNormal(view_z, view_z_derivative, normal);

	Texture2D<float4> raw = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_raw_];
	float4 raw_value = raw.Load(int3(pixel, 0));

	float3 previous_radiance = float3(0.0, 0.0, 0.0);
	float2 previous_moments = float2(0.0, 0.0);
	float previous_age = 0.0;
	bool valid = false;
	float reprojection = 0.0;
	if (constants.history_valid_ != 0)
	{
		Texture2D<float2> velocity = ResourceDescriptorHeap[temporal_bindings.material_velocity_];
		float2 surface_uv = DenoiserPreviousUv(uv, velocity.Load(int3(pixel, 0)));
		float3 view = normalize(scene.camera_position_.xyz - world_position);
		float dominant_a = 0.298475 * log(39.4115 - 39.0029 * saturate(roughness));
		float dominant_factor = saturate(pow(saturate(1.0 - abs(dot(normal, view))), 10.8649) * (1.0 - dominant_a) + dominant_a);
		float3 dominant_direction = normalize(lerp(normal, reflect(-view, normal), dominant_factor));
		float4 virtual_clip = mul(float4(world_position + dominant_direction * raw_value.a, 1.0), scene.previous_view_projection_);
		float2 virtual_ndc = virtual_clip.xy / max(virtual_clip.w, 1e-6);
		float2 virtual_uv = float2(virtual_ndc.x * 0.5 + 0.5, 0.5 - virtual_ndc.y * 0.5);
		float2 previous_uv = lerp(surface_uv, virtual_uv, saturate(1.0 - roughness / REFLECTION_VIRTUAL_MOTION_ROUGHNESS_CUTOFF));
		reprojection = length((previous_uv - uv) * screen_size);

		Texture2D<float4> accumulated_history = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_accumulated_history_];
		Texture2D<float2> moments_history = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_moments_history_];
		Texture2D<float> age_history = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_age_history_];
		Texture2D<float4> depth_normal_history = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_depth_normal_history_];

		float2 previous_position = previous_uv * screen_size - 0.5;
		int2 previous_base = int2(floor(previous_position));
		float2 previous_fraction = previous_position - float2(previous_base);
		const int2 bilinear_offsets[4] = { int2(0, 0), int2(1, 0), int2(0, 1), int2(1, 1) };
		float bilinear_weights[4] =
		{
			(1.0 - previous_fraction.x) * (1.0 - previous_fraction.y),
			previous_fraction.x * (1.0 - previous_fraction.y),
			(1.0 - previous_fraction.x) * previous_fraction.y,
			previous_fraction.x * previous_fraction.y
		};
		float weight_sum = 0.0;
		[unroll]
		for (int tap = 0; tap < 4; ++tap)
		{
			int2 tap_pixel = previous_base + bilinear_offsets[tap];
			if (any(tap_pixel < int2(0, 0)) || any(tap_pixel > screen_max))
			{
				continue;
			}
			float4 tap_depth_normal = depth_normal_history.Load(int3(tap_pixel, 0));
			if (tap_depth_normal.x <= 0.0 || abs(tap_depth_normal.x - view_z) / (view_z_derivative + view_z * REFLECTION_DEPTH_SLOPE_FLOOR) > REFLECTION_REPROJECT_DEPTH_TOLERANCE || distance(normal, SvgfUnpackNormal(tap_depth_normal)) / (normal_derivative + 0.01) > REFLECTION_REPROJECT_NORMAL_TOLERANCE)
			{
				continue;
			}
			previous_radiance += accumulated_history.Load(int3(tap_pixel, 0)).rgb * bilinear_weights[tap];
			previous_moments += moments_history.Load(int3(tap_pixel, 0)) * bilinear_weights[tap];
			previous_age += age_history.Load(int3(tap_pixel, 0)) * bilinear_weights[tap];
			weight_sum += bilinear_weights[tap];
		}
		if (weight_sum >= 0.01)
		{
			previous_radiance /= weight_sum;
			previous_moments /= weight_sum;
			previous_age /= weight_sum;
			valid = true;
		}
		else
		{
			float fallback_count = 0.0;
			int2 previous_pixel = int2(previous_uv * screen_size);
			[unroll]
			for (int offset_y = -1; offset_y <= 1; ++offset_y)
			{
				[unroll]
				for (int offset_x = -1; offset_x <= 1; ++offset_x)
				{
					int2 tap_pixel = previous_pixel + int2(offset_x, offset_y);
					if (any(tap_pixel < int2(0, 0)) || any(tap_pixel > screen_max))
					{
						continue;
					}
					float4 tap_depth_normal = depth_normal_history.Load(int3(tap_pixel, 0));
					if (tap_depth_normal.x <= 0.0 || abs(tap_depth_normal.x - view_z) / (view_z_derivative + view_z * REFLECTION_DEPTH_SLOPE_FLOOR) > REFLECTION_REPROJECT_DEPTH_TOLERANCE || distance(normal, SvgfUnpackNormal(tap_depth_normal)) / (normal_derivative + 0.01) > REFLECTION_REPROJECT_NORMAL_TOLERANCE)
					{
						continue;
					}
					previous_radiance += accumulated_history.Load(int3(tap_pixel, 0)).rgb;
					previous_moments += moments_history.Load(int3(tap_pixel, 0));
					previous_age += age_history.Load(int3(tap_pixel, 0));
					fallback_count += 1.0;
				}
			}
			if (fallback_count > 0.0)
			{
				previous_radiance /= fallback_count;
				previous_moments /= fallback_count;
				previous_age /= fallback_count;
				valid = true;
			}
		}
	}

	Texture2D<float> confidence_texture = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_confidence_];
	float age = min(REFLECTION_MAX_HISTORY_LENGTH, valid ? previous_age + 1.0 : 1.0);
	float alpha = valid ? max(lerp(REFLECTION_TEMPORAL_ALPHA, 1.0, confidence_texture.Load(int3(pixel, 0))), 1.0 / age) : 1.0;
	float moments_alpha = valid ? max(REFLECTION_MOMENTS_ALPHA, 1.0 / age) : 1.0;
	float raw_luminance = DenoiserLuminance(raw_value.rgb);
	float2 moments = lerp(previous_moments, float2(raw_luminance, raw_luminance * raw_luminance), moments_alpha);

	ping[pixel] = float4(lerp(previous_radiance, raw_value.rgb, alpha), max(0.0, moments.y - moments.x * moments.x));
	moments_output[pixel] = moments;
	age_output[pixel] = age;

	if (scene.view_mode_ >= VIEW_MODE_DENOISER_RAW && scene.view_mode_ <= VIEW_MODE_DENOISER_REPROJECTION)
	{
		RWTexture2D<float2> validity = ResourceDescriptorHeap[temporal_bindings.raytracing_reflection_validity_];
		validity[pixel] = float2(valid ? 1.0 : 0.0, reprojection);
	}
}

[numthreads(8, 8, 1)]
void Moments(uint3 id : SV_DispatchThreadID)
{
	Texture2D<float4> source = ResourceDescriptorHeap[moments_bindings.raytracing_reflection_ping_];
	uint width;
	uint height;
	source.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	int2 pixel = int2(id.xy);
	int2 screen_max = int2(width, height) - 1;
	Texture2D<float2> moments_texture = ResourceDescriptorHeap[moments_bindings.raytracing_reflection_moments_];
	Texture2D<float> age_texture = ResourceDescriptorHeap[moments_bindings.raytracing_reflection_age_];
	Texture2D<float4> depth_normal_texture = ResourceDescriptorHeap[moments_bindings.raytracing_reflection_depth_normal_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[moments_bindings.material_normal_roughness_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[moments_bindings.raytracing_reflection_pong_];

	float4 center = source.Load(int3(pixel, 0));
	float4 center_depth_normal = depth_normal_texture.Load(int3(pixel, 0));
	float age = age_texture.Load(int3(pixel, 0));
	if (center_depth_normal.x <= 0.0 || age >= REFLECTION_SPATIAL_VARIANCE_FRAMES)
	{
		destination[pixel] = center;
		return;
	}

	float roughness = (surface_normal_roughness.Load(int3(pixel, 0)) >> 24) / 255.0;
	float phi_normal = lerp(REFLECTION_PHI_NORMAL, REFLECTION_PHI_NORMAL * REFLECTION_ROUGHNESS_PHI_NORMAL_SCALE, roughness);
	float3 center_normal = SvgfUnpackNormal(center_depth_normal);
	float phi_depth = max(center_depth_normal.y, center_depth_normal.x * REFLECTION_DEPTH_SLOPE_FLOOR) * 3.0;
	float center_luminance = DenoiserLuminance(center.rgb);

	float weight_sum = 0.0;
	float3 radiance_sum = float3(0.0, 0.0, 0.0);
	float2 moments_sum = float2(0.0, 0.0);
	for (int offset_y = -3; offset_y <= 3; ++offset_y)
	{
		for (int offset_x = -3; offset_x <= 3; ++offset_x)
		{
			int2 tap_pixel = pixel + int2(offset_x, offset_y);
			if (any(tap_pixel < int2(0, 0)) || any(tap_pixel > screen_max))
			{
				continue;
			}
			float4 tap_depth_normal = depth_normal_texture.Load(int3(tap_pixel, 0));
			if (tap_depth_normal.x <= 0.0)
			{
				continue;
			}
			float4 tap = source.Load(int3(tap_pixel, 0));
			float geometry_weight = SvgfDepthNormalWeight(center_depth_normal.x, tap_depth_normal.x, phi_depth * length(float2(offset_x, offset_y)), center_normal, SvgfUnpackNormal(tap_depth_normal), phi_normal);
			float weight = geometry_weight * SvgfLuminanceWeight(center_luminance, DenoiserLuminance(tap.rgb), REFLECTION_PHI_LUMINANCE);
			radiance_sum += tap.rgb * weight;
			moments_sum += moments_texture.Load(int3(tap_pixel, 0)) * weight;
			weight_sum += weight;
		}
	}

	weight_sum = max(weight_sum, 1e-6);
	float2 moments = moments_sum / weight_sum;
	float variance = max(0.0, moments.y - moments.x * moments.x) * REFLECTION_SPATIAL_VARIANCE_FRAMES / max(age, 1.0);
	destination[pixel] = float4(radiance_sum / weight_sum, variance);
}

float4 RaytracingReflectionATrous(Texture2D<float4> source, Texture2D<float4> depth_normal_texture, Texture2D<uint> surface_normal_roughness, int2 pixel, int2 screen_max, int step)
{
	float4 center = source.Load(int3(pixel, 0));
	float4 center_depth_normal = depth_normal_texture.Load(int3(pixel, 0));
	if (center_depth_normal.x <= 0.0)
	{
		return float4(0.0, 0.0, 0.0, 0.0);
	}

	float roughness = (surface_normal_roughness.Load(int3(pixel, 0)) >> 24) / 255.0;
	float phi_normal = lerp(REFLECTION_PHI_NORMAL, REFLECTION_PHI_NORMAL * REFLECTION_ROUGHNESS_PHI_NORMAL_SCALE, roughness);
	float3 center_normal = SvgfUnpackNormal(center_depth_normal);
	float phi_luminance = REFLECTION_PHI_LUMINANCE * sqrt(max(0.0, SvgfVarianceCenter(source, pixel, screen_max).x) + 1e-10);
	float phi_depth = max(center_depth_normal.y, center_depth_normal.x * REFLECTION_DEPTH_SLOPE_FLOOR) * float(step);
	float center_luminance = DenoiserLuminance(center.rgb);

	float weight_sum = 1.0;
	float3 radiance_sum = center.rgb;
	float variance_sum = center.a;
	[unroll]
	for (int offset_y = -2; offset_y <= 2; ++offset_y)
	{
		[unroll]
		for (int offset_x = -2; offset_x <= 2; ++offset_x)
		{
			if (offset_x == 0 && offset_y == 0)
			{
				continue;
			}
			int2 tap_pixel = pixel + int2(offset_x, offset_y) * step;
			if (any(tap_pixel < int2(0, 0)) || any(tap_pixel > screen_max))
			{
				continue;
			}
			float4 tap_depth_normal = depth_normal_texture.Load(int3(tap_pixel, 0));
			if (tap_depth_normal.x <= 0.0)
			{
				continue;
			}
			float4 tap = source.Load(int3(tap_pixel, 0));
			float kernel_weight = SVGF_ATROUS_KERNEL[abs(offset_x)] * SVGF_ATROUS_KERNEL[abs(offset_y)];
			float geometry_weight = SvgfDepthNormalWeight(center_depth_normal.x, tap_depth_normal.x, phi_depth * length(float2(offset_x, offset_y)), center_normal, SvgfUnpackNormal(tap_depth_normal), phi_normal);
			float weight = kernel_weight * geometry_weight * SvgfLuminanceWeight(center_luminance, DenoiserLuminance(tap.rgb), phi_luminance);
			radiance_sum += tap.rgb * weight;
			variance_sum += tap.a * weight * weight;
			weight_sum += weight;
		}
	}
	return float4(radiance_sum / weight_sum, variance_sum / (weight_sum * weight_sum));
}

[numthreads(8, 8, 1)]
void ATrous(uint3 id : SV_DispatchThreadID)
{
	uint iteration = dispatch_bindings.constants_;
	Texture2D<float4> source = ResourceDescriptorHeap[atrous_bindings.raytracing_reflection_source_];
	uint width;
	uint height;
	source.GetDimensions(width, height);
	if (id.x >= width || id.y >= height)
	{
		return;
	}

	Texture2D<float4> depth_normal_texture = ResourceDescriptorHeap[atrous_bindings.raytracing_reflection_depth_normal_];
	Texture2D<uint> surface_normal_roughness = ResourceDescriptorHeap[atrous_bindings.material_normal_roughness_];
	RWTexture2D<float4> destination = ResourceDescriptorHeap[atrous_bindings.raytracing_reflection_destination_];
	float4 filtered = RaytracingReflectionATrous(source, depth_normal_texture, surface_normal_roughness, int2(id.xy), int2(width, height) - 1, int(1u << iteration));
	if (iteration < 2)
	{
		destination[id.xy] = filtered;
		return;
	}
	ConstantBuffer<RaytracingReflectionConstantBuffer> constants = ResourceDescriptorHeap[atrous_bindings.raytracing_reflection_constants_];
	bool surface = depth_normal_texture.Load(int3(id.xy, 0)).x > 0.0;
	destination[id.xy] = surface ? float4(DenoiserSanitizeRadiance(filtered.rgb), max(constants.strength_, 0.0)) : float4(0.0, 0.0, 0.0, 0.0);
}

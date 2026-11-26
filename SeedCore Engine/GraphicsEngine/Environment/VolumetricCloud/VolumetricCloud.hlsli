#ifndef __VOLUMETRIC_CLOUD_HLSL__
#define __VOLUMETRIC_CLOUD_HLSL__

static const float CLOUD_PI = 3.14159265358979;

struct VolumetricCloudConstantBuffer
{
	float cloud_bottom_;
	float cloud_top_;

	float coverage_;

	float density_scale_;

	float3 cloud_albedo_;

	float scattering_g_;

	uint step_count_;
	uint light_step_count_;

	float noise_scale_;

	float wind_x_;

	uint frame_index_;

	float cloud_type_;

	float rain_;

	float detail_scale_;

	float3 sky_zenith_color_;

	float sun_size_;

	float sun_emissive_;

	float3 sky_horizon_color_;

	float sky_brightness_;

	float3 ground_color_;

	uint procedural_sky_enabled_;

	float planet_radius_;

	float max_march_distance_;

	float lod_distance_;

	float aerial_density_;

	float weather_scale_;

	float weather_amount_;

	float detail_strength_;

	float detail_wind_speed_;

	float phase_backward_;
	float phase_blend_;

	float powder_strength_;

	float ambient_strength_;

	uint multi_scatter_octaves_;
	float multi_scatter_attenuation_;
	float multi_scatter_contribution_;
	float multi_scatter_eccentricity_;

	float wind_z_;

	float ibl_intensity_;

	float light_density_;

	float sun_strength_;
};

float CloudCurvature(float3 direction, float planet_radius)
{
	float horizontal_squared = saturate(1.0 - direction.y * direction.y);
	return horizontal_squared / (2.0 * max(planet_radius, 1.0));
}

float CloudAltitudeAt(float origin_height, float direction_y, float curvature, float t)
{
	return origin_height + direction_y * t + curvature * t * t;
}

bool CloudSolveAltitude(float origin_height, float direction_y, float curvature, float target, out float near_t, out float far_t)
{
	near_t = 0.0;
	far_t = 0.0;

	if (curvature < 1e-12)
	{
		if (abs(direction_y) < 1e-6)
		{
			return false;
		}

		float linear_t = (target - origin_height) / direction_y;
		near_t = linear_t;
		far_t = linear_t;
		return linear_t > 0.0;
	}

	float c = origin_height - target;
	float discriminant = direction_y * direction_y - 4.0 * curvature * c;

	if (discriminant < 0.0)
	{
		return false;
	}

	float root = sqrt(discriminant);
	float q = -0.5 * (direction_y + (direction_y >= 0.0 ? root : -root));

	float t0 = q / curvature;
	float t1 = abs(q) > 1e-20 ? c / q : t0;

	near_t = min(t0, t1);
	far_t = max(t0, t1);
	return true;
}

bool CloudLayerInterval(float3 origin, float3 direction, VolumetricCloudConstantBuffer tuning, out float t_enter, out float t_exit)
{
	t_enter = 0.0;
	t_exit = 0.0;

	float curvature = CloudCurvature(direction, tuning.planet_radius_);
	float height = origin.y;

	float ground_limit = tuning.max_march_distance_;
	float ground_near;
	float ground_far;
	if (CloudSolveAltitude(height, direction.y, curvature, 0.0, ground_near, ground_far) && ground_near > 0.0)
	{
		ground_limit = ground_near;
	}

	float bottom_near;
	float bottom_far;
	float top_near;
	float top_far;
	bool hits_bottom = CloudSolveAltitude(height, direction.y, curvature, tuning.cloud_bottom_, bottom_near, bottom_far);
	bool hits_top = CloudSolveAltitude(height, direction.y, curvature, tuning.cloud_top_, top_near, top_far);

	if (height < tuning.cloud_bottom_)
	{
		if (!hits_bottom || !hits_top)
		{
			return false;
		}
		t_enter = max(bottom_far, 0.0);
		t_exit = max(top_far, 0.0);
	}
	else if (height > tuning.cloud_top_)
	{
		if (!hits_top)
		{
			return false;
		}
		t_enter = max(top_near, 0.0);
		t_exit = hits_bottom ? max(bottom_near, 0.0) : max(top_far, 0.0);
	}
	else
	{
		t_enter = 0.0;
		if (direction.y >= 0.0)
		{
			t_exit = hits_top ? top_far : tuning.max_march_distance_;
		}
		else
		{
			t_exit = hits_bottom ? bottom_near : (hits_top ? top_far : tuning.max_march_distance_);
		}
	}

	float far_limit = min(ground_limit, tuning.max_march_distance_);
	t_exit = min(t_exit, far_limit);

	return t_exit > t_enter && t_enter < far_limit;
}

float CloudDensity(float3 world_position, float altitude, VolumetricCloudConstantBuffer tuning, float total_time,
	Texture3D<float> shape_noise, Texture3D<float> detail_noise, SamplerState wrap_sampler, float detail_strength)
{
	float layer_thickness = max(tuning.cloud_top_ - tuning.cloud_bottom_, 0.0001);
	float height01 = (altitude - tuning.cloud_bottom_) / layer_thickness;
	if (height01 <= 0.0 || height01 >= 1.0)
	{
		return 0.0;
	}

	float2 wind_offset = float2(tuning.wind_x_, tuning.wind_z_) * total_time;

	float2 weather_position = world_position.xz * tuning.weather_scale_ + wind_offset * (tuning.weather_scale_ / max(tuning.noise_scale_, 1e-8));
	float weather = shape_noise.SampleLevel(wrap_sampler, float3(weather_position, 0.713), 0);
	weather = smoothstep(0.35, 0.75, weather);

	float coverage = saturate(tuning.coverage_ + tuning.rain_ * 0.25);
	coverage = saturate(coverage * lerp(1.0 - tuning.weather_amount_, 1.0 + tuning.weather_amount_, weather));
	if (coverage <= 0.0)
	{
		return 0.0;
	}

	float top_extent = lerp(0.35, 1.0, weather);
	float profile_height = saturate(height01 / max(top_extent, 0.0001));

	float stratus_gradient = saturate(profile_height / 0.08) * saturate((0.3 - profile_height) / 0.15);
	float cumulus_gradient = saturate(profile_height / 0.07) * saturate((1.0 - profile_height) / 0.6);
	float height_gradient = lerp(stratus_gradient, cumulus_gradient, saturate(tuning.cloud_type_));
	if (height_gradient <= 0.0)
	{
		return 0.0;
	}

	float3 shape_position = float3(world_position.x * tuning.noise_scale_ + wind_offset.x, (altitude - tuning.cloud_bottom_) * (0.5 / layer_thickness), world_position.z * tuning.noise_scale_ + wind_offset.y);
	float shape = shape_noise.SampleLevel(wrap_sampler, shape_position, 0);

	shape = saturate((shape - 0.3) / 0.4);

	float base = saturate((shape * height_gradient - (1.0 - coverage)) / max(coverage, 0.0001)) * coverage;
	if (base <= 0.0)
	{
		return 0.0;
	}

	if (detail_strength > 0.0)
	{
		float2 detail_wind = wind_offset * tuning.detail_wind_speed_;
		float3 detail_position = world_position * tuning.noise_scale_ * tuning.detail_scale_
			+ float3(detail_wind.x, -length(detail_wind) * 0.5, detail_wind.y);
		float detail = detail_noise.SampleLevel(wrap_sampler, detail_position, 0);

		float detail_modifier = lerp(detail, 1.0 - detail, saturate(height01 * 10.0));
		float erosion = detail_modifier * tuning.detail_strength_ * 0.5 * detail_strength;
		base = saturate((base - erosion) / max(1.0 - erosion, 0.0001));
	}

	float density_boost = 1.0 + tuning.rain_ * 1.5;
	return base * tuning.density_scale_ * density_boost;
}

float CloudPhaseHenyeyGreenstein(float cos_theta, float g)
{
	float g2 = g * g;
	float denominator = 1.0 + g2 - 2.0 * g * cos_theta;
	return (1.0 - g2) / (4.0 * CLOUD_PI * denominator * sqrt(max(denominator, 0.0001)));
}

float CloudPhase(float cos_theta, VolumetricCloudConstantBuffer tuning, float eccentricity)
{
	float forward = CloudPhaseHenyeyGreenstein(cos_theta, tuning.scattering_g_ * eccentricity);
	float backward = CloudPhaseHenyeyGreenstein(cos_theta, -tuning.phase_backward_ * eccentricity);
	return lerp(forward, backward, saturate(tuning.phase_blend_));
}

float CloudPowder(float light_optical_depth, float cos_theta, float strength)
{
	float powder = 1.0 - exp(-light_optical_depth * 2.0);
	float weight = saturate(1.0 - (cos_theta * 0.5 + 0.5)) * saturate(strength);
	return lerp(1.0, powder, weight);
}

#define CLOUD_MULTI_SCATTER_MAX_OCTAVES 8

struct CloudMultiScatter
{
	float weight_[CLOUD_MULTI_SCATTER_MAX_OCTAVES];
	float attenuation_[CLOUD_MULTI_SCATTER_MAX_OCTAVES];
};

CloudMultiScatter CloudMultiScatterSetup(float cos_theta, VolumetricCloudConstantBuffer tuning)
{
	CloudMultiScatter result;

	uint octaves = min(max(tuning.multi_scatter_octaves_, 1u), (uint)CLOUD_MULTI_SCATTER_MAX_OCTAVES);

	float attenuation = 1.0;
	float contribution = 1.0;
	float eccentricity = 1.0;

	[unroll]
	for (uint octave = 0; octave < CLOUD_MULTI_SCATTER_MAX_OCTAVES; octave++)
	{
		result.weight_[octave] = octave < octaves ? contribution * CloudPhase(cos_theta, tuning, eccentricity) * 4.0 * CLOUD_PI * tuning.sun_strength_ : 0.0;
		result.attenuation_[octave] = attenuation;

		attenuation *= tuning.multi_scatter_attenuation_;
		contribution *= tuning.multi_scatter_contribution_;
		eccentricity *= tuning.multi_scatter_eccentricity_;
	}

	return result;
}

float CloudSunLight(float light_optical_depth, float powder, CloudMultiScatter multi_scatter)
{
	float result = 0.0;

	[unroll]
	for (uint octave = 0; octave < CLOUD_MULTI_SCATTER_MAX_OCTAVES; octave++)
	{
		float transmittance = exp(-light_optical_depth * multi_scatter.attenuation_[octave]);

		float powder_term = octave == 0 ? powder : 1.0;

		result += multi_scatter.weight_[octave] * transmittance * powder_term;
	}

	return result;
}

float CloudLightOpticalDepth(float3 position, float altitude, float3 light_direction,
	VolumetricCloudConstantBuffer tuning, float total_time,
	Texture3D<float> shape_noise, Texture3D<float> detail_noise, SamplerState wrap_sampler,
	uint step_count, float jitter)
{
	float layer_thickness = max(tuning.cloud_top_ - tuning.cloud_bottom_, 1.0);
	float curvature = CloudCurvature(light_direction, tuning.planet_radius_);

	float march_distance = layer_thickness * 2.0;

	float exit_near;
	float exit_far;
	if (CloudSolveAltitude(altitude, light_direction.y, curvature, tuning.cloud_top_, exit_near, exit_far))
	{
		float candidate = exit_near > 0.0 ? exit_near : exit_far;
		if (candidate > 0.0)
		{
			march_distance = candidate;
		}
	}

	march_distance = clamp(march_distance, layer_thickness * 0.25, layer_thickness * 12.0);

	uint steps = max(step_count, 1u);

	const float growth = 1.6;
	float weight_sum = (pow(growth, float(steps)) - 1.0) / (growth - 1.0);
	float unit = march_distance / max(weight_sum, 0.0001);

	float optical_depth = 0.0;
	float travelled = 0.0;
	float weight = 1.0;

	for (uint step = 0; step < steps; step++)
	{
		float segment = unit * weight;
		float offset = travelled + segment * (0.5 + (jitter - 0.5) * 0.8);

		float sample_altitude = CloudAltitudeAt(altitude, light_direction.y, curvature, offset);
		float3 sample_position = position + light_direction * offset;

		optical_depth += CloudDensity(sample_position, sample_altitude, tuning, total_time,
			shape_noise, detail_noise, wrap_sampler, 0.0) * segment;

		travelled += segment;
		weight *= growth;
	}

	return optical_depth * tuning.light_density_;
}

float3 ProceduralSkyColor(float3 direction, float3 sun_direction, float3 sun_radiance, VolumetricCloudConstantBuffer tuning)
{
	float up = direction.y;

	float horizon_blend = pow(saturate(1.0 - max(up, 0.0)), 1.8);
	float3 sky = lerp(tuning.sky_zenith_color_, tuning.sky_horizon_color_, horizon_blend);
	float3 color = lerp(sky, tuning.ground_color_, saturate(-up * 6.0));

	color *= tuning.sky_brightness_;

	if (up > -0.05)
	{
		float cos_view_sun = dot(direction, sun_direction);

		float cos_disc = cos(tuning.sun_size_);
		float cos_rim = cos(tuning.sun_size_ * 1.6);
		float disc = smoothstep(cos_rim, cos_disc, cos_view_sun);

		float glow = pow(saturate(cos_view_sun), 96.0) * 0.06 + pow(saturate(cos_view_sun), 8.0) * 0.02;

		color += sun_radiance * (disc * tuning.sun_emissive_ + glow);
	}

	return color;
}

#endif

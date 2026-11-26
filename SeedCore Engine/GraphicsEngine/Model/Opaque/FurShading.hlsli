float3 EvalDirectLightFur(float3 normal, float3 fibre_tangent, float3 view, float3 light_direction, float3 diffuse_color, float3 specular_color, float shininess)
{
	float normal_dot_light = dot(normal, light_direction);

	float wrap = 0.5;
	float wrapped_diffuse = saturate((normal_dot_light + wrap) / (1.0 + wrap));

	float fibre_length = length(fibre_tangent);
	if (fibre_length < 1e-3)
	{
		return diffuse_color * wrapped_diffuse;
	}

	float3 tangent = fibre_tangent / fibre_length;
	float tangent_dot_light = dot(tangent, light_direction);
	float tangent_dot_view = dot(tangent, view);

	float sin_tangent_light = sqrt(saturate(1.0 - tangent_dot_light * tangent_dot_light));
	float sin_tangent_view = sqrt(saturate(1.0 - tangent_dot_view * tangent_dot_view));

	float diffuse_term = lerp(sin_tangent_light, wrapped_diffuse, 0.5);

	float specular_cosine = sin_tangent_light * sin_tangent_view - tangent_dot_light * tangent_dot_view;
	float specular_term = pow(saturate(specular_cosine), shininess);

	float visibility = saturate(normal_dot_light + 0.35);

	return (diffuse_color * diffuse_term + specular_color * specular_term) * visibility;
}

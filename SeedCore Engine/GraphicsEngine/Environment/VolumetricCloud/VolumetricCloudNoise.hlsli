#ifndef __VOLUMETRIC_CLOUD_NOISE_HLSL__
#define __VOLUMETRIC_CLOUD_NOISE_HLSL__

float3 CloudCellHash(float3 cell)
{
	float3 p = float3(
		dot(cell, float3(127.1, 311.7, 74.7)),
		dot(cell, float3(269.5, 183.3, 246.1)),
		dot(cell, float3(113.5, 271.9, 124.6)));
	return frac(sin(p) * 43758.5453123);
}

float CloudRemap(float value, float low, float high, float new_low, float new_high)
{
	return new_low + saturate((value - low) / max(high - low, 0.0001)) * (new_high - new_low);
}

float TileableWorley(float3 p, float cells)
{
	float3 grid_position = p * cells;
	float3 base_cell = floor(grid_position);
	float3 fraction = grid_position - base_cell;

	float min_distance_squared = 1e5;

	[unroll]
	for (int z = -1; z <= 1; z++)
	{
		[unroll]
		for (int y = -1; y <= 1; y++)
		{
			[unroll]
			for (int x = -1; x <= 1; x++)
			{
				float3 offset = float3(x, y, z);
				float3 wrapped_cell = fmod(base_cell + offset + cells, cells);
				float3 feature = CloudCellHash(wrapped_cell);
				float3 delta = offset + feature - fraction;
				min_distance_squared = min(min_distance_squared, dot(delta, delta));
			}
		}
	}

	return sqrt(min_distance_squared);
}

float TileableWorleyFbm(float3 p, float base_cells, int octaves)
{
	float total = 0.0;
	float amplitude = 0.5;
	float amplitude_sum = 0.0;
	float cells = base_cells;

	for (int octave = 0; octave < octaves; octave++)
	{
		total += (1.0 - saturate(TileableWorley(p, cells))) * amplitude;
		amplitude_sum += amplitude;
		amplitude *= 0.5;
		cells *= 2.0;
	}

	return total / max(amplitude_sum, 0.0001);
}

float3 CloudCellGradient(float3 cell)
{
	float3 v = CloudCellHash(cell) * 2.0 - 1.0;
	return v * rsqrt(max(dot(v, v), 0.0001));
}

float TileablePerlin(float3 p, float cells)
{
	float3 grid_position = p * cells;
	float3 base_cell = floor(grid_position);
	float3 fraction = grid_position - base_cell;
	float3 fade = fraction * fraction * fraction * (fraction * (fraction * 6.0 - 15.0) + 10.0);

	float total = 0.0;

	[unroll]
	for (int z = 0; z <= 1; z++)
	{
		[unroll]
		for (int y = 0; y <= 1; y++)
		{
			[unroll]
			for (int x = 0; x <= 1; x++)
			{
				float3 offset = float3(x, y, z);
				float3 wrapped_cell = fmod(base_cell + offset + cells, cells);
				float value = dot(CloudCellGradient(wrapped_cell), fraction - offset);
				float3 blend = lerp(1.0 - fade, fade, offset);
				total += value * blend.x * blend.y * blend.z;
			}
		}
	}

	return total;
}

float TileablePerlinFbm(float3 p, float base_cells, int octaves)
{
	float total = 0.0;
	float amplitude = 0.5;
	float amplitude_sum = 0.0;
	float cells = base_cells;

	for (int octave = 0; octave < octaves; octave++)
	{
		total += TileablePerlin(p, cells) * amplitude;
		amplitude_sum += amplitude;
		amplitude *= 0.5;
		cells *= 2.0;
	}

	return saturate(total / max(amplitude_sum, 0.0001) * 0.7 + 0.5);
}

float TileablePerlinWorley(float3 p, float base_cells, int octaves)
{
	float worley = TileableWorleyFbm(p, base_cells, octaves);
	float perlin = TileablePerlinFbm(p, base_cells, octaves);

	return saturate(worley * CloudRemap(perlin, 0.25, 0.75, 0.55, 1.45));
}

#endif

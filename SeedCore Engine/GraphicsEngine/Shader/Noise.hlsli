#ifndef __NOISE_HLSL__
#define __NOISE_HLSL__

static const float NOISE_PI = 3.14159265358979;

float SinHash(float2 p)
{
	return frac(sin(dot(p, float2(12.9898, 78.233))) * 43758.5453123);
}

float SinHash(float3 p)
{
	return frac(sin(dot(p, float3(12.9898, 78.233, 45.164))) * 43758.5453123);
}

float2 SinHash2(float2 p)
{
	return frac(sin(float2(dot(p, float2(127.1, 311.7)), dot(p, float2(269.5, 183.3)))) * 43758.5453123);
}

float3 SinHash3(float3 p)
{
	float3 h = float3(dot(p, float3(127.1, 311.7, 74.7)), dot(p, float3(269.5, 183.3, 246.1)), dot(p, float3(113.5, 271.9, 124.6)));
	return frac(sin(h) * 43758.5453123);
}

float4 SinHash4(float3 p)
{
	float4 h = float4(dot(p, float3(127.1, 311.7, 74.7)), dot(p, float3(269.5, 183.3, 246.1)), dot(p, float3(113.5, 271.9, 124.6)), dot(p, float3(53.1, 34.7, 91.3)));
	return frac(sin(h) * 43758.5453123);
}

float IQHash(float2 p)
{
	float3 p3 = frac(float3(p.xyx) * 0.1031);
	p3 += dot(p3, p3.yzx + 33.33);
	return frac((p3.x + p3.y) * p3.z);
}

float IQHash(float3 p)
{
	p = frac(p * 0.1031);
	p += dot(p, p.yzx + 33.33);
	return frac((p.x + p.y) * p.z);
}

float2 IQHash2(float2 p)
{
	float3 p3 = frac(float3(p.xyx) * float3(0.1031, 0.1030, 0.0973));
	p3 += dot(p3, p3.yzx + 33.33);
	return frac((p3.xx + p3.yz) * p3.zy);
}

float3 IQHash3(float3 p)
{
	p = frac(p * float3(0.1031, 0.1030, 0.0973));
	p += dot(p, p.yxz + 33.33);
	return frac((p.xxy + p.yzz) * p.zyx);
}

float4 IQHash4(float3 p)
{
	float4 p4 = frac(float4(p.xyzx) * float4(0.1031, 0.1030, 0.0973, 0.1099));
	p4 += dot(p4, p4.wzxy + 33.33);
	return frac((p4.xxyz + p4.yzzw) * p4.zywx);
}

uint WangHash(uint seed)
{
	seed = (seed ^ 61u) ^ (seed >> 16);
	seed *= 9u;
	seed ^= seed >> 4;
	seed *= 0x27d4eb2du;
	seed ^= seed >> 15;
	return seed;
}

uint PackSeed(float2 p)
{
	return WangHash(asuint(p.x) ^ WangHash(asuint(p.y) + 0x9e3779b9u));
}

uint PackSeed(float3 p)
{
	uint s = asuint(p.x);
	s = WangHash(s ^ (asuint(p.y) + 0x9e3779b9u));
	s = WangHash(s ^ (asuint(p.z) + 0x9e3779b9u + (s << 6) + (s >> 2)));
	return s;
}

float WangHash1(float2 p)
{
	return WangHash(PackSeed(p)) / 4294967295.0; 
}

float WangHash1(float3 p) 
{
	return WangHash(PackSeed(p)) / 4294967295.0; 
}

float2 WangHash2(float2 p)
{
	uint s = PackSeed(p);
	uint a = WangHash(s);
	uint b = WangHash(s ^ 0x68bc21ebu);
	return float2(a, b) / 4294967295.0;
}

float3 WangHash3(float3 p)
{
	uint s = PackSeed(p);
	uint a = WangHash(s);
	uint b = WangHash(s ^ 0x68bc21ebu);
	uint c = WangHash(s ^ 0xb5297a4du);
	return float3(a, b, c) / 4294967295.0;
}

float4 WangHash4(float3 p)
{
	uint s = PackSeed(p);
	uint a = WangHash(s);
	uint b = WangHash(s ^ 0x68bc21ebu);
	uint c = WangHash(s ^ 0xb5297a4du);
	uint d = WangHash(s ^ 0x1b56c4e9u);
	return float4(a, b, c, d) / 4294967295.0;
}

uint PCGHash(uint seed)
{
	uint state = seed * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return (word >> 22u) ^ word;
}

float PCGHash1(float2 p) 
{
	return PCGHash(PackSeed(p)) / 4294967295.0; 
}

float PCGHash1(float3 p) 
{
	return PCGHash(PackSeed(p)) / 4294967295.0;
}

float2 PCGHash2(float2 p)
{
	uint s = PackSeed(p);
	return float2(PCGHash(s), PCGHash(s ^ 0x68bc21ebu)) / 4294967295.0;
}

float3 PCGHash3(float3 p)
{
	uint s = PackSeed(p);
	return float3(PCGHash(s), PCGHash(s ^ 0x68bc21ebu), PCGHash(s ^ 0xb5297a4du)) / 4294967295.0;
}

float4 PCGHash4(float3 p)
{
	uint s = PackSeed(p);
	return float4(PCGHash(s), PCGHash(s ^ 0x68bc21ebu), PCGHash(s ^ 0xb5297a4du), PCGHash(s ^ 0x1b56c4e9u)) / 4294967295.0;
}

float LcgNext(inout uint state)
{
	state = state * 1664525u + 1013904223u;
	return state / 4294967295.0;
}

float XorshiftNext(inout uint state)
{
	state ^= state << 13;
	state ^= state >> 17;
	state ^= state << 5;
	return state / 4294967295.0;
}

float PcgNext(inout uint state)
{
	state = state * 747796405u + 2891336453u;
	uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
	return ((word >> 22u) ^ word) / 4294967295.0;
}

uint2 Add64(uint2 a, uint2 b)
{
	uint lo = a.x + b.x;
	uint carry = (lo < a.x) ? 1u : 0u;
	return uint2(lo, a.y + b.y + carry);
}

uint2 Xor64(uint2 a, uint2 b)
{
	return uint2(a.x ^ b.x, a.y ^ b.y); 
}

uint2 ShiftRight64(uint2 v, uint n)
{
	if (n == 0u) 
	{
		return v;
	}
	if (n < 32u) 
	{
		return uint2((v.x >> n) | (v.y << (32u - n)), v.y >> n);
	}
	return uint2(v.y >> (n - 32u), 0u);
}

void Mul32To64(uint a, uint b, out uint lo, out uint hi)
{
	uint a_l = a & 0xFFFFu, a_h = a >> 16u;
	uint b_l = b & 0xFFFFu, b_h = b >> 16u;

	uint p_ll = a_l * b_l;
	uint p_lh = a_l * b_h;
	uint p_hl = a_h * b_l;
	uint p_hh = a_h * b_h;

	uint cross = p_lh + p_hl;
	uint cross_carry = (cross < p_lh) ? 1u : 0u;

	uint lo_sum = p_ll + (cross << 16u);
	uint lo_carry = (lo_sum < p_ll) ? 1u : 0u;

	lo = lo_sum;
	hi = p_hh + (cross >> 16u) + (cross_carry << 16u) + lo_carry;
}

uint2 Mul64Low(uint2 a, uint2 b)
{
	uint lo, hi;
	Mul32To64(a.x, b.x, lo, hi);
	hi += a.x * b.y + a.y * b.x;
	return uint2(lo, hi);
}

float SplitMix64Next(inout uint2 state)
{
	state = Add64(state, uint2(0x7f4a7c15u, 0x9e3779b9u));
	uint2 z = state;
	z = Xor64(z, ShiftRight64(z, 30u));
	z = Mul64Low(z, uint2(0x1ce4e5b9u, 0xbf58476du));
	z = Xor64(z, ShiftRight64(z, 27u));
	z = Mul64Low(z, uint2(0x133111ebu, 0x94d049bbu));
	z = Xor64(z, ShiftRight64(z, 31u));
	uint2 top = ShiftRight64(z, 40u);
	return float(top.x) / 16777215.0;
}

#define NOISE_MT_N 624
struct MersenneTwisterState
{
	uint data_[NOISE_MT_N];
	uint index_;
};

void MersenneTwisterSeed(inout MersenneTwisterState mt, uint seed)
{
	mt.data_[0] = seed;
	for (uint i = 1; i < NOISE_MT_N; ++i)
	{
		mt.data_[i] = 1812433253u * (mt.data_[i - 1] ^ (mt.data_[i - 1] >> 30)) + i;
	}
	mt.index_ = NOISE_MT_N;
}

void MersenneTwisterTwist(inout MersenneTwisterState mt)
{
	const uint upper_mask = 0x80000000u;
	const uint lower_mask = 0x7fffffffu;
	for (uint i = 0; i < NOISE_MT_N; ++i)
	{
		uint x = (mt.data_[i] & upper_mask) | (mt.data_[(i + 1) % NOISE_MT_N] & lower_mask);
		uint x_a = x >> 1;
		if (x & 1u) 
		{
			x_a ^= 0x9908b0dfu; 
		}
		mt.data_[i] = mt.data_[(i + 397) % NOISE_MT_N] ^ x_a;
	}
	mt.index_ = 0;
}

float MersenneTwisterNext(inout MersenneTwisterState mt)
{
	if (mt.index_ >= NOISE_MT_N)
	{
		MersenneTwisterTwist(mt);
	}
	uint y = mt.data_[mt.index_++];
	y ^= y >> 11;
	y ^= (y << 7) & 0x9d2c5680u;
	y ^= (y << 15) & 0xefc60000u;
	y ^= y >> 18;
	return y / 4294967295.0;
}

uint SeedFromPixel(uint2 pixel, uint frame_index)
{
	return WangHash(pixel.x * 1973u + pixel.y * 9277u + frame_index * 26699u);
}

float Rand(inout uint state) 
{
	return PcgNext(state); 
}

float2 Rand2(inout uint state) 
{
	return float2(Rand(state), Rand(state)); 
}

float3 Rand3(inout uint state)
{
	return float3(Rand(state), Rand(state), Rand(state)); 
}

float4 Rand4(inout uint state) 
{
	return float4(Rand(state), Rand(state), Rand(state), Rand(state)); 
}

float VanDerCorput(uint index, uint base)
{
	if (base == 2u)
	{
		index = (index << 16u) | (index >> 16u);
		index = ((index & 0x55555555u) << 1u) | ((index & 0xAAAAAAAAu) >> 1u);
		index = ((index & 0x33333333u) << 2u) | ((index & 0xCCCCCCCCu) >> 2u);
		index = ((index & 0x0F0F0F0Fu) << 4u) | ((index & 0xF0F0F0F0u) >> 4u);
		index = ((index & 0x00FF00FFu) << 8u) | ((index & 0xFF00FF00u) >> 8u);
		return index / 4294967296.0;
	}
	float result = 0.0;
	float denom = 1.0;
	float inv_base = 1.0 / float(base);
	while (index > 0u)
	{
		denom *= inv_base;
		result += float(index % base) * denom;
		index /= base;
	}
	return result;
}

float Halton(uint index, uint base)
{
	return VanDerCorput(index, base);
}

float2 Halton2(uint index)
{
	return float2(VanDerCorput(index, 2u), VanDerCorput(index, 3u));
}

float2 Hammersley2(uint index, uint sample_count)
{
	return float2(float(index) / float(sample_count), VanDerCorput(index, 2u));
}

float Sobol(uint index, uint dimension)
{
	uint result = 0u;
	uint direction = 1u << 31u;
	if (dimension == 1u)
	{
		uint v = 1u << 31u;
		for (uint bit = index; bit != 0u; bit >>= 1u)
		{
			if (bit & 1u) 
			{
				result ^= v; 
			}
			v ^= v >> 1u;
		}
		return result / 4294967296.0;
	}
	for (uint bit = index; bit != 0u; bit >>= 1u)
	{
		if (bit & 1u) 
		{
			result ^= direction; 
		}
		direction >>= 1u;
	}
	return result / 4294967296.0;
}

float2 Sobol2(uint index)
{
	return float2(Sobol(index, 0u), Sobol(index, 1u));
}

float2 R2Sequence2(uint index)
{
	const float g = 1.32471795724474602596;
	const float2 a = float2(1.0 / g, 1.0 / (g * g));
	return frac(0.5 + a * float(index));
}

float3 R2Sequence3(uint index)
{
	const float g = 1.22074408460575947536;
	const float3 a = float3(1.0 / g, 1.0 / (g * g), 1.0 / (g * g * g));
	return frac(0.5 + a * float(index));
}

float WhiteNoise(float2 p) 
{
	return IQHash(p);
}

float WhiteNoise(float3 p) 
{
	return IQHash(p); 
}

float ValueNoise(float2 p)
{
	float2 i = floor(p);
	float2 f = frac(p);
	f = f * f * (3.0 - 2.0 * f);
	float n00 = IQHash(i + float2(0, 0));
	float n10 = IQHash(i + float2(1, 0));
	float n01 = IQHash(i + float2(0, 1));
	float n11 = IQHash(i + float2(1, 1));
	return lerp(lerp(n00, n10, f.x), lerp(n01, n11, f.x), f.y);
}

float ValueNoise(float3 p)
{
	float3 i = floor(p);
	float3 f = frac(p);
	f = f * f * (3.0 - 2.0 * f);
	float n000 = IQHash(i + float3(0, 0, 0));
	float n100 = IQHash(i + float3(1, 0, 0));
	float n010 = IQHash(i + float3(0, 1, 0));
	float n110 = IQHash(i + float3(1, 1, 0));
	float n001 = IQHash(i + float3(0, 0, 1));
	float n101 = IQHash(i + float3(1, 0, 1));
	float n011 = IQHash(i + float3(0, 1, 1));
	float n111 = IQHash(i + float3(1, 1, 1));
	float nx00 = lerp(n000, n100, f.x);
	float nx10 = lerp(n010, n110, f.x);
	float nx01 = lerp(n001, n101, f.x);
	float nx11 = lerp(n011, n111, f.x);
	float nxy0 = lerp(nx00, nx10, f.y);
	float nxy1 = lerp(nx01, nx11, f.y);
	return lerp(nxy0, nxy1, f.z);
}

float PerlinNoise(float2 p)
{
	float2 i = floor(p);
	float2 f = frac(p);
	float2 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

	float2 g00 = normalize(IQHash2(i + float2(0, 0)) * 2.0 - 1.0);
	float2 g10 = normalize(IQHash2(i + float2(1, 0)) * 2.0 - 1.0);
	float2 g01 = normalize(IQHash2(i + float2(0, 1)) * 2.0 - 1.0);
	float2 g11 = normalize(IQHash2(i + float2(1, 1)) * 2.0 - 1.0);

	float n00 = dot(g00, f - float2(0, 0));
	float n10 = dot(g10, f - float2(1, 0));
	float n01 = dot(g01, f - float2(0, 1));
	float n11 = dot(g11, f - float2(1, 1));

	return lerp(lerp(n00, n10, u.x), lerp(n01, n11, u.x), u.y) * 1.4142136;
}

float PerlinNoise(float3 p)
{
	float3 i = floor(p);
	float3 f = frac(p);
	float3 u = f * f * f * (f * (f * 6.0 - 15.0) + 10.0);

	float3 g000 = normalize(IQHash3(i + float3(0, 0, 0)) * 2.0 - 1.0);
	float3 g100 = normalize(IQHash3(i + float3(1, 0, 0)) * 2.0 - 1.0);
	float3 g010 = normalize(IQHash3(i + float3(0, 1, 0)) * 2.0 - 1.0);
	float3 g110 = normalize(IQHash3(i + float3(1, 1, 0)) * 2.0 - 1.0);
	float3 g001 = normalize(IQHash3(i + float3(0, 0, 1)) * 2.0 - 1.0);
	float3 g101 = normalize(IQHash3(i + float3(1, 0, 1)) * 2.0 - 1.0);
	float3 g011 = normalize(IQHash3(i + float3(0, 1, 1)) * 2.0 - 1.0);
	float3 g111 = normalize(IQHash3(i + float3(1, 1, 1)) * 2.0 - 1.0);

	float n000 = dot(g000, f - float3(0, 0, 0));
	float n100 = dot(g100, f - float3(1, 0, 0));
	float n010 = dot(g010, f - float3(0, 1, 0));
	float n110 = dot(g110, f - float3(1, 1, 0));
	float n001 = dot(g001, f - float3(0, 0, 1));
	float n101 = dot(g101, f - float3(1, 0, 1));
	float n011 = dot(g011, f - float3(0, 1, 1));
	float n111 = dot(g111, f - float3(1, 1, 1));

	float nx00 = lerp(n000, n100, u.x);
	float nx10 = lerp(n010, n110, u.x);
	float nx01 = lerp(n001, n101, u.x);
	float nx11 = lerp(n011, n111, u.x);
	float nxy0 = lerp(nx00, nx10, u.y);
	float nxy1 = lerp(nx01, nx11, u.y);
	return lerp(nxy0, nxy1, u.z) * 1.1547005;
}

float SimplexNoise(float2 p)
{
	const float skew_factor = 0.36602540378;
	const float unskew_factor = 0.2113248654;

	float skew = (p.x + p.y) * skew_factor;
	float2 cell = floor(p + skew);
	float unskew = (cell.x + cell.y) * unskew_factor;
	float2 origin = cell - unskew;
	float2 d0 = p - origin;

	float2 offset1 = (d0.x > d0.y) ? float2(1, 0) : float2(0, 1);
	float2 d1 = d0 - offset1 + unskew_factor;
	float2 d2 = d0 - 1.0 + 2.0 * unskew_factor;

	float3 contribution = max(0.5 - float3(dot(d0, d0), dot(d1, d1), dot(d2, d2)), 0.0);
	contribution = contribution * contribution * contribution * contribution;

	float n0 = contribution.x * dot(normalize(IQHash2(cell) * 2.0 - 1.0), d0);
	float n1 = contribution.y * dot(normalize(IQHash2(cell + offset1) * 2.0 - 1.0), d1);
	float n2 = contribution.z * dot(normalize(IQHash2(cell + 1.0) * 2.0 - 1.0), d2);

	return 70.0 * (n0 + n1 + n2);
}

float SimplexNoise(float3 p)
{
	const float skew_factor = 1.0 / 3.0;
	const float unskew_factor = 1.0 / 6.0;

	float skew = (p.x + p.y + p.z) * skew_factor;
	float3 cell = floor(p + skew);
	float unskew = (cell.x + cell.y + cell.z) * unskew_factor;
	float3 origin = cell - unskew;
	float3 d0 = p - origin;

	float3 g = step(d0.yzx, d0.xyz);
	float3 l = 1.0 - g;
	float3 offset1 = min(g, l.zxy);
	float3 offset2 = max(g, l.zxy);

	float3 d1 = d0 - offset1 + unskew_factor;
	float3 d2 = d0 - offset2 + 2.0 * unskew_factor;
	float3 d3 = d0 - 1.0 + 3.0 * unskew_factor;

	float4 contribution = max(0.6 - float4(dot(d0, d0), dot(d1, d1), dot(d2, d2), dot(d3, d3)), 0.0);
	contribution = contribution * contribution * contribution * contribution;

	float n0 = contribution.x * dot(normalize(IQHash3(cell) * 2.0 - 1.0), d0);
	float n1 = contribution.y * dot(normalize(IQHash3(cell + offset1) * 2.0 - 1.0), d1);
	float n2 = contribution.z * dot(normalize(IQHash3(cell + offset2) * 2.0 - 1.0), d2);
	float n3 = contribution.w * dot(normalize(IQHash3(cell + 1.0) * 2.0 - 1.0), d3);

	return 32.0 * (n0 + n1 + n2 + n3);
}

float2 WorleyNoise(float2 p)
{
	float2 cell = floor(p);
	float2 f1_f2 = float2(8.0, 8.0);
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			float2 neighbor = float2(x, y);
			float2 point_offset = IQHash2(cell + neighbor);
			float d = length(neighbor + point_offset - frac(p));
			if (d < f1_f2.x)
			{
				f1_f2.y = f1_f2.x; f1_f2.x = d; 
			}
			else if (d < f1_f2.y)
			{
				f1_f2.y = d; 
			}
		}
	}
	return f1_f2;
}

float2 WorleyNoise(float3 p)
{
	float3 cell = floor(p);
	float2 f1_f2 = float2(8.0, 8.0);
	for (int z = -1; z <= 1; ++z)
	{
		for (int y = -1; y <= 1; ++y)
		{
			for (int x = -1; x <= 1; ++x)
			{
				float3 neighbor = float3(x, y, z);
				float3 point_offset = IQHash3(cell + neighbor);
				float d = length(neighbor + point_offset - frac(p));
				if (d < f1_f2.x) 
				{ 
					f1_f2.y = f1_f2.x; f1_f2.x = d;
				}
				else if (d < f1_f2.y)
				{ 
					f1_f2.y = d;
				}
			}
		}
	}
	return f1_f2;
}

float WaveletNoise(float2 p)
{
	return ValueNoise(p * 2.0) - 0.5 * ValueNoise(p);
}

float WaveletNoise(float3 p)
{
	return ValueNoise(p * 2.0) - 0.5 * ValueNoise(p);
}

float GaborNoise(float2 p, float frequency, float bandwidth)
{
	float2 cell = floor(p);
	float sum = 0.0;
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			float2 neighbor_cell = cell + float2(x, y);
			float4 h = IQHash4(float3(neighbor_cell, 0.0));
			float2 kernel_center = neighbor_cell + h.xy;
			float2 d = p - kernel_center;
			float orientation = h.z * 2.0 * NOISE_PI;
			float2 rotated = float2(d.x * cos(orientation) + d.y * sin(orientation), -d.x * sin(orientation) + d.y * cos(orientation));
			float gaussian = exp(-NOISE_PI * bandwidth * bandwidth * dot(d, d));
			float carrier = cos(2.0 * NOISE_PI * frequency * rotated.x);
			float weight = h.w * 2.0 - 1.0;
			sum += weight * gaussian * carrier;
		}
	}
	return sum;
}

float InterleavedGradientNoise(float2 p)
{
	const float3 magic = float3(0.06711056, 0.00583715, 52.9829189);
	return frac(magic.z * frac(dot(p, magic.xy)));
}

void DiamondSquareDiamondStep(RWTexture2D<float> heightmap, uint2 coord, uint step, float amplitude, inout uint rng_state)
{
	uint half_step = step / 2u;
	float sum = heightmap[coord + uint2(0, 0)] + heightmap[coord + uint2(step, 0)] + heightmap[coord + uint2(0, step)] + heightmap[coord + uint2(step, step)];
	float displacement = (Rand(rng_state) * 2.0 - 1.0) * amplitude;
	heightmap[coord + uint2(half_step, half_step)] = sum * 0.25 + displacement;
}

void DiamondSquareSquareStep(RWTexture2D<float> heightmap, uint2 coord, uint step, float amplitude, inout uint rng_state)
{
	uint half_step = step / 2u;
	float sum = heightmap[coord - uint2(half_step, 0)] + heightmap[coord + uint2(half_step, 0)] + heightmap[coord - uint2(0, half_step)] + heightmap[coord + uint2(0, half_step)];
	float displacement = (Rand(rng_state) * 2.0 - 1.0) * amplitude;
	heightmap[coord] = sum * 0.25 + displacement;
}

float BlueNoise(float2 p)
{
	float ign = InterleavedGradientNoise(p);
	float shifted = ign < 0.5 ? sqrt(2.0 * ign) - 1.0 : 1.0 - sqrt(2.0 - 2.0 * ign);
	return shifted * 0.5 + 0.5;
}

float PinkNoise(float2 p, int octaves)
{
	float sum = 0.0;
	float amplitude_sum = 0.0;
	float frequency = 1.0;
	for (int i = 0; i < octaves; ++i)
	{
		float amplitude = 1.0 / frequency;
		sum += WhiteNoise(p * frequency) * amplitude;
		amplitude_sum += amplitude;
		frequency *= 2.0;
	}
	return sum / amplitude_sum;
}

float PinkNoise(float3 p, int octaves)
{
	float sum = 0.0;
	float amplitude_sum = 0.0;
	float frequency = 1.0;
	for (int i = 0; i < octaves; ++i)
	{
		float amplitude = 1.0 / frequency;
		sum += WhiteNoise(p * frequency) * amplitude;
		amplitude_sum += amplitude;
		frequency *= 2.0;
	}
	return sum / amplitude_sum;
}

float BrownNoise(float2 p, int octaves)
{
	float sum = 0.0;
	float amplitude_sum = 0.0;
	float frequency = 1.0;
	for (int i = 0; i < octaves; ++i)
	{
		float amplitude = 1.0 / (frequency * frequency);
		sum += WhiteNoise(p * frequency) * amplitude;
		amplitude_sum += amplitude;
		frequency *= 2.0;
	}
	return sum / amplitude_sum;
}

float BrownNoise(float3 p, int octaves)
{
	float sum = 0.0;
	float amplitude_sum = 0.0;
	float frequency = 1.0;
	for (int i = 0; i < octaves; ++i)
	{
		float amplitude = 1.0 / (frequency * frequency);
		sum += WhiteNoise(p * frequency) * amplitude;
		amplitude_sum += amplitude;
		frequency *= 2.0;
	}
	return sum / amplitude_sum;
}

float Fbm(float2 p, int octaves, float persistence = 0.5, float lacunarity = 2.0)
{
	float sum = 0.0;
	float amplitude = 0.5;
	float frequency = 1.0;
	for (int i = 0; i < octaves; ++i)
	{
		sum += PerlinNoise(p * frequency) * amplitude;
		frequency *= lacunarity;
		amplitude *= persistence;
	}
	return sum;
}

float Fbm(float3 p, int octaves, float persistence = 0.5, float lacunarity = 2.0)
{
	float sum = 0.0;
	float amplitude = 0.5;
	float frequency = 1.0;
	for (int i = 0; i < octaves; ++i)
	{
		sum += PerlinNoise(p * frequency) * amplitude;
		frequency *= lacunarity;
		amplitude *= persistence;
	}
	return sum;
}

float Turbulence(float2 p, int octaves, float persistence = 0.5, float lacunarity = 2.0)
{
	float sum = 0.0;
	float amplitude = 0.5;
	float frequency = 1.0;
	for (int i = 0; i < octaves; ++i)
	{
		sum += abs(PerlinNoise(p * frequency)) * amplitude;
		frequency *= lacunarity;
		amplitude *= persistence;
	}
	return sum;
}

float Turbulence(float3 p, int octaves, float persistence = 0.5, float lacunarity = 2.0)
{
	float sum = 0.0;
	float amplitude = 0.5;
	float frequency = 1.0;
	for (int i = 0; i < octaves; ++i)
	{
		sum += abs(PerlinNoise(p * frequency)) * amplitude;
		frequency *= lacunarity;
		amplitude *= persistence;
	}
	return sum;
}

float2 CurlNoise2(float2 p, float epsilon = 0.001)
{
	float dx = (PerlinNoise(p + float2(epsilon, 0)) - PerlinNoise(p - float2(epsilon, 0))) / (2.0 * epsilon);
	float dy = (PerlinNoise(p + float2(0, epsilon)) - PerlinNoise(p - float2(0, epsilon))) / (2.0 * epsilon);
	return float2(dy, -dx);
}

float3 CurlNoise3(float3 p, float epsilon = 0.001)
{
	float3 dx = float3(0, 0, 0), dy = float3(0, 0, 0), dz = float3(0, 0, 0);

	float3 offset_y = float3(19.1, 33.4, 47.2);
	float3 offset_z = float3(74.3, 12.5, 61.8);

	float p_y1 = PerlinNoise(p + offset_y + float3(0, epsilon, 0));
	float p_y0 = PerlinNoise(p + offset_y - float3(0, epsilon, 0));
	float p_z1 = PerlinNoise(p + offset_y + float3(0, 0, epsilon));
	float p_z0 = PerlinNoise(p + offset_y - float3(0, 0, epsilon));

	float q_x1 = PerlinNoise(p + offset_z + float3(epsilon, 0, 0));
	float q_x0 = PerlinNoise(p + offset_z - float3(epsilon, 0, 0));
	float q_z1 = PerlinNoise(p + offset_z + float3(0, 0, epsilon));
	float q_z0 = PerlinNoise(p + offset_z - float3(0, 0, epsilon));

	float r_x1 = PerlinNoise(p + float3(epsilon, 0, 0));
	float r_x0 = PerlinNoise(p - float3(epsilon, 0, 0));
	float r_y1 = PerlinNoise(p + float3(0, epsilon, 0));
	float r_y0 = PerlinNoise(p - float3(0, epsilon, 0));

	float curl_x = (p_y1 - p_y0) - (q_z1 - q_z0);
	float curl_y = (q_z1 - q_z0) - (r_x1 - r_x0);
	float curl_z = (r_x1 - r_x0) - (p_y1 - p_y0);

	return float3(curl_x, curl_y, curl_z) / (2.0 * epsilon);
}

float2 DomainWarp2(float2 p, float warp_strength, int fbm_octaves = 4)
{
	float2 warp = float2(Fbm(p, fbm_octaves), Fbm(p + float2(5.2, 1.3), fbm_octaves));
	return p + warp * warp_strength;
}

float3 DomainWarp3(float3 p, float warp_strength, int fbm_octaves = 4)
{
	float3 warp = float3(Fbm(p, fbm_octaves), Fbm(p + float3(5.2, 1.3, 9.4), fbm_octaves), Fbm(p + float3(3.1, 8.6, 2.7), fbm_octaves));
	return p + warp * warp_strength;
}

void BuildOrthonormalBasis(float3 normal, out float3 tangent, out float3 bitangent)
{
	float s = normal.z >= 0.0 ? 1.0 : -1.0;
	float a = -1.0 / (s + normal.z);
	float b = normal.x * normal.y * a;
	tangent   = float3(1.0 + s * normal.x * normal.x * a, s * b, -s * normal.x);
	bitangent = float3(b, s + normal.y * normal.y * a, -normal.y);
}

float3 CosineSampleHemisphere(inout uint state, float3 normal)
{
	float2 u = Rand2(state);
	float r = sqrt(u.x);
	float phi = 2.0 * NOISE_PI * u.y;

	float3 h = float3(r * cos(phi), r * sin(phi), sqrt(max(0.0, 1.0 - u.x)));

	float3 tangent, bitangent;
	BuildOrthonormalBasis(normal, tangent, bitangent);
	return normalize(tangent * h.x + bitangent * h.y + normal * h.z);
}

float3 UniformSampleSphere(inout uint state)
{
	float2 u = Rand2(state);
	float z = 1.0 - 2.0 * u.x;
	float r = sqrt(max(0.0, 1.0 - z * z));
	float phi = 2.0 * NOISE_PI * u.y;
	return float3(r * cos(phi), r * sin(phi), z);
}

float3 SampleCone(inout uint state, float3 direction, float cos_theta_max)
{
	float2 u = Rand2(state);
	float cos_theta = 1.0 - u.x * (1.0 - cos_theta_max);
	float sin_theta = sqrt(max(0.0, 1.0 - cos_theta * cos_theta));
	float phi = 2.0 * NOISE_PI * u.y;

	float3 local_direction = float3(sin_theta * cos(phi), sin_theta * sin(phi), cos_theta);

	float3 tangent, bitangent;
	BuildOrthonormalBasis(direction, tangent, bitangent);
	return normalize(tangent * local_direction.x + bitangent * local_direction.y + direction * local_direction.z);
}

#endif

